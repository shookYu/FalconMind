/**
 * @file BuiltinPlugins.cpp
 * @brief SDK内建插件实现
 * 
 * 提供默认的检测、跟踪、导航、任务规划能力实现。
 * 这些实现作为基础功能，可以通过动态插件进行替换。
 */

#include "falconmind/sdk/plugin/BuiltinPlugins.h"
#include "falconmind/sdk/plugin/IPlugin.h"
#include "falconmind/sdk/plugin/CapabilityRegistry.h"
#include "falconmind/sdk/perception/DetectionTypes.h"
#include "falconmind/sdk/perception/TrackingTypes.h"
#include "falconmind/sdk/perception/TargetTypes.h"
#include "falconmind/sdk/sensors/NavigationTypes.h"
#include "falconmind/sdk/sensors/SensorTypes.h"
#include "falconmind/sdk/flight/GuidanceTypes.h"
#include "falconmind/sdk/mission/MissionTypes.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include "falconmind/sdk/obstacles/ObstacleTypes.h"
#include "falconmind/sdk/core/ConfigManager.h"
#include "falconmind/sdk/core/ErrorCode.h"

#include <cmath>
#include <map>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <optional>
#include <mutex>
#include <random>

namespace falconmind {
namespace sdk {
namespace plugin {

using namespace perception;
using namespace sensors;
using namespace flight;
using namespace mission;
using namespace obstacles;

//=============================================================================
// 1. 基础检测器 - 基于帧差分的简单运动检测
//=============================================================================

class BuiltinMotionDetector : public IDetectorPlugin {
public:
    BuiltinMotionDetector() = default;
    
    PluginMetadata getMetadata() const override {
        return {
            "builtin_motion_detector",
            "1.0.0",
            "Built-in motion detector using frame differencing",
            "FalconMind SDK",
            "1.0.0",
            PluginType::Detector,
            PluginCapability::RealTime,
            {},
            {},
            {"x86", "arm64"}
        };
    }
    
    bool initialize(const ConfigManager& config) override {
        state_ = PluginState::Initializing;
        
        confidenceThreshold_ = config.get<double>("detector.confidence_threshold", 0.25);
        minObjectSize_ = config.get<int>("detector.min_object_size", 100);
        inputWidth_ = config.get<int>("detector.input_width", 640);
        inputHeight_ = config.get<int>("detector.input_height", 480);
        
        // 分配缓冲区
        prevFrame_.resize(inputWidth_ * inputHeight_, 0);
        
        state_ = PluginState::Active;
        return true;
    }
    
    void shutdown() override {
        state_ = PluginState::Unloading;
        prevFrame_.clear();
        state_ = PluginState::Unloaded;
    }
    
    PluginState getState() const override {
        return state_;
    }
    
    bool loadModel(const std::string& modelPath, const std::string& device) override {
        // 运动检测器不需要模型文件
        return true;
    }
    
    DetectionResult detect(const ImageView& image) override {
        DetectionResult result;
        result.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        result.frameIndex = frameIndex_++;
        
        if (!image.data || image.width != inputWidth_ || image.height != inputHeight_) {
            return result;
        }
        
        // 转换为灰度图
        std::vector<uint8_t> grayFrame(inputWidth_ * inputHeight_);
        convertToGrayscale(image, grayFrame.data());
        
        // 如果不是第一帧，计算帧差
        if (!prevFrame_.empty() && frameIndex_ > 1) {
            result.detections = detectMotionRegions(grayFrame.data());
        }
        
        // 保存当前帧
        prevFrame_ = grayFrame;
        
        return result;
    }
    
    std::vector<std::string> getSupportedClasses() const override {
        return {"motion", "unknown"};
    }
    
    void setConfidenceThreshold(float threshold) override {
        confidenceThreshold_ = threshold;
    }
    
    void setInputSize(int width, int height) override {
        inputWidth_ = width;
        inputHeight_ = height;
        prevFrame_.resize(width * height, 0);
    }
    
    std::map<std::string, std::string> getModelInfo() const override {
        return {
            {"type", "motion_detection"},
            {"algorithm", "frame_differencing"},
            {"input_size", std::to_string(inputWidth_) + "x" + std::to_string(inputHeight_)}
        };
    }

private:
    void convertToGrayscale(const ImageView& image, uint8_t* gray) {
        // 简单的RGB/BGR转灰度
        for (int y = 0; y < image.height; ++y) {
            for (int x = 0; x < image.width; ++x) {
                int idx = y * image.width + x;
                if (image.pixelFormat == "RGB8") {
                    int offset = idx * 3;
                    gray[idx] = static_cast<uint8_t>(
                        (image.data[offset] * 0.299f + 
                         image.data[offset + 1] * 0.587f + 
                         image.data[offset + 2] * 0.114f));
                } else if (image.pixelFormat == "BGR8") {
                    int offset = idx * 3;
                    gray[idx] = static_cast<uint8_t>(
                        (image.data[offset + 2] * 0.299f + 
                         image.data[offset + 1] * 0.587f + 
                         image.data[offset] * 0.114f));
                } else {
                    gray[idx] = image.data[idx];
                }
            }
        }
    }
    
    std::vector<Detection> detectMotionRegions(const uint8_t* grayFrame) {
        std::vector<Detection> detections;
        
        // 差分阈值
        const uint8_t threshold = 30;
        const int minRegionSize = minObjectSize_;
        
        // 创建二值差分图
        std::vector<bool> diffMask(inputWidth_ * inputHeight_, false);
        int diffPixels = 0;
        
        for (int i = 0; i < inputWidth_ * inputHeight_; ++i) {
            int diff = std::abs(static_cast<int>(grayFrame[i]) - static_cast<int>(prevFrame_[i]));
            if (diff > threshold) {
                diffMask[i] = true;
                diffPixels++;
            }
        }
        
        // 如果差分像素足够多，检测为运动
        if (diffPixels > minRegionSize) {
            // 计算运动区域的边界框
            int minX = inputWidth_, minY = inputHeight_;
            int maxX = 0, maxY = 0;
            
            for (int y = 0; y < inputHeight_; ++y) {
                for (int x = 0; x < inputWidth_; ++x) {
                    if (diffMask[y * inputWidth_ + x]) {
                        minX = std::min(minX, x);
                        minY = std::min(minY, y);
                        maxX = std::max(maxX, x);
                        maxY = std::max(maxY, y);
                    }
                }
            }
            
            if (maxX > minX && maxY > minY) {
                Detection det;
                det.bbox.x = static_cast<float>(minX);
                det.bbox.y = static_cast<float>(minY);
                det.bbox.width = static_cast<float>(maxX - minX);
                det.bbox.height = static_cast<float>(maxY - minY);
                det.score = std::min(1.0f, static_cast<float>(diffPixels) / minRegionSize);
                det.classId = 0;
                det.className = "motion";
                detections.push_back(det);
            }
        }
        
        return detections;
    }
    
    PluginState state_{PluginState::Unloaded};
    std::vector<uint8_t> prevFrame_;
    uint32_t frameIndex_{0};
    float confidenceThreshold_{0.25f};
    int minObjectSize_{100};
    int inputWidth_{640};
    int inputHeight_{480};
};

//=============================================================================
// 2. 基础跟踪器 - 基于IOU的简单跟踪
//=============================================================================

class BuiltinIoUTracker : public ITrackerPlugin {
public:
    BuiltinIoUTracker() = default;
    
    PluginMetadata getMetadata() const override {
        return {
            "builtin_iou_tracker",
            "1.0.0",
            "Built-in IoU-based tracker",
            "FalconMind SDK",
            "1.0.0",
            PluginType::Tracker,
            PluginCapability::RealTime | PluginCapability::MultiObject,
            {},
            {},
            {"x86", "arm64"}
        };
    }
    
    bool initialize(const ConfigManager& config) override {
        state_ = PluginState::Initializing;
        
        maxTracks_ = config.get<int>("tracker.max_tracks", 100);
        iouThreshold_ = config.get<double>("tracker.iou_threshold", 0.3);
        maxLostFrames_ = config.get<int>("tracker.max_lost_frames", 30);
        
        state_ = PluginState::Active;
        return true;
    }
    
    void shutdown() override {
        state_ = PluginState::Unloading;
        tracks_.clear();
        state_ = PluginState::Unloaded;
    }
    
    PluginState getState() const override {
        return state_;
    }
    
    bool init(int maxTracks) override {
        maxTracks_ = maxTracks;
        return true;
    }
    
    TrackingResult update(const DetectionResult& detections) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        TrackingResult result;
        result.timestampNs = detections.timestampNs;
        result.frameIndex = detections.frameIndex;
        
        // 更新现有跟踪
        for (auto& track : tracks_) {
            track.second.lostFrames++;
        }
        
        // 匹配检测和跟踪
        std::vector<bool> detectionMatched(detections.detections.size(), false);
        
        for (auto& [trackId, track] : tracks_) {
            float bestIou = 0;
            size_t bestDetIdx = 0;
            
            for (size_t i = 0; i < detections.detections.size(); ++i) {
                if (detectionMatched[i]) continue;
                
                float iou = calculateIoU(track.lastBbox, detections.detections[i].bbox);
                if (iou > iouThreshold_ && iou > bestIou) {
                    bestIou = iou;
                    bestDetIdx = i;
                }
            }
            
            if (bestIou > iouThreshold_) {
                // 更新跟踪
                track.lastBbox = detections.detections[bestDetIdx].bbox;
                track.confidence = detections.detections[bestDetIdx].score;
                track.className = detections.detections[bestDetIdx].className;
                track.lostFrames = 0;
                track.hits++;
                
                TrackHistoryPoint point;
                point.timestampNs = detections.timestampNs;
                point.bbox = track.lastBbox;
                track.trajectory.push_back(point);
                
                detectionMatched[bestDetIdx] = true;
            }
        }
        
        // 创建新跟踪
        for (size_t i = 0; i < detections.detections.size(); ++i) {
            if (!detectionMatched[i] && tracks_.size() < static_cast<size_t>(maxTracks_)) {
                InternalTrack newTrack;
                newTrack.id = nextTrackId_++;
                newTrack.lastBbox = detections.detections[i].bbox;
                newTrack.confidence = detections.detections[i].score;
                newTrack.className = detections.detections[i].className;
                newTrack.lostFrames = 0;
                newTrack.hits = 1;
                
                TrackHistoryPoint point;
                point.timestampNs = detections.timestampNs;
                point.bbox = newTrack.lastBbox;
                newTrack.trajectory.push_back(point);
                
                tracks_[newTrack.id] = newTrack;
            }
        }
        
        // 移除丢失太久的跟踪
        for (auto it = tracks_.begin(); it != tracks_.end();) {
            if (it->second.lostFrames > maxLostFrames_) {
                it = tracks_.erase(it);
            } else {
                ++it;
            }
        }
        
        // 构建结果
        for (const auto& [id, track] : tracks_) {
            if (track.lostFrames == 0) {  // 只返回活跃的跟踪
                TrackingState state;
                state.trackId = track.id;
                state.targetClassName = track.className;
                state.status = "ACTIVE";
                state.trajectory = track.trajectory;
                result.tracks.push_back(state);
            }
        }
        
        return result;
    }
    
    std::optional<Track> getTrack(uint64_t trackId) const override {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = tracks_.find(trackId);
        if (it != tracks_.end()) {
            Track track;
            track.id = it->second.id;
            return track;
        }
        return std::nullopt;
    }
    
    void removeTrack(uint64_t trackId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        tracks_.erase(trackId);
    }
    
    std::vector<Track> getActiveTracks() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<Track> active;
        for (const auto& [id, track] : tracks_) {
            if (track.lostFrames == 0) {
                Track t;
                t.id = track.id;
                active.push_back(t);
            }
        }
        return active;
    }
    
    void reset() override {
        std::lock_guard<std::mutex> lock(mutex_);
        tracks_.clear();
        nextTrackId_ = 0;
    }

private:
    struct InternalTrack {
        uint64_t id{0};
        DetectionBBox lastBbox;
        float confidence{0};
        std::string className;
        int lostFrames{0};
        int hits{0};
        std::vector<TrackHistoryPoint> trajectory;
    };
    
    float calculateIoU(const DetectionBBox& a, const DetectionBBox& b) {
        float x1 = std::max(a.x, b.x);
        float y1 = std::max(a.y, b.y);
        float x2 = std::min(a.x + a.width, b.x + b.width);
        float y2 = std::min(a.y + a.height, b.y + b.height);
        
        if (x2 <= x1 || y2 <= y1) return 0.0f;
        
        float intersection = (x2 - x1) * (y2 - y1);
        float areaA = a.width * a.height;
        float areaB = b.width * b.height;
        float union_ = areaA + areaB - intersection;
        
        return union_ > 0 ? intersection / union_ : 0.0f;
    }
    
    PluginState state_{PluginState::Unloaded};
    mutable std::mutex mutex_;
    std::map<uint64_t, InternalTrack> tracks_;
    uint64_t nextTrackId_{0};
    int maxTracks_{100};
    float iouThreshold_{0.3f};
    int maxLostFrames_{30};
};

//=============================================================================
// 3. 基础导航 - GNSS + 惯性导航
//=============================================================================

class BuiltinGNSSInertialNavigation : public INavigationPlugin {
public:
    BuiltinGNSSInertialNavigation() = default;
    
    PluginMetadata getMetadata() const override {
        return {
            "builtin_gnss_inertial_nav",
            "1.0.0",
            "Built-in GNSS + Inertial navigation",
            "FalconMind SDK",
            "1.0.0",
            PluginType::Navigation,
            PluginCapability::AntiSpoofing | PluginCapability::GNSSDenied,
            {},
            {},
            {"x86", "arm64"}
        };
    }
    
    bool initialize(const ConfigManager& config) override {
        state_ = PluginState::Initializing;
        
        useDeadReckoning_ = config.get<bool>("navigation.use_dead_reckoning", true);
        spoofingDetectionEnabled_ = config.get<bool>("navigation.spoofing_detection", true);
        deniedModeThreshold_ = config.get<double>("navigation.denied_threshold", 5.0);
        
        state_ = PluginState::Active;
        return true;
    }
    
    void shutdown() override {
        state_ = PluginState::Unloading;
        state_ = PluginState::Unloaded;
    }
    
    PluginState getState() const override {
        return state_;
    }
    
    bool initializePosition(const sensors::GeoPoint& position) override {
        initialPosition_ = position;
        currentPosition_ = position;
        deadReckoningPosition_ = position;
        positionInitialized_ = true;
        return true;
    }
    
    void updateSensors(const SensorData& data) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        lastSensorData_ = data;
        
        // 更新GNSS位置
        if (data.gnssPosition.has_value()) {
            // 检查欺骗
            if (spoofingDetectionEnabled_ && lastValidPosition_.has_value()) {
                double jumpDistance = calculateDistance(
                    data.gnssPosition.value(), lastValidPosition_.value());
                
                // 如果位置跳跃太大，可能是欺骗
                if (jumpDistance > 100.0) {  // 100米跳跃阈值
                    spoofingDetected_ = true;
                    gnssAvailable_ = false;
                } else {
                    spoofingDetected_ = false;
                    currentPosition_ = data.gnssPosition.value();
                    deadReckoningPosition_ = data.gnssPosition.value();
                    lastValidPosition_ = data.gnssPosition.value();
                    gnssAvailable_ = true;
                    gnssLostCounter_ = 0;
                }
            } else {
                currentPosition_ = data.gnssPosition.value();
                deadReckoningPosition_ = data.gnssPosition.value();
                lastValidPosition_ = data.gnssPosition.value();
                gnssAvailable_ = true;
            }
        } else {
            gnssLostCounter_++;
            if (gnssLostCounter_ > deniedModeThreshold_ * 10) {  // 假设10Hz更新
                gnssAvailable_ = false;
                inDeniedMode_ = true;
            }
        }
        
        // 惯性导航更新（当GNSS不可用时）
        if (!gnssAvailable_ && useDeadReckoning_) {
            updateDeadReckoning(data);
        }
        
        // 更新姿态
        if (data.imuHealthy) {
            // 简化的姿态估计（实际应该用卡尔曼滤波）
            // 这里只是演示
        }
        
        // 更新速度
        if (data.gnssPosition.has_value() && lastValidPosition_.has_value() && data.timestampNs > lastUpdateTime_) {
            double dt = (data.timestampNs - lastUpdateTime_) / 1e9;
            if (dt > 0) {
                // 计算速度（简单的差分）
                double distance = calculateDistance(currentPosition_, lastValidPosition_.value());
                double speed = distance / dt;
                
                // 简化的ENU速度计算
                double dLat = currentPosition_.latitude - lastValidPosition_.value().latitude;
                double dLon = currentPosition_.longitude - lastValidPosition_.value().longitude;
                
                currentVelocity_.north = dLat * 111320.0 / dt;  // 粗略转换
                currentVelocity_.east = dLon * 111320.0 * std::cos(currentPosition_.latitude * M_PI / 180.0) / dt;
            }
        }
        
        lastUpdateTime_ = data.timestampNs;
    }
    
    sensors::GeoPoint getPosition() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return inDeniedMode_ ? deadReckoningPosition_ : currentPosition_;
    }
    
    Attitude getAttitude() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return currentAttitude_;
    }
    
    Velocity getVelocity() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return currentVelocity_;
    }
    
    bool isGNSSSpoofed() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return spoofingDetected_;
    }
    
    bool isInDeniedEnvironment() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return inDeniedMode_;
    }
    
    double getPositionAccuracy() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (gnssAvailable_ && lastSensorData_.gnssAccuracy.has_value()) {
            return lastSensorData_.gnssAccuracy.value();
        } else if (inDeniedMode_) {
            // 拒止模式下精度随时间降低
            return 5.0 + gnssLostCounter_ * 0.5;  // 每丢失一帧增加0.5米误差
        }
        return 10.0;  // 默认值
    }
    
    void reset() override {
        std::lock_guard<std::mutex> lock(mutex_);
        positionInitialized_ = false;
        gnssAvailable_ = false;
        inDeniedMode_ = false;
        spoofingDetected_ = false;
        gnssLostCounter_ = 0;
        currentVelocity_ = Velocity{};
        currentAttitude_ = Attitude{};
    }

private:
    void updateDeadReckoning(const SensorData& data) {
        if (!data.imuHealthy) return;
        
        // 简化的航位推算
        // 积分加速度得到速度变化，积分速度得到位置变化
        if (data.timestampNs > lastUpdateTime_ && 
            data.linearAcceleration[0].has_value() &&
            data.linearAcceleration[1].has_value()) {
            
            double dt = (data.timestampNs - lastUpdateTime_) / 1e9;
            
            // 加速度积分（简化的，实际需要去除重力、坐标转换等）
            double ax = data.linearAcceleration[0].value();
            double ay = data.linearAcceleration[1].value();
            
            // 更新速度
            currentVelocity_.north += ay * dt;
            currentVelocity_.east += ax * dt;
            
            // 速度积分更新位置（粗略的本地坐标更新）
            double dNorth = currentVelocity_.north * dt;
            double dEast = currentVelocity_.east * dt;
            
            // 转换为经纬度变化（粗略）
            deadReckoningPosition_.latitude += dNorth / 111320.0;
            deadReckoningPosition_.longitude += dEast / (111320.0 * std::cos(deadReckoningPosition_.latitude * M_PI / 180.0));
        }
    }
    
    double calculateDistance(const sensors::GeoPoint& a, const sensors::GeoPoint& b) {
        // 简化的平面距离计算（适用于短距离）
        double dLat = (a.latitude - b.latitude) * 111320.0;
        double dLon = (a.longitude - b.longitude) * 111320.0 * std::cos(a.latitude * M_PI / 180.0);
        return std::sqrt(dLat * dLat + dLon * dLon);
    }
    
    PluginState state_{PluginState::Unloaded};
    mutable std::mutex mutex_;
    
    bool positionInitialized_{false};
    bool gnssAvailable_{false};
    bool inDeniedMode_{false};
    bool spoofingDetected_{false};
    
    sensors::GeoPoint initialPosition_;
    sensors::GeoPoint currentPosition_;
    sensors::GeoPoint deadReckoningPosition_;
    std::optional<sensors::GeoPoint> lastValidPosition_;
    
    Attitude currentAttitude_;
    Velocity currentVelocity_;
    
    SensorData lastSensorData_;
    uint64_t lastUpdateTime_{0};
    int gnssLostCounter_{0};
    
    bool useDeadReckoning_{true};
    bool spoofingDetectionEnabled_{true};
    double deniedModeThreshold_{5.0};
};

//=============================================================================
// 4. 基础任务规划器 - 直线航点规划
//=============================================================================

class BuiltinStraightLinePlanner : public IMissionPlannerPlugin {
public:
    BuiltinStraightLinePlanner() = default;
    
    PluginMetadata getMetadata() const override {
        return {
            "builtin_straight_line_planner",
            "1.0.0",
            "Built-in straight-line waypoint planner",
            "FalconMind SDK",
            "1.0.0",
            PluginType::MissionPlanner,
            PluginCapability::RealTime,
            {},
            {},
            {"x86", "arm64"}
        };
    }
    
    bool initialize(const ConfigManager& config) override {
        state_ = PluginState::Initializing;
        
        defaultSpeed_ = config.get<double>("planner.default_speed", 5.0);
        waypointSpacing_ = config.get<double>("planner.waypoint_spacing", 50.0);
        minAltitude_ = config.get<double>("planner.min_altitude", 10.0);
        maxAltitude_ = config.get<double>("planner.max_altitude", 120.0);
        
        state_ = PluginState::Active;
        return true;
    }
    
    void shutdown() override {
        state_ = PluginState::Unloading;
        state_ = PluginState::Unloaded;
    }
    
    PluginState getState() const override {
        return state_;
    }
    
    std::vector<Waypoint> plan(
        const MissionDefinition& mission,
        const PlanningConstraints& constraints) override {
        
        std::vector<Waypoint> waypoints;
        
        // 如果任务已经有航点，进行验证和优化
        if (!mission.waypoints.empty()) {
            waypoints = optimize(mission.waypoints);
        } else {
            // 生成简单的航点（例如，从起点到终点的直线）
            // 这里只是一个示例实现
        }
        
        // 应用约束
        applyConstraints(waypoints, constraints);
        
        return waypoints;
    }
    
    std::vector<Waypoint> replan(
        const Waypoint& currentPosition,
        const std::vector<Obstacle>& obstacles) override {
        
        // 简单的重规划：如果路径上有障碍物，添加中间航点绕过
        std::vector<Waypoint> newPath;
        newPath.push_back(currentPosition);
        
        // 检查障碍物并生成避障路径（简化实现）
        for (const auto& obstacle : obstacles) {
            if (obstacle.isValid() && obstacle.distanceToLocal(0, 0, 0) < 20.0) {
                // 生成绕过障碍物的航点
                Waypoint bypass;
                bypass.position.latitude = currentPosition.position.latitude + 0.0001;
                bypass.position.longitude = currentPosition.position.longitude + 0.0001;
                bypass.position.altitude = currentPosition.position.altitude + 10;
                bypass.speed = currentPosition.speed;
                newPath.push_back(bypass);
            }
        }
        
        return newPath;
    }
    
    std::vector<Waypoint> optimize(
        const std::vector<Waypoint>& path) override {
        
        if (path.size() < 2) return path;
        
        std::vector<Waypoint> optimized;
        optimized.push_back(path[0]);
        
        // 简化的航点优化：移除共线的中间点
        for (size_t i = 1; i < path.size() - 1; ++i) {
            const auto& prev = optimized.back();
            const auto& curr = path[i];
            const auto& next = path[i + 1];
            
            // 检查是否共线（简化检查）
            double d1 = calculateDistance(prev.position, curr.position);
            double d2 = calculateDistance(curr.position, next.position);
            double d3 = calculateDistance(prev.position, next.position);
            
            // 如果中间点几乎在直线上，跳过它
            if (std::abs(d1 + d2 - d3) > 5.0) {  // 5米容差
                optimized.push_back(curr);
            }
        }
        
        optimized.push_back(path.back());
        
        return optimized;
    }
    
    FeasibilityResult checkFeasibility(
        const std::vector<Waypoint>& path) override {
        
        FeasibilityResult result;
        
        if (path.size() < 2) {
            result.status = FeasibilityStatus::INFEASIBLE;
            result.errors.push_back("Path must have at least 2 waypoints");
            return result;
        }
        
        double totalDistance = 0;
        
        for (size_t i = 0; i < path.size(); ++i) {
            const auto& wp = path[i];
            
            // 检查高度约束
            if (wp.position.altitude < minAltitude_) {
                result.warnings.push_back("Waypoint " + std::to_string(i) + " below minimum altitude");
            }
            if (wp.position.altitude > maxAltitude_) {
                result.errors.push_back("Waypoint " + std::to_string(i) + " exceeds maximum altitude");
            }
            
            // 累加距离
            if (i > 0) {
                totalDistance += calculateDistance(path[i-1].position, wp.position);
            }
        }
        
        result.estimatedDistance = static_cast<float>(totalDistance);
        result.estimatedTime = static_cast<float>(totalDistance / defaultSpeed_);
        
        if (result.errors.empty()) {
            result.status = result.warnings.empty() ? 
                FeasibilityStatus::FEASIBLE : FeasibilityStatus::PARTIALLY_FEASIBLE;
            result.message = "Path is feasible";
        } else {
            result.status = FeasibilityStatus::INFEASIBLE;
            result.message = "Path has errors";
        }
        
        return result;
    }

private:
    void applyConstraints(std::vector<Waypoint>& waypoints, 
                          const PlanningConstraints& constraints) {
        for (auto& wp : waypoints) {
            // 应用速度约束
            wp.speed = std::min(wp.speed, constraints.maxSpeed);
            wp.speed = std::max(wp.speed, 1.0f);  // 最小1m/s
            
            // 应用高度约束
            wp.position.altitude = std::min(wp.position.altitude, 
                                            static_cast<double>(constraints.maxAltitude));
            wp.position.altitude = std::max(wp.position.altitude, 
                                            static_cast<double>(constraints.minAltitude));
        }
    }
    
    double calculateDistance(const sensors::GeoPoint& a, const sensors::GeoPoint& b) {
        // Haversine公式计算距离
        const double R = 6371000;  // 地球半径（米）
        double lat1 = a.latitude * M_PI / 180.0;
        double lat2 = b.latitude * M_PI / 180.0;
        double dLat = (b.latitude - a.latitude) * M_PI / 180.0;
        double dLon = (b.longitude - a.longitude) * M_PI / 180.0;
        
        double c1 = std::sin(dLat / 2) * std::sin(dLat / 2);
        double c2 = std::cos(lat1) * std::cos(lat2);
        double c3 = std::sin(dLon / 2) * std::sin(dLon / 2);
        double c = 2 * std::atan2(std::sqrt(c1 + c2 * c3), std::sqrt(1 - c1 - c2 * c3));
        
        return R * c;
    }
    
    PluginState state_{PluginState::Unloaded};
    double defaultSpeed_{5.0};
    double waypointSpacing_{50.0};
    double minAltitude_{10.0};
    double maxAltitude_{120.0};
};

//=============================================================================
// 5. 基础视觉制导 - 简单比例导引
//=============================================================================

class BuiltinProportionalGuidance : public IVisualGuidancePlugin {
public:
    BuiltinProportionalGuidance() = default;
    
    PluginMetadata getMetadata() const override {
        return {
            "builtin_proportional_guidance",
            "1.0.0",
            "Built-in proportional navigation guidance",
            "FalconMind SDK",
            "1.0.0",
            PluginType::VisualGuidance,
            PluginCapability::RealTime,
            {},
            {},
            {"x86", "arm64"}
        };
    }
    
    bool initialize(const ConfigManager& config) override {
        state_ = PluginState::Initializing;
        
        kp_ = config.get<double>("guidance.kp", 0.5);
        maxVelocity_ = config.get<double>("guidance.max_velocity", 5.0);
        targetDistance_ = config.get<double>("guidance.target_distance", 10.0);
        
        state_ = PluginState::Active;
        return true;
    }
    
    void shutdown() override {
        state_ = PluginState::Unloading;
        targetLocked_ = false;
        state_ = PluginState::Unloaded;
    }
    
    PluginState getState() const override {
        return state_;
    }
    
    bool setTarget(const Target& target) override {
        if (target.confidence < 0.5) {
            targetLocked_ = false;
            return false;
        }
        
        currentTarget_ = target;
        targetLocked_ = true;
        return true;
    }
    
    GuidanceCommand process(const ImageView& image, const Pose& currentPose) override {
        if (!targetLocked_) {
            return GuidanceCommand::createInvalidCommand("No target locked");
        }
        
        // 计算图像中心误差
        float imageCenterX = image.width / 2.0f;
        float imageCenterY = image.height / 2.0f;
        
        float errorX = currentTarget_.imageX - imageCenterX;
        float errorY = currentTarget_.imageY - imageCenterY;
        
        // 归一化误差
        float normErrorX = errorX / (image.width / 2.0f);
        float normErrorY = errorY / (image.height / 2.0f);
        
        // 比例控制
        float vx = kp_ * normErrorX * maxVelocity_;  // 横向速度
        float vy = 0;  // 前向速度（根据目标距离调整）
        float vz = -kp_ * normErrorY * maxVelocity_;  // 垂直速度
        
        // 根据目标距离调整前向速度
        if (currentTarget_.estimatedDistance > targetDistance_) {
            vy = std::min(static_cast<float>(maxVelocity_),
                static_cast<float>(currentTarget_.estimatedDistance - targetDistance_));
        } else if (currentTarget_.estimatedDistance < targetDistance_ * 0.8) {
            vy = -maxVelocity_ * 0.5f;  // 后退
        }
        
        // 限制速度
        float speed = std::sqrt(vx * vx + vy * vy + vz * vz);
        if (speed > maxVelocity_) {
            float scale = maxVelocity_ / speed;
            vx *= scale;
            vy *= scale;
            vz *= scale;
        }
        
        // 计算偏航率（让机头指向目标）
        float yawRate = kp_ * normErrorX * 30.0f;  // 最大30度/秒
        
        return GuidanceCommand::createVelocityCommand(vy, vx, vz, yawRate);
    }
    
    bool isTargetLocked() const override {
        return targetLocked_;
    }
    
    RelativePosition getTargetRelativePosition() const override {
        RelativePosition rel;
        if (targetLocked_) {
            rel.distance = currentTarget_.estimatedDistance;
            rel.bearing = currentTarget_.relativePosition.has_value() ? 
                currentTarget_.relativePosition.value().bearing : 0.0;
            rel.elevation = currentTarget_.relativePosition.has_value() ? 
                currentTarget_.relativePosition.value().elevation : 0.0;
        }
        return rel;
    }

private:
    PluginState state_{PluginState::Unloaded};
    bool targetLocked_{false};
    Target currentTarget_;
    
    double kp_{0.5};
    double maxVelocity_{5.0};
    double targetDistance_{10.0};
};

//=============================================================================
// 注册函数
//=============================================================================

void registerBuiltinPlugins() {
    auto& registry = CapabilityRegistry::instance();
    
    // 注册内建检测器
    registry.registerDetector("builtin_motion", 
        []() -> std::shared_ptr<IDetectorPlugin> {
            return std::make_shared<BuiltinMotionDetector>();
        });
    
    // 注册内建跟踪器
    registry.registerTracker("builtin_iou",
        []() -> std::shared_ptr<ITrackerPlugin> {
            return std::make_shared<BuiltinIoUTracker>();
        });
    
    // 注册内建导航
    registry.registerNavigation("builtin_gnss_inertial",
        []() -> std::shared_ptr<INavigationPlugin> {
            return std::make_shared<BuiltinGNSSInertialNavigation>();
        });
    
    // 注册拒止导航（使用相同的实现）
    registry.registerNavigation("builtin_gnss_denied",
        []() -> std::shared_ptr<INavigationPlugin> {
            auto nav = std::make_shared<BuiltinGNSSInertialNavigation>();
            return nav;
        });
    
    // 注册防欺骗导航（使用相同的实现）
    registry.registerNavigation("builtin_anti_spoof",
        []() -> std::shared_ptr<INavigationPlugin> {
            auto nav = std::make_shared<BuiltinGNSSInertialNavigation>();
            return nav;
        });
    
    // 注册内建任务规划器
    registry.registerMissionPlanner("builtin_straight_line",
        []() -> std::shared_ptr<IMissionPlannerPlugin> {
            return std::make_shared<BuiltinStraightLinePlanner>();
        });
    
    // 注册内建视觉制导
    registry.registerVisualGuidance("builtin_proportional",
        []() -> std::shared_ptr<IVisualGuidancePlugin> {
            return std::make_shared<BuiltinProportionalGuidance>();
        });
    
    // 设置默认值
    registry.setDefaultDetector("builtin_motion");
    registry.setDefaultTracker("builtin_iou");
    registry.setDefaultNavigation("builtin_gnss_inertial");
    registry.setDefaultMissionPlanner("builtin_straight_line");
    registry.setDefaultVisualGuidance("builtin_proportional");
}

void unregisterBuiltinPlugins() {
    auto& registry = CapabilityRegistry::instance();
    
    // 注销所有内建插件
    // 注意：实际注销逻辑取决于CapabilityRegistry的实现
    // 这里只是占位
}

} // namespace plugin
} // namespace sdk
} // namespace falconmind
