/**
 * @file CapabilityRegistry.cpp
 * @brief 能力注册中心实现
 */

#include "falconmind/sdk/plugin/CapabilityRegistry.h"
#include "falconmind/sdk/core/Logger.h"
#include <algorithm>

namespace falconmind {
namespace sdk {
namespace plugin {

// 单例实现
CapabilityRegistry& CapabilityRegistry::instance() {
    static CapabilityRegistry instance;
    return instance;
}

void CapabilityRegistry::initialize(const std::string& pluginDir) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("CapabilityRegistry") << "Initializing capability registry...";
    
    // 初始化插件管理器
    pluginManager_ = std::make_unique<PluginManager>();
    PluginLoadConfig config;
    config.pluginDir = pluginDir;
    config.enableHotReload = true;
    
    auto result = pluginManager_->initialize(config);
    if (!result) {
        LOG_ERROR("CapabilityRegistry") << "Failed to initialize plugin manager: " 
                  << result.error().message();
        return;
    }
    
    // 注册插件加载回调
    pluginManager_->onStateChange([this](const std::string& name, 
                                            PluginState oldState, 
                                            PluginState newState) {
        if (newState == PluginState::Active) {
            auto plugin = pluginManager_->getPlugin(name);
            if (plugin) {
                registerFromPlugin(plugin);
            }
        }
    });
    
    LOG_INFO("CapabilityRegistry") << "Capability registry initialized";
}

void CapabilityRegistry::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("CapabilityRegistry") << "Shutting down capability registry...";
    
    factories_.clear();
    defaults_.clear();
    
    if (pluginManager_) {
        pluginManager_->shutdown();
        pluginManager_.reset();
    }
    
    LOG_INFO("CapabilityRegistry") << "Capability registry shut down";
}

// ========================================================================
// 检测器能力
// ========================================================================

void CapabilityRegistry::registerDetector(const std::string& name, 
                                         DetectorFactory factory,
                                         const std::string& version,
                                         const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    FactoryEntry entry;
    entry.factory = factory;
    entry.info = {
        .name = name,
        .version = version,
        .description = description,
        .type = PluginType::Detector,
        .isBuiltin = true,
        .isLoaded = true,
        .registeredTime = std::chrono::steady_clock::now()
    };
    
    factories_[name] = std::move(entry);
    
    // 如果这是第一个检测器，设为默认
    if (defaults_.find(PluginType::Detector) == defaults_.end()) {
        defaults_[PluginType::Detector] = name;
    }
    
    notifyCapabilityChange(name, PluginType::Detector, true);
    
    LOG_INFO("CapabilityRegistry") << "Registered detector: " << name 
              << " v" << version;
}

std::shared_ptr<IDetectorPlugin> CapabilityRegistry::createDetector(
    const std::string& name,
    const ConfigManager& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        LOG_ERROR("CapabilityRegistry") << "Detector not found: " << name;
        return nullptr;
    }
    
    if (it->second.info.type != PluginType::Detector) {
        LOG_ERROR("CapabilityRegistry") << "Capability " << name << " is not a detector";
        return nullptr;
    }
    
    auto factory = std::get<DetectorFactory>(it->second.factory);
    auto detector = factory();
    
    if (detector) {
        detector->initialize(config);
    }
    
    return detector;
}

std::shared_ptr<IDetectorPlugin> CapabilityRegistry::getDefaultDetector(
    const ConfigManager& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = defaults_.find(PluginType::Detector);
    if (it == defaults_.end()) {
        LOG_ERROR("CapabilityRegistry") << "No default detector registered";
        return nullptr;
    }
    
    return createDetector(it->second, config);
}

void CapabilityRegistry::setDefaultDetector(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end() || it->second.info.type != PluginType::Detector) {
        LOG_ERROR("CapabilityRegistry") << "Invalid detector: " << name;
        return;
    }
    
    defaults_[PluginType::Detector] = name;
    LOG_INFO("CapabilityRegistry") << "Default detector set to: " << name;
}

std::vector<CapabilityInfo> CapabilityRegistry::listDetectors() const {
    return listCapabilitiesByType(PluginType::Detector);
}

// ========================================================================
// 跟踪器能力
// ========================================================================

void CapabilityRegistry::registerTracker(const std::string& name,
                                        TrackerFactory factory,
                                        const std::string& version,
                                        const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    FactoryEntry entry;
    entry.factory = factory;
    entry.info = {
        .name = name,
        .version = version,
        .description = description,
        .type = PluginType::Tracker,
        .isBuiltin = true,
        .isLoaded = true,
        .registeredTime = std::chrono::steady_clock::now()
    };
    
    factories_[name] = std::move(entry);
    
    if (defaults_.find(PluginType::Tracker) == defaults_.end()) {
        defaults_[PluginType::Tracker] = name;
    }
    
    notifyCapabilityChange(name, PluginType::Tracker, true);
    
    LOG_INFO("CapabilityRegistry") << "Registered tracker: " << name 
              << " v" << version;
}

std::shared_ptr<ITrackerPlugin> CapabilityRegistry::createTracker(
    const std::string& name,
    const ConfigManager& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        LOG_ERROR("CapabilityRegistry") << "Tracker not found: " << name;
        return nullptr;
    }
    
    if (it->second.info.type != PluginType::Tracker) {
        LOG_ERROR("CapabilityRegistry") << "Capability " << name << " is not a tracker";
        return nullptr;
    }
    
    auto factory = std::get<TrackerFactory>(it->second.factory);
    auto tracker = factory();
    
    if (tracker) {
        tracker->initialize(config);
    }
    
    return tracker;
}

std::shared_ptr<ITrackerPlugin> CapabilityRegistry::getDefaultTracker(
    const ConfigManager& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = defaults_.find(PluginType::Tracker);
    if (it == defaults_.end()) {
        LOG_ERROR("CapabilityRegistry") << "No default tracker registered";
        return nullptr;
    }
    
    return createTracker(it->second, config);
}

void CapabilityRegistry::setDefaultTracker(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end() || it->second.info.type != PluginType::Tracker) {
        LOG_ERROR("CapabilityRegistry") << "Invalid tracker: " << name;
        return;
    }
    
    defaults_[PluginType::Tracker] = name;
    LOG_INFO("CapabilityRegistry") << "Default tracker set to: " << name;
}

std::vector<CapabilityInfo> CapabilityRegistry::listTrackers() const {
    return listCapabilitiesByType(PluginType::Tracker);
}

// ========================================================================
// 视觉制导能力
// ========================================================================

void CapabilityRegistry::registerVisualGuidance(const std::string& name,
                                               VisualGuidanceFactory factory,
                                               const std::string& version,
                                               const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    FactoryEntry entry;
    entry.factory = factory;
    entry.info = {
        .name = name,
        .version = version,
        .description = description,
        .type = PluginType::VisualGuidance,
        .isBuiltin = true,
        .isLoaded = true,
        .registeredTime = std::chrono::steady_clock::now()
    };
    
    factories_[name] = std::move(entry);
    
    if (defaults_.find(PluginType::VisualGuidance) == defaults_.end()) {
        defaults_[PluginType::VisualGuidance] = name;
    }
    
    notifyCapabilityChange(name, PluginType::VisualGuidance, true);
    
    LOG_INFO("CapabilityRegistry") << "Registered visual guidance: " << name 
              << " v" << version;
}

std::shared_ptr<IVisualGuidancePlugin> CapabilityRegistry::createVisualGuidance(
    const std::string& name,
    const ConfigManager& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        LOG_ERROR("CapabilityRegistry") << "Visual guidance not found: " << name;
        return nullptr;
    }
    
    if (it->second.info.type != PluginType::VisualGuidance) {
        LOG_ERROR("CapabilityRegistry") << "Capability " << name 
                  << " is not visual guidance";
        return nullptr;
    }
    
    auto factory = std::get<VisualGuidanceFactory>(it->second.factory);
    auto guidance = factory();
    
    if (guidance) {
        guidance->initialize(config);
    }
    
    return guidance;
}

std::shared_ptr<IVisualGuidancePlugin> CapabilityRegistry::getDefaultVisualGuidance(
    const ConfigManager& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = defaults_.find(PluginType::VisualGuidance);
    if (it == defaults_.end()) {
        LOG_ERROR("CapabilityRegistry") << "No default visual guidance registered";
        return nullptr;
    }
    
    return createVisualGuidance(it->second, config);
}

void CapabilityRegistry::setDefaultVisualGuidance(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end() || it->second.info.type != PluginType::VisualGuidance) {
        LOG_ERROR("CapabilityRegistry") << "Invalid visual guidance: " << name;
        return;
    }
    
    defaults_[PluginType::VisualGuidance] = name;
    LOG_INFO("CapabilityRegistry") << "Default visual guidance set to: " << name;
}

std::vector<CapabilityInfo> CapabilityRegistry::listVisualGuidance() const {
    return listCapabilitiesByType(PluginType::VisualGuidance);
}

// ========================================================================
// 导航能力
// ========================================================================

void CapabilityRegistry::registerNavigation(const std::string& name,
                                           NavigationFactory factory,
                                           const std::string& version,
                                           const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    FactoryEntry entry;
    entry.factory = factory;
    entry.info = {
        .name = name,
        .version = version,
        .description = description,
        .type = PluginType::Navigation,
        .isBuiltin = true,
        .isLoaded = true,
        .registeredTime = std::chrono::steady_clock::now()
    };
    
    factories_[name] = std::move(entry);
    
    if (defaults_.find(PluginType::Navigation) == defaults_.end()) {
        defaults_[PluginType::Navigation] = name;
    }
    
    notifyCapabilityChange(name, PluginType::Navigation, true);
    
    LOG_INFO("CapabilityRegistry") << "Registered navigation: " << name 
              << " v" << version;
}

std::shared_ptr<INavigationPlugin> CapabilityRegistry::createNavigation(
    const std::string& name,
    const ConfigManager& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        LOG_ERROR("CapabilityRegistry") << "Navigation not found: " << name;
        return nullptr;
    }
    
    if (it->second.info.type != PluginType::Navigation) {
        LOG_ERROR("CapabilityRegistry") << "Capability " << name 
                  << " is not navigation";
        return nullptr;
    }
    
    auto factory = std::get<NavigationFactory>(it->second.factory);
    auto nav = factory();
    
    if (nav) {
        nav->initialize(config);
    }
    
    return nav;
}

std::shared_ptr<INavigationPlugin> CapabilityRegistry::getDefaultNavigation(
    const ConfigManager& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = defaults_.find(PluginType::Navigation);
    if (it == defaults_.end()) {
        LOG_ERROR("CapabilityRegistry") << "No default navigation registered";
        return nullptr;
    }
    
    return createNavigation(it->second, config);
}

void CapabilityRegistry::setDefaultNavigation(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end() || it->second.info.type != PluginType::Navigation) {
        LOG_ERROR("CapabilityRegistry") << "Invalid navigation: " << name;
        return;
    }
    
    defaults_[PluginType::Navigation] = name;
    LOG_INFO("CapabilityRegistry") << "Default navigation set to: " << name;
}

std::vector<CapabilityInfo> CapabilityRegistry::listNavigation() const {
    return listCapabilitiesByType(PluginType::Navigation);
}

std::shared_ptr<INavigationPlugin> CapabilityRegistry::getAntiSpoofNavigation(
    const ConfigManager& config) {
    
    // 查找支持抗欺骗能力的导航
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [name, entry] : factories_) {
        if (entry.info.type == PluginType::Navigation) {
            // 检查能力标志
            // TODO: 从metadata检查AntiSpoofing能力
            auto nav = createNavigation(name, config);
            if (nav) {
                return nav;
            }
        }
    }
    
    // 返回默认导航
    return getDefaultNavigation(config);
}

std::shared_ptr<INavigationPlugin> CapabilityRegistry::getGNSSDeniedNavigation(
    const ConfigManager& config) {
    
    // 查找支持拒止导航的
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& [name, entry] : factories_) {
        if (entry.info.type == PluginType::Navigation) {
            // 检查GNSSDenied能力
            auto nav = createNavigation(name, config);
            if (nav) {
                return nav;
            }
        }
    }
    
    return getDefaultNavigation(config);
}

// ========================================================================
// 任务规划能力
// ========================================================================

void CapabilityRegistry::registerMissionPlanner(const std::string& name,
                                               MissionPlannerFactory factory,
                                               const std::string& version,
                                               const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    FactoryEntry entry;
    entry.factory = factory;
    entry.info = {
        .name = name,
        .version = version,
        .description = description,
        .type = PluginType::MissionPlanner,
        .isBuiltin = true,
        .isLoaded = true,
        .registeredTime = std::chrono::steady_clock::now()
    };
    
    factories_[name] = std::move(entry);
    
    if (defaults_.find(PluginType::MissionPlanner) == defaults_.end()) {
        defaults_[PluginType::MissionPlanner] = name;
    }
    
    notifyCapabilityChange(name, PluginType::MissionPlanner, true);
    
    LOG_INFO("CapabilityRegistry") << "Registered mission planner: " << name 
              << " v" << version;
}

std::shared_ptr<IMissionPlannerPlugin> CapabilityRegistry::createMissionPlanner(
    const std::string& name,
    const ConfigManager& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        LOG_ERROR("CapabilityRegistry") << "Mission planner not found: " << name;
        return nullptr;
    }
    
    if (it->second.info.type != PluginType::MissionPlanner) {
        LOG_ERROR("CapabilityRegistry") << "Capability " << name 
                  << " is not a mission planner";
        return nullptr;
    }
    
    auto factory = std::get<MissionPlannerFactory>(it->second.factory);
    auto planner = factory();
    
    if (planner) {
        planner->initialize(config);
    }
    
    return planner;
}

std::shared_ptr<IMissionPlannerPlugin> CapabilityRegistry::getDefaultMissionPlanner(
    const ConfigManager& config) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = defaults_.find(PluginType::MissionPlanner);
    if (it == defaults_.end()) {
        LOG_ERROR("CapabilityRegistry") << "No default mission planner registered";
        return nullptr;
    }
    
    return createMissionPlanner(it->second, config);
}

void CapabilityRegistry::setDefaultMissionPlanner(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end() || it->second.info.type != PluginType::MissionPlanner) {
        LOG_ERROR("CapabilityRegistry") << "Invalid mission planner: " << name;
        return;
    }
    
    defaults_[PluginType::MissionPlanner] = name;
    LOG_INFO("CapabilityRegistry") << "Default mission planner set to: " << name;
}

std::vector<CapabilityInfo> CapabilityRegistry::listMissionPlanners() const {
    return listCapabilitiesByType(PluginType::MissionPlanner);
}

// ========================================================================
// 插件加载
// ========================================================================

Result<void> CapabilityRegistry::loadCapabilityFromPlugin(
    const std::filesystem::path& path,
    const ConfigManager& config) {
    
    if (!pluginManager_) {
        return Error(ErrorCode::InvalidArgument, 
                    "Plugin manager not initialized");
    }
    
    auto result = pluginManager_->loadPlugin(path, config);
    if (!result) {
        return result.error();
    }
    
    return {};
}

std::vector<std::string> CapabilityRegistry::loadAllCapabilitiesFromDirectory(
    const std::filesystem::path& dir) {
    
    std::vector<std::string> loaded;
    
    if (!pluginManager_) {
        LOG_ERROR("CapabilityRegistry") << "Plugin manager not initialized";
        return loaded;
    }
    
    auto results = pluginManager_->loadAllPluginsFromDirectory(dir);
    
    for (const auto& result : results) {
        if (result.success) {
            loaded.push_back(result.name);
        }
    }
    
    return loaded;
}

Result<void> CapabilityRegistry::unloadCapability(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        return Error(ErrorCode::NodeNotFound, "Capability not found: " + name);
    }
    
    auto type = it->second.info.type;
    factories_.erase(it);
    
    notifyCapabilityChange(name, type, false);
    
    // 如果卸载的是默认能力，清除默认设置
    auto defaultIt = defaults_.find(type);
    if (defaultIt != defaults_.end() && defaultIt->second == name) {
        defaults_.erase(defaultIt);
    }
    
    LOG_INFO("CapabilityRegistry") << "Unloaded capability: " << name;
    
    return {};
}

// ========================================================================
// 查询和监控
// ========================================================================

std::vector<CapabilityInfo> CapabilityRegistry::listAllCapabilities() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<CapabilityInfo> result;
    for (const auto& [name, entry] : factories_) {
        result.push_back(entry.info);
    }
    
    return result;
}

std::vector<CapabilityInfo> CapabilityRegistry::listCapabilitiesByType(PluginType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<CapabilityInfo> result;
    for (const auto& [name, entry] : factories_) {
        if (entry.info.type == type) {
            result.push_back(entry.info);
        }
    }
    
    return result;
}

bool CapabilityRegistry::hasCapability(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return factories_.find(name) != factories_.end();
}

std::optional<PluginType> CapabilityRegistry::getCapabilityType(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        return std::nullopt;
    }
    
    return it->second.info.type;
}

void CapabilityRegistry::onCapabilityChange(CapabilityChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    changeCallbacks_.push_back(callback);
}

void CapabilityRegistry::enableHotReload(bool enable) {
    if (pluginManager_) {
        pluginManager_->enableHotReload(enable);
    }
}

// ========================================================================
// 私有方法
// ========================================================================

void CapabilityRegistry::notifyCapabilityChange(const std::string& name, 
                                               PluginType type,
                                               bool added) {
    for (const auto& callback : changeCallbacks_) {
        try {
            callback(name, type, added);
        } catch (...) {
            // 忽略回调异常
        }
    }
}

void CapabilityRegistry::registerFromPlugin(IPlugin* plugin) {
    if (!plugin) return;
    
    auto metadata = plugin->getMetadata();
    
    LOG_INFO("CapabilityRegistry") << "Registering capability from plugin: " 
              << metadata.name << " (" << static_cast<int>(metadata.type) << ")";
    
    // 注意：从插件加载的实例不能直接注册工厂，
    // 因为插件中的对象是单例。这里我们只是记录元数据，
    // 实际获取通过PluginManager
    
    // TODO: 实现从插件导出工厂的方法
}

} // namespace plugin
} // namespace sdk
} // namespace falconmind
