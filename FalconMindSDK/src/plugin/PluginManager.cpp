/**
 * @file PluginManager.cpp
 * @brief 插件管理器实现 - 真实的动态加载
 */

#include "falconmind/sdk/plugin/PluginManager.h"
#include "falconmind/sdk/core/Logger.h"
#include <sstream>
#include <iostream>

namespace falconmind {
namespace sdk {
namespace plugin {

PluginManager::PluginManager() : running_(false) {
    stats_.startTime = std::chrono::steady_clock::now();
    stats_.totalPlugins = 0;
    stats_.activePlugins = 0;
    stats_.failedPlugins = 0;
    stats_.hotReloadCount = 0;
}

PluginManager::~PluginManager() {
    shutdown();
}

Result<void> PluginManager::initialize(const PluginLoadConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    config_ = config;
    running_ = true;
    
    LOG_INFO("PluginManager") << "Initializing with plugin directory: " << config_.pluginDir;
    
    // 创建插件目录
    if (!std::filesystem::exists(config_.pluginDir)) {
        try {
            std::filesystem::create_directories(config_.pluginDir);
        } catch (const std::exception& e) {
            return Error(ErrorCode::InvalidArgument, 
                        "Failed to create plugin directory", e.what());
        }
    }
    
    // 启动热更新线程
    if (config_.enableHotReload) {
        hotReloadThread_ = std::thread(&PluginManager::hotReloadLoop, this);
        LOG_INFO("PluginManager") << "Hot reload enabled with interval " 
                  << config_.hotReloadIntervalMs << "ms";
    }
    
    // 自动加载指定类型的插件
    if (!config_.autoLoadTypes.empty()) {
        LOG_INFO("PluginManager") << "Auto-loading plugins...";
        
        for (const auto& entry : std::filesystem::directory_iterator(config_.pluginDir)) {
            if (!entry.is_regular_file()) continue;
            
            std::string ext = entry.path().extension().string();
#ifdef __linux__
            if (ext != ".so") continue;
#elif defined(_WIN32)
            if (ext != ".dll") continue;
#endif
            
            auto result = loadPlugin(entry.path());
            if (!result) {
                LOG_WARN("PluginManager") << "Failed to auto-load plugin " 
                          << entry.path().filename() << ": " 
                          << result.error().message();
            }
        }
    }
    
    return {};
}

void PluginManager::shutdown() {
    running_ = false;
    
    if (hotReloadThread_.joinable()) {
        hotReloadThread_.join();
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("PluginManager") << "Shutting down, unloading all plugins...";
    
    // 先停用所有插件
    for (auto& pair : plugins_) {
        auto& plugin = *pair.second;
        if (plugin.state == PluginState::Active) {
            shutdownPlugin(plugin);
        }
    }
    
    // 然后卸载（按依赖顺序）
    for (auto& pair : plugins_) {
        auto& plugin = *pair.second;
        if (plugin.instance && plugin.destroyFunc) {
            plugin.destroyFunc(plugin.instance);
            plugin.instance = nullptr;
        }
    }
    
    plugins_.clear();
    
    LOG_INFO("PluginManager") << "All plugins unloaded";
}

Result<PluginLoadResult> PluginManager::loadPlugin(
    const std::filesystem::path& path,
    const ConfigManager& config) {
    
    auto startTime = std::chrono::steady_clock::now();
    
    LOG_INFO("PluginManager") << "Loading plugin from: " << path;
    
    // 加载动态库
    auto libResult = loadDynamicLibrary(path);
    if (!libResult) {
        return libResult.error();
    }
    
    LibraryHandle library = std::move(libResult.value());
    
    // 获取导出函数
    auto createFunc = library.getSymbol<CreatePluginFunc>("createPlugin");
    auto destroyFunc = library.getSymbol<DestroyPluginFunc>("destroyPlugin");
    auto getVersionFunc = library.getSymbol<const char* (*)()>("getPluginSDKVersion");
    
    if (!createFunc || !destroyFunc || !getVersionFunc) {
        return Error(ErrorCode::InvalidArgument, 
                    "Missing required exports in plugin",
                    "Plugin must export: createPlugin, destroyPlugin, getPluginSDKVersion");
    }
    
    // 版本检查
    const char* sdkVersion = getVersionFunc();
    if (!checkVersionCompatibility(sdkVersion)) {
        std::string error = "SDK version mismatch: plugin requires " + 
                           std::string(sdkVersion) + ", but system is " + 
                           config_.requiredSDKVersion;
        LOG_ERROR("PluginManager") << error;
        return Error(ErrorCode::InvalidArgument, error);
    }
    
    // 创建插件实例
    IPlugin* instance = createFunc();
    if (!instance) {
        return Error(ErrorCode::NodeCreationFailed, "Failed to create plugin instance");
    }
    
    // 获取元数据
    PluginMetadata metadata = instance->getMetadata();
    
    LOG_INFO("PluginManager") << "Plugin info: name=" << metadata.name 
              << ", version=" << metadata.version 
              << ", type=" << static_cast<int>(metadata.type);
    
    // 检查是否已存在
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (plugins_.find(metadata.name) != plugins_.end()) {
            destroyFunc(instance);
            return Error(ErrorCode::NodeAlreadyExists, 
                        "Plugin with name '" + metadata.name + "' already loaded");
        }
    }
    
    // 创建插件记录
    auto plugin = std::make_unique<LoadedPlugin>();
    plugin->name = metadata.name;
    plugin->path = path.string();
    plugin->version = metadata.version;
    plugin->type = metadata.type;
    plugin->state = PluginState::Loaded;
    plugin->library = std::move(library);
    plugin->instance = instance;
    plugin->createFunc = createFunc;
    plugin->destroyFunc = destroyFunc;
    plugin->getSDKVersionFunc = getVersionFunc;
    plugin->lastModified = std::filesystem::last_write_time(path);
    
    // 初始化插件
    auto initResult = initializePlugin(*plugin, config);
    if (!initResult) {
        destroyFunc(instance);
        plugin->instance = nullptr;
        return initResult.error();
    }
    
    // 保存插件
    {
        std::lock_guard<std::mutex> lock(mutex_);
        plugins_[metadata.name] = std::move(plugin);
        stats_.totalPlugins++;
        stats_.activePlugins++;
    }
    
    auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime);
    
    LOG_INFO("PluginManager") << "Plugin " << metadata.name 
              << " loaded successfully in " << loadTime.count() << "ms";
    
    PluginLoadResult result;
    result.success = true;
    result.name = metadata.name;
    result.version = metadata.version;
    result.loadTime = loadTime;
    
    return result;
}

Result<void> PluginManager::unloadPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return Error(ErrorCode::NodeNotFound, "Plugin not found: " + name);
    }
    
    auto& plugin = *it->second;
    
    LOG_INFO("PluginManager") << "Unloading plugin: " << name;
    
    PluginState oldState = plugin.state;
    plugin.state = PluginState::Unloading;
    notifyStateChange(name, oldState, PluginState::Unloading);
    
    // 关闭插件
    shutdownPlugin(plugin);
    
    // 销毁实例
    if (plugin.instance && plugin.destroyFunc) {
        plugin.destroyFunc(plugin.instance);
        plugin.instance = nullptr;
    }
    
    // 库句柄会在 LoadedPlugin 析构时自动关闭
    plugins_.erase(it);
    
    stats_.activePlugins--;
    
    LOG_INFO("PluginManager") << "Plugin " << name << " unloaded";
    
    return {};
}

Result<PluginLoadResult> PluginManager::reloadPlugin(
    const std::string& name,
    const ConfigManager& config) {
    
    std::filesystem::path path;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = plugins_.find(name);
        if (it == plugins_.end()) {
            return Error(ErrorCode::NodeNotFound, "Plugin not found: " + name);
        }
        path = it->second->path;
    }
    
    LOG_INFO("PluginManager") << "Reloading plugin: " << name;
    
    // 先卸载
    auto unloadResult = unloadPlugin(name);
    if (!unloadResult) {
        return unloadResult.error();
    }
    
    // 等待一小段时间确保库完全释放
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 重新加载
    auto loadResult = loadPlugin(path, config);
    
    if (loadResult) {
        stats_.hotReloadCount++;
    }
    
    return loadResult;
}

IPlugin* PluginManager::getPlugin(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return nullptr;
    }
    
    return it->second->instance;
}

std::vector<std::string> PluginManager::getLoadedPlugins() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> names;
    for (const auto& pair : plugins_) {
        names.push_back(pair.first);
    }
    
    return names;
}

std::vector<std::string> PluginManager::getPluginsByType(PluginType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> names;
    for (const auto& pair : plugins_) {
        if (pair.second->type == type) {
            names.push_back(pair.first);
        }
    }
    
    return names;
}

bool PluginManager::isPluginLoaded(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return plugins_.find(name) != plugins_.end();
}

bool PluginManager::isPluginActive(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = plugins_.find(name);
    if (it == plugins_.end()) {
        return false;
    }
    
    return it->second->state == PluginState::Active;
}

void PluginManager::enableHotReload(bool enable) {
    config_.enableHotReload = enable;
    
    if (enable && !hotReloadThread_.joinable()) {
        hotReloadThread_ = std::thread(&PluginManager::hotReloadLoop, this);
    }
}

std::vector<PluginLoadResult> PluginManager::checkForUpdates() {
    std::vector<PluginLoadResult> results;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : plugins_) {
        auto& plugin = *pair.second;
        
        if (hasPluginFileChanged(plugin)) {
            LOG_INFO("PluginManager") << "Plugin " << plugin.name 
                      << " has been updated, reloading...";
            
            // 解锁后重新加载
            ConfigManager config;
            auto result = reloadPlugin(plugin.name, config);
            
            if (result) {
                results.push_back(result.value());
            }
        }
    }
    
    return results;
}

void PluginManager::onStateChange(StateChangeCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    stateCallbacks_.push_back(callback);
}

PluginManager::Statistics PluginManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

std::vector<std::pair<std::string, std::string>> PluginManager::getErrorHistory() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return errorHistory_;
}

// Private methods

Result<LibraryHandle> PluginManager::loadDynamicLibrary(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return Error(ErrorCode::InvalidArgument, 
                    "Plugin file not found: " + path.string());
    }
    
#ifdef __linux__
    void* handle = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        return Error(ErrorCode::NodeCreationFailed, 
                    "Failed to load library", dlerror());
    }
#elif defined(_WIN32)
    HMODULE handle = LoadLibrary(path.c_str());
    if (!handle) {
        DWORD error = GetLastError();
        return Error(ErrorCode::NodeCreationFailed, 
                    "Failed to load library, error code: " + std::to_string(error));
    }
#endif
    
    return LibraryHandle(handle);
}

bool PluginManager::checkVersionCompatibility(const std::string& pluginVersion) const {
    if (!config_.strictVersionCheck) {
        return true;
    }
    
    return compareVersions(pluginVersion, config_.requiredSDKVersion) <= 0;
}

int PluginManager::compareVersions(const std::string& v1, const std::string& v2) const {
    std::vector<int> parts1, parts2;
    
    // 解析版本号
    auto parseVersion = [](const std::string& v) -> std::vector<int> {
        std::vector<int> parts;
        std::stringstream ss(v);
        std::string part;
        
        while (std::getline(ss, part, '.')) {
            parts.push_back(std::stoi(part));
        }
        
        return parts;
    };
    
    parts1 = parseVersion(v1);
    parts2 = parseVersion(v2);
    
    // 比较
    size_t maxLen = std::max(parts1.size(), parts2.size());
    for (size_t i = 0; i < maxLen; ++i) {
        int p1 = i < parts1.size() ? parts1[i] : 0;
        int p2 = i < parts2.size() ? parts2[i] : 0;
        
        if (p1 < p2) return -1;
        if (p1 > p2) return 1;
    }
    
    return 0;
}

Result<void> PluginManager::initializePlugin(
    LoadedPlugin& plugin, 
    const ConfigManager& config) {
    
    PluginState oldState = plugin.state;
    plugin.state = PluginState::Initializing;
    notifyStateChange(plugin.name, oldState, PluginState::Initializing);
    
    // 合并配置
    ConfigManager mergedConfig = config_;
    mergedConfig.merge(config);
    
    // 设置默认配置
    if (config_.defaultConfigs.find(plugin.name) != config_.defaultConfigs.end()) {
        // 从默认配置加载
    }
    
    // 初始化
    if (!plugin.instance->initialize(mergedConfig)) {
        plugin.state = PluginState::Error;
        return Error(ErrorCode::NodeConfigurationFailed, 
                    "Plugin initialization failed: " + plugin.name);
    }
    
    plugin.state = PluginState::Active;
    plugin.loadTime = std::chrono::steady_clock::now();
    notifyStateChange(plugin.name, PluginState::Initializing, PluginState::Active);
    
    return {};
}

void PluginManager::shutdownPlugin(LoadedPlugin& plugin) {
    if (!plugin.instance) return;
    
    PluginState oldState = plugin.state;
    plugin.state = PluginState::Unloading;
    notifyStateChange(plugin.name, oldState, PluginState::Unloading);
    
    plugin.instance->shutdown();
    
    plugin.state = PluginState::Unloaded;
    notifyStateChange(plugin.name, PluginState::Unloading, PluginState::Unloaded);
}

void PluginManager::hotReloadLoop() {
    LOG_INFO("PluginManager") << "Hot reload thread started";
    
    while (running_) {
        if (config_.enableHotReload) {
            checkForUpdates();
        }
        
        // 使用条件变量等待，以便快速退出
        std::this_thread::sleep_for(
            std::chrono::milliseconds(config_.hotReloadIntervalMs));
    }
    
    LOG_INFO("PluginManager") << "Hot reload thread stopped";
}

bool PluginManager::hasPluginFileChanged(const LoadedPlugin& plugin) const {
    try {
        auto currentTime = std::filesystem::last_write_time(plugin.path);
        return currentTime != plugin.lastModified;
    } catch (const std::exception&) {
        return false;
    }
}

void PluginManager::notifyStateChange(
    const std::string& name, 
    PluginState oldState, 
    PluginState newState) {
    
    for (const auto& callback : stateCallbacks_) {
        try {
            callback(name, oldState, newState);
        } catch (...) {
            // 忽略回调异常
        }
    }
}

PluginManager& globalPluginManager() {
    static PluginManager instance;
    return instance;
}

} // namespace plugin
} // namespace sdk
} // namespace falconmind
