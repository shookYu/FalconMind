/**
 * @file IPlugin.h
 * @brief 插件系统核心接口
 * 
 * 定义所有插件必须实现的接口和插件元数据
 * 
 * @example
 * @code
 * // 实现一个检测插件
 * class Yolo26Detector : public IDetectorPlugin {
 * public:
 *     std::string getName() const override { return "yolo26"; }
 *     std::string getVersion() const override { return "1.0.0"; }
 *     
 *     bool initialize(const PluginConfig& config) override {
 *         // 加载模型
 *         return model_.load(config.get<std::string>("model_path"));
 *     }
 *     
 *     DetectionResult detect(const ImageView& image) override {
 *         return model_.infer(image);
 *     }
 * };
 * 
 * // 导出插件
 * EXPORT_PLUGIN(Yolo26Detector)
 * @endcode
 */

#pragma once

#include "falconmind/sdk/core/ErrorCode.h"
#include "falconmind/sdk/core/ConfigManager.h"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace falconmind {
namespace sdk {
namespace plugin {

using namespace core;

/**
 * @brief 插件类型枚举
 */
enum class PluginType {
    Detector,       ///< 目标检测器
    Tracker,        ///< 目标跟踪器
    VisualGuidance, ///< 视觉制导
    MissionPlanner, ///< 任务规划器
    Navigation,     ///< 导航（含拒止导航、防GPS欺骗）
    Sensor,         ///< 传感器驱动
    Communication,  ///< 通信协议
    Custom          ///< 自定义类型
};

/**
 * @brief 插件能力标志
 */
enum class PluginCapability : uint64_t {
    None                    = 0,
    RealTime                = 1ULL << 0,   ///< 支持实时处理
    BatchProcessing         = 1ULL << 1,   ///< 支持批处理
    GPUAccelerated          = 1ULL << 2,   ///< GPU加速
    Quantized               = 1ULL << 3,   ///< 支持量化模型
    MultiObject             = 1ULL << 4,   ///< 多目标检测
    ReIdentificatioin       = 1ULL << 5,   ///< 支持重识别
    AntiSpoofing            = 1ULL << 6,   ///< 防欺骗
    GNSSDenied              = 1ULL << 7,   ///< 拒止环境导航
    VisualOdometry          = 1ULL << 8,   ///< 视觉里程计
    SLAM                    = 1ULL << 9,   ///< SLAM支持
    DynamicObstacleAvoid    = 1ULL << 10,  ///< 动态避障
    TerrainFollowing        = 1ULL << 11,  ///< 地形跟随
    BehaviorTree            = 1ULL << 12,  ///< 行为树支持
    WaypointOptimization    = 1ULL << 13,  ///< 航点优化
};

inline PluginCapability operator|(PluginCapability a, PluginCapability b) {
    return static_cast<PluginCapability>(
        static_cast<uint64_t>(a) | static_cast<uint64_t>(b)
    );
}

inline bool hasCapability(PluginCapability flags, PluginCapability cap) {
    return (static_cast<uint64_t>(flags) & static_cast<uint64_t>(cap)) != 0;
}

/**
 * @brief 插件元数据
 */
struct PluginMetadata {
    std::string name;                    ///< 插件名称（唯一标识）
    std::string version;                 ///< 版本号（语义化版本）
    std::string description;             ///< 描述
    std::string author;                  ///< 作者
    std::string sdkVersion;              ///< 兼容的SDK版本
    PluginType type;                     ///< 插件类型
    PluginCapability capabilities;       ///< 能力标志
    std::vector<std::string> dependencies; ///< 依赖的其他插件
    std::vector<std::string> supportedModels; ///< 支持的模型名称
    std::vector<std::string> supportedPlatforms; ///< 支持的平台（x86, arm64等）
};

/**
 * @brief 插件状态
 */
enum class PluginState {
    Unloaded,    ///< 未加载
    Loading,     ///< 加载中
    Loaded,      ///< 已加载但未初始化
    Initializing,///< 初始化中
    Active,      ///< 正常运行
    Error,       ///< 错误状态
    Unloading    ///< 卸载中
};

/**
 * @brief 插件接口基类
 * 
 * 所有插件必须继承此类
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;
    
    /**
     * @brief 获取插件元数据
     */
    virtual PluginMetadata getMetadata() const = 0;
    
    /**
     * @brief 初始化插件
     * @param config 配置参数
     * @return 是否成功
     */
    virtual bool initialize(const ConfigManager& config) = 0;
    
    /**
     * @brief 关闭插件
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief 获取当前状态
     */
    virtual PluginState getState() const = 0;
    
    /**
     * @brief 获取状态信息（错误时返回错误描述）
     */
    virtual std::string getStatusMessage() const { return "OK"; }
    
    /**
     * @brief 重新加载配置（热更新）
     */
    virtual bool reloadConfig(const ConfigManager& config) { return true; }
    
    /**
     * @brief 检查插件健康状态
     */
    virtual bool isHealthy() const { return getState() == PluginState::Active; }
    
    /**
     * @brief 获取性能统计
     */
    virtual std::map<std::string, double> getPerformanceStats() const {
        return {};
    }
};

/**
 * @brief 检测器插件接口
 */
class IDetectorPlugin : public IPlugin {
public:
    /**
     * @brief 加载模型
     * @param modelPath 模型文件路径
     * @param device 运行设备（"cpu", "cuda:0", "rknn"等）
     */
    virtual bool loadModel(const std::string& modelPath, 
                          const std::string& device = "auto") = 0;
    
    /**
     * @brief 执行检测
     */
    virtual perception::DetectionResult detect(const perception::ImageView& image) = 0;
    
    /**
     * @brief 批量检测
     */
    virtual std::vector<perception::DetectionResult> detectBatch(
        const std::vector<perception::ImageView>& images) {
        std::vector<perception::DetectionResult> results;
        for (const auto& img : images) {
            results.push_back(detect(img));
        }
        return results;
    }
    
    /**
     * @brief 获取支持的类别
     */
    virtual std::vector<std::string> getSupportedClasses() const = 0;
    
    /**
     * @brief 设置置信度阈值
     */
    virtual void setConfidenceThreshold(float threshold) = 0;
    
    /**
     * @brief 设置输入尺寸
     */
    virtual void setInputSize(int width, int height) = 0;
    
    /**
     * @brief 获取模型信息
     */
    virtual std::map<std::string, std::string> getModelInfo() const = 0;
};

/**
 * @brief 跟踪器插件接口
 */
class ITrackerPlugin : public IPlugin {
public:
    /**
     * @brief 初始化跟踪器
     * @param maxTracks 最大跟踪目标数
     */
    virtual bool init(int maxTracks = 100) = 0;
    
    /**
     * @brief 更新跟踪
     * @param detections 当前帧检测结果
     * @return 跟踪结果
     */
    virtual perception::TrackingResult update(
        const perception::DetectionResult& detections) = 0;
    
    /**
     * @brief 获取特定ID的跟踪对象
     */
    virtual std::optional<perception::Track> getTrack(uint64_t trackId) const = 0;
    
    /**
     * @brief 移除跟踪对象
     */
    virtual void removeTrack(uint64_t trackId) = 0;
    
    /**
     * @brief 获取所有活跃跟踪
     */
    virtual std::vector<perception::Track> getActiveTracks() const = 0;
    
    /**
     * @brief 重置跟踪器
     */
    virtual void reset() = 0;
};

/**
 * @brief 视觉制导插件接口
 */
class IVisualGuidancePlugin : public IPlugin {
public:
    /**
     * @brief 设置目标
     * @param target 目标信息（图像坐标或地理坐标）
     */
    virtual bool setTarget(const perception::Target& target) = 0;
    
    /**
     * @brief 处理图像并生成制导指令
     * @param image 当前图像
     * @param currentPose 当前位姿
     * @return 制导指令
     */
    virtual flight::GuidanceCommand process(
        const perception::ImageView& image,
        const sensors::Pose& currentPose) = 0;
    
    /**
     * @brief 检查是否锁定目标
     */
    virtual bool isTargetLocked() const = 0;
    
    /**
     * @brief 获取目标相对位置
     */
    virtual sensors::RelativePosition getTargetRelativePosition() const = 0;
};

/**
 * @brief 导航插件接口（含拒止导航、防欺骗）
 */
class INavigationPlugin : public IPlugin {
public:
    /**
     * @brief 设置初始位置
     */
    virtual bool initializePosition(const sensors::GeoPoint& position) = 0;
    
    /**
     * @brief 更新传感器数据
     */
    virtual void updateSensors(const sensors::SensorData& data) = 0;
    
    /**
     * @brief 获取当前位置估计
     */
    virtual sensors::GeoPoint getPosition() const = 0;
    
    /**
     * @brief 获取当前姿态
     */
    virtual sensors::Attitude getAttitude() const = 0;
    
    /**
     * @brief 获取速度估计
     */
    virtual sensors::Velocity getVelocity() const = 0;
    
    /**
     * @brief 检查GNSS是否被欺骗
     */
    virtual bool isGNSSSpoofed() const = 0;
    
    /**
     * @brief 检查是否处于拒止环境
     */
    virtual bool isInDeniedEnvironment() const = 0;
    
    /**
     * @brief 获取位置精度估计（米）
     */
    virtual double getPositionAccuracy() const = 0;
    
    /**
     * @brief 重置导航状态
     */
    virtual void reset() = 0;
};

/**
 * @brief 任务规划插件接口
 */
class IMissionPlannerPlugin : public IPlugin {
public:
    /**
     * @brief 规划任务
     * @param mission 任务定义
     * @param constraints 约束条件
     * @return 规划结果（航点列表）
     */
    virtual std::vector<mission::Waypoint> plan(
        const mission::MissionDefinition& mission,
        const mission::PlanningConstraints& constraints) = 0;
    
    /**
     * @brief 重新规划（避障或动态调整）
     */
    virtual std::vector<mission::Waypoint> replan(
        const mission::Waypoint& currentPosition,
        const std::vector<obstacles::Obstacle>& obstacles) = 0;
    
    /**
     * @brief 优化现有路径
     */
    virtual std::vector<mission::Waypoint> optimize(
        const std::vector<mission::Waypoint>& path) = 0;
    
    /**
     * @brief 评估路径可行性
     */
    virtual mission::FeasibilityResult checkFeasibility(
        const std::vector<mission::Waypoint>& path) = 0;
};

/**
 * @brief 插件工厂函数类型
 */
using CreatePluginFunc = IPlugin* (*)();
using DestroyPluginFunc = void (*)(IPlugin*);

/**
 * @brief 插件导出宏
 */
#define EXPORT_PLUGIN(PluginClass) \
    extern "C" { \
        __attribute__((visibility("default"))) \
        falconmind::sdk::plugin::IPlugin* createPlugin() { \
            return new PluginClass(); \
        } \
        __attribute__((visibility("default"))) \
        void destroyPlugin(falconmind::sdk::plugin::IPlugin* plugin) { \
            delete plugin; \
        } \
        __attribute__((visibility("default"))) \
        const char* getPluginSDKVersion() { \
            return "1.0.0"; \
        } \
    }

} // namespace plugin
} // namespace sdk
} // namespace falconmind
