/**
 * @file CapabilityRegistry.h
 * @brief 能力注册中心 - 统一管理所有可插拔核心能力
 * 
 * 所有核心能力（检测、跟踪、导航、规划等）都通过此中心注册和获取，
 * 支持运行时动态替换实现，无需修改SDK代码。
 * 
 * @example
 * @code
 * // 注册自定义检测器
 * CapabilityRegistry::registerDetector("yolo26", []() {
 *     return std::make_shared<Yolo26Detector>();
 * });
 * 
 * // 使用
 * auto detector = CapabilityRegistry::createDetector("yolo26");
 * 
 * // 从插件加载
 * CapabilityRegistry::loadFromPlugin("./plugins/libyolo26.so");
 * @endcode
 */

#pragma once

#include "falconmind/sdk/plugin/IPlugin.h"
#include "falconmind/sdk/plugin/PluginManager.h"
#include "falconmind/sdk/core/ErrorCode.h"
#include <memory>
#include <functional>
#include <unordered_map>
#include <mutex>

namespace falconmind {
namespace sdk {
namespace plugin {

/**
 * @brief 能力工厂函数类型
 */
using DetectorFactory = std::function<std::shared_ptr<IDetectorPlugin>()>;
using TrackerFactory = std::function<std::shared_ptr<ITrackerPlugin>()>;
using VisualGuidanceFactory = std::function<std::shared_ptr<IVisualGuidancePlugin>()>;
using NavigationFactory = std::function<std::shared_ptr<INavigationPlugin>()>;
using MissionPlannerFactory = std::function<std::shared_ptr<IMissionPlannerPlugin>()>;

/**
 * @brief 能力信息
 */
struct CapabilityInfo {
    std::string name;
    std::string version;
    std::string description;
    PluginType type;
    bool isBuiltin;
    bool isLoaded;
    std::chrono::steady_clock::time_point registeredTime;
};

/**
 * @brief 能力注册中心
 * 
 * 单例模式，管理所有核心能力的注册、创建和插件加载
 */
class CapabilityRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static CapabilityRegistry& instance();
    
    /**
     * @brief 初始化注册中心
     * @param pluginDir 插件目录
     */
    void initialize(const std::string& pluginDir = "./plugins");
    
    /**
     * @brief 关闭注册中心
     */
    void shutdown();
    
    // ========================================================================
    // 检测器能力
    // ========================================================================
    
    /**
     * @brief 注册检测器工厂
     */
    void registerDetector(const std::string& name, DetectorFactory factory, 
                         const std::string& version = "1.0.0",
                         const std::string& description = "");
    
    /**
     * @brief 创建检测器实例
     */
    std::shared_ptr<IDetectorPlugin> createDetector(const std::string& name,
                                                      const ConfigManager& config = ConfigManager());
    
    /**
     * @brief 获取默认检测器
     */
    std::shared_ptr<IDetectorPlugin> getDefaultDetector(const ConfigManager& config = ConfigManager());
    
    /**
     * @brief 设置默认检测器
     */
    void setDefaultDetector(const std::string& name);
    
    /**
     * @brief 列出所有检测器
     */
    std::vector<CapabilityInfo> listDetectors() const;
    
    // ========================================================================
    // 跟踪器能力
    // ========================================================================
    
    void registerTracker(const std::string& name, TrackerFactory factory,
                        const std::string& version = "1.0.0",
                        const std::string& description = "");
    
    std::shared_ptr<ITrackerPlugin> createTracker(const std::string& name,
                                                   const ConfigManager& config = ConfigManager());
    
    std::shared_ptr<ITrackerPlugin> getDefaultTracker(const ConfigManager& config = ConfigManager());
    
    void setDefaultTracker(const std::string& name);
    
    std::vector<CapabilityInfo> listTrackers() const;
    
    // ========================================================================
    // 视觉制导能力
    // ========================================================================
    
    void registerVisualGuidance(const std::string& name, VisualGuidanceFactory factory,
                               const std::string& version = "1.0.0",
                               const std::string& description = "");
    
    std::shared_ptr<IVisualGuidancePlugin> createVisualGuidance(const std::string& name,
                                                                const ConfigManager& config = ConfigManager());
    
    std::shared_ptr<IVisualGuidancePlugin> getDefaultVisualGuidance(const ConfigManager& config = ConfigManager());
    
    void setDefaultVisualGuidance(const std::string& name);
    
    std::vector<CapabilityInfo> listVisualGuidance() const;
    
    // ========================================================================
    // 导航能力（含拒止导航、防GPS欺骗）
    // ========================================================================
    
    void registerNavigation(const std::string& name, NavigationFactory factory,
                           const std::string& version = "1.0.0",
                           const std::string& description = "");
    
    std::shared_ptr<INavigationPlugin> createNavigation(const std::string& name,
                                                        const ConfigManager& config = ConfigManager());
    
    std::shared_ptr<INavigationPlugin> getDefaultNavigation(const ConfigManager& config = ConfigManager());
    
    void setDefaultNavigation(const std::string& name);
    
    std::vector<CapabilityInfo> listNavigation() const;
    
    /**
     * @brief 获取抗欺骗导航
     */
    std::shared_ptr<INavigationPlugin> getAntiSpoofNavigation(const ConfigManager& config = ConfigManager());
    
    /**
     * @brief 获取拒止环境导航
     */
    std::shared_ptr<INavigationPlugin> getGNSSDeniedNavigation(const ConfigManager& config = ConfigManager());
    
    // ========================================================================
    // 任务规划能力
    // ========================================================================
    
    void registerMissionPlanner(const std::string& name, MissionPlannerFactory factory,
                               const std::string& version = "1.0.0",
                               const std::string& description = "");
    
    std::shared_ptr<IMissionPlannerPlugin> createMissionPlanner(const std::string& name,
                                                                const ConfigManager& config = ConfigManager());
    
    std::shared_ptr<IMissionPlannerPlugin> getDefaultMissionPlanner(const ConfigManager& config = ConfigManager());
    
    void setDefaultMissionPlanner(const std::string& name);
    
    std::vector<CapabilityInfo> listMissionPlanners() const;
    
    // ========================================================================
    // 插件加载
    // ========================================================================
    
    /**
     * @brief 从.so文件加载能力
     */
    Result<void> loadCapabilityFromPlugin(const std::filesystem::path& path,
                                           const ConfigManager& config = ConfigManager());
    
    /**
     * @brief 扫描目录自动加载所有能力插件
     */
    std::vector<std::string> loadAllCapabilitiesFromDirectory(const std::filesystem::path& dir);
    
    /**
     * @brief 卸载能力
     */
    Result<void> unloadCapability(const std::string& name);
    
    // ========================================================================
    // 查询和监控
    // ========================================================================
    
    /**
     * @brief 获取所有能力信息
     */
    std::vector<CapabilityInfo> listAllCapabilities() const;
    
    /**
     * @brief 获取特定类型的能力
     */
    std::vector<CapabilityInfo> listCapabilitiesByType(PluginType type) const;
    
    /**
     * @brief 检查能力是否存在
     */
    bool hasCapability(const std::string& name) const;
    
    /**
     * @brief 获取能力类型
     */
    std::optional<PluginType> getCapabilityType(const std::string& name) const;
    
    /**
     * @brief 注册能力变更回调
     */
    using CapabilityChangeCallback = std::function<void(const std::string& name, 
                                                          PluginType type,
                                                          bool added)>;
    void onCapabilityChange(CapabilityChangeCallback callback);
    
    /**
     * @brief 启用热更新
     */
    void enableHotReload(bool enable);

private:
    CapabilityRegistry() = default;
    ~CapabilityRegistry() = default;
    CapabilityRegistry(const CapabilityRegistry&) = delete;
    CapabilityRegistry& operator=(const CapabilityRegistry&) = delete;
    
    struct FactoryEntry {
        std::variant<DetectorFactory, TrackerFactory, VisualGuidanceFactory,
                     NavigationFactory, MissionPlannerFactory> factory;
        CapabilityInfo info;
    };
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, FactoryEntry> factories_;
    std::unordered_map<PluginType, std::string> defaults_;
    std::vector<CapabilityChangeCallback> changeCallbacks_;
    std::unique_ptr<PluginManager> pluginManager_;
    
    void notifyCapabilityChange(const std::string& name, PluginType type, bool added);
    void registerFromPlugin(IPlugin* plugin);
};

/**
 * @brief 便捷访问函数
 */
inline CapabilityRegistry& registry() {
    return CapabilityRegistry::instance();
}

} // namespace plugin
} // namespace sdk
} // namespace falconmind
