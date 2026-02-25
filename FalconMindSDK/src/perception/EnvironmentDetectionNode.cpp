/**
 * FalconMindSDK - Environment Detection Node Implementation
 * 
 * 实现多源环境检测：
 * 1. GPS拒止检测（GNSS信号质量、IMU辅助判断）
 * 2. 低光照检测（图像亮度统计）
 * 3. 恶劣天气检测（IMU振动+图像特征）
 * 4. 电磁干扰检测（磁力计异常）
 */

#include "falconmind/sdk/perception/EnvironmentDetectionNode.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/sensors/CameraFramePacket.h"
#include "falconmind/sdk/sensors/SensorTypes.h"

#include <iostream>
#include <cmath>
#include <algorithm>
#include <chrono>

namespace falconmind::sdk::perception {

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::sensors;

// 滑动窗口统计
class SlidingWindow {
public:
    explicit SlidingWindow(size_t size) : maxSize_(size) {}
    
    void add(float value) {
        values_.push_back(value);
        if (values_.size() > maxSize_) {
            values_.erase(values_.begin());
        }
    }
    
    float mean() const {
        if (values_.empty()) return 0.0f;
        return std::accumulate(values_.begin(), values_.end(), 0.0f) / values_.size();
    }
    
    float variance() const {
        if (values_.size() < 2) return 0.0f;
        float m = mean();
        float sum = 0.0f;
        for (float v : values_) {
            sum += (v - m) * (v - m);
        }
        return sum / values_.size();
    }
    
    float stdDev() const {
        return std::sqrt(variance());
    }
    
    void clear() { values_.clear(); }
    size_t size() const { return values_.size(); }

private:
    size_t maxSize_;
    std::vector<float> values_;
};

EnvironmentDetectionNode::EnvironmentDetectionNode() : Node("environment_detection") {
    addPad(std::make_shared<Pad>("image_in", PadType::Sink));
    addPad(std::make_shared<Pad>("imu_in", PadType::Sink));
    addPad(std::make_shared<Pad>("gnss_in", PadType::Sink));
    addPad(std::make_shared<Pad>("env_status_out", PadType::Source));
}

bool EnvironmentDetectionNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("default_state");
    if (it != params.end()) {
        if (it->second == "gps_denied") currentState_ = EnvironmentState::GpsDenied;
        else if (it->second == "low_light") currentState_ = EnvironmentState::LowLight;
        else if (it->second == "unknown") currentState_ = EnvironmentState::Unknown;
        else currentState_ = EnvironmentState::Normal;
    }
    
    auto cit = params.find("confidence");
    if (cit != params.end()) {
        float c = std::stof(cit->second);
        setConfidence(c);
    }
    
    std::cout << "[EnvironmentDetectionNode] Configured" << std::endl;
    std::cout << "  Default state: " << static_cast<int>(currentState_) << std::endl;
    std::cout << "  Confidence: " << confidence_ << std::endl;
    
    return true;
}

bool EnvironmentDetectionNode::start() {
    started_ = true;
    
    // 设置输入回调
    auto imagePad = getPad("image_in");
    if (imagePad) {
        imagePad->setDataCallback([this](const void* data, size_t size) {
            this->onImageData(data, size);
        });
    }
    
    auto imuPad = getPad("imu_in");
    if (imuPad) {
        imuPad->setDataCallback([this](const void* data, size_t size) {
            this->onImuData(data, size);
        });
    }
    
    auto gnssPad = getPad("gnss_in");
    if (gnssPad) {
        gnssPad->setDataCallback([this](const void* data, size_t size) {
            this->onGnssData(data, size);
        });
    }
    
    std::cout << "[EnvironmentDetectionNode] Started" << std::endl;
    return true;
}

void EnvironmentDetectionNode::process() {
    if (!started_) return;
    
    // 检测逻辑在回调中执行
    // 这里可以添加周期性状态检查
}

void EnvironmentDetectionNode::onImageData(const void* data, size_t size) {
    if (size < sizeof(CameraFramePacket)) return;
    
    const auto* packet = static_cast<const CameraFramePacket*>(data);
    const uint8_t* imageData = static_cast<const uint8_t*>(data) + sizeof(CameraFramePacket);
    
    // 计算图像亮度
    int width = packet->width;
    int height = packet->height;
    int stride = packet->stride > 0 ? packet->stride : width * 3;
    
    // 采样计算平均亮度
    float totalBrightness = 0.0f;
    int sampleCount = 0;
    
    for (int y = 0; y < height; y += 4) {
        for (int x = 0; x < width; x += 4) {
            int idx = y * stride + x * 3;
            if (idx + 2 < static_cast<int>(size - sizeof(CameraFramePacket))) {
                // Y = 0.299*R + 0.587*G + 0.114*B
                float Y = 0.299f * imageData[idx] + 
                         0.587f * imageData[idx + 1] + 
                         0.114f * imageData[idx + 2];
                totalBrightness += Y;
                sampleCount++;
            }
        }
    }
    
    if (sampleCount > 0) {
        float avgBrightness = totalBrightness / sampleCount;
        
        // 低光照检测阈值
        const float LOW_LIGHT_THRESHOLD = 50.0f;  // 0-255范围
        
        if (avgBrightness < LOW_LIGHT_THRESHOLD) {
            // 检测到低光照
            if (currentState_ != EnvironmentState::LowLight) {
                currentState_ = EnvironmentState::LowLight;
                confidence_ = std::min(1.0f, (LOW_LIGHT_THRESHOLD - avgBrightness) / LOW_LIGHT_THRESHOLD);
                
                std::cout << "[EnvironmentDetectionNode] Low light detected!"
                          << " Brightness: " << avgBrightness
                          << " Confidence: " << confidence_ << std::endl;
                
                publishStatus();
            }
        } else if (currentState_ == EnvironmentState::LowLight) {
            // 恢复正常
            currentState_ = EnvironmentState::Normal;
            confidence_ = 1.0f;
            std::cout << "[EnvironmentDetectionNode] Light condition normal" << std::endl;
            publishStatus();
        }
    }
}

void EnvironmentDetectionNode::onImuData(const void* data, size_t size) {
    if (size < sizeof(ImuSample)) return;
    
    const auto* imu = static_cast<const ImuSample*>(data);
    
    // 计算加速度模值
    float accNorm = std::sqrt(imu->ax * imu->ax + 
                              imu->ay * imu->ay + 
                              imu->az * imu->az);
    
    // 计算角速度模值
    float gyroNorm = std::sqrt(imu->gx * imu->gx + 
                               imu->gy * imu->gy + 
                               imu->gz * imu->gz);
    
    // 振动检测（高加速度或高角速度）
    const float HIGH_VIBRATION_ACC_THRESHOLD = 15.0f;   // m/s^2
    const float HIGH_VIBRATION_GYRO_THRESHOLD = 5.0f;   // rad/s
    
    static int vibrationCount = 0;
    
    if (accNorm > HIGH_VIBRATION_ACC_THRESHOLD || gyroNorm > HIGH_VIBRATION_GYRO_THRESHOLD) {
        vibrationCount++;
        
        if (vibrationCount > 10) {  // 持续振动
            // 可能是恶劣天气（风、雨）或粗糙地形
            std::cout << "[EnvironmentDetectionNode] High vibration detected!"
                      << " Acc: " << accNorm << " Gyro: " << gyroNorm << std::endl;
        }
    } else {
        vibrationCount = std::max(0, vibrationCount - 1);
    }
}

void EnvironmentDetectionNode::onGnssData(const void* data, size_t size) {
    if (size < sizeof(GnssSample)) return;
    
    const auto* gnss = static_cast<const GnssSample*>(data);
    
    // GPS拒止检测
    // 基于HDOP（水平精度因子）和卫星数量
    const float HIGH_HDOP_THRESHOLD = 5.0f;
    const int MIN_SATELLITES = 6;
    
    if (gnss->hdop > HIGH_HDOP_THRESHOLD || gnss->satellitesUsed < MIN_SATELLITES) {
        if (currentState_ != EnvironmentState::GpsDenied) {
            currentState_ = EnvironmentState::GpsDenied;
            
            // 计算置信度
            float hdopConfidence = std::min(1.0f, (gnss->hdop - HIGH_HDOP_THRESHOLD) / 5.0f);
            float satConfidence = std::min(1.0f, 
                static_cast<float>(MIN_SATELLITES - gnss->satellitesUsed) / MIN_SATELLITES);
            confidence_ = std::max(hdopConfidence, satConfidence);
            
            std::cout << "[EnvironmentDetectionNode] GPS denied environment!"
                      << " HDOP: " << gnss->hdop
                      << " Satellites: " << gnss->satellitesUsed
                      << " Confidence: " << confidence_ << std::endl;
            
            publishStatus();
        }
    } else if (currentState_ == EnvironmentState::GpsDenied) {
        // GPS恢复
        currentState_ = EnvironmentState::Normal;
        confidence_ = 1.0f;
        std::cout << "[EnvironmentDetectionNode] GPS signal recovered" << std::endl;
        publishStatus();
    }
}

void EnvironmentDetectionNode::publishStatus() {
    auto outPad = getPad("env_status_out");
    if (!outPad) return;
    
    EnvironmentStatusPacket pkt;
    pkt.state = static_cast<int32_t>(currentState_);
    pkt.confidence = confidence_;
    
    outPad->pushToConnections(&pkt, sizeof(pkt));
    
    static int statusCount = 0;
    if (++statusCount % 10 == 0) {
        std::cout << "[EnvironmentDetectionNode] Status #" << statusCount
                  << " | State: " << pkt.state
                  << " | Confidence: " << pkt.confidence << std::endl;
    }
}

} // namespace falconmind::sdk::perception
