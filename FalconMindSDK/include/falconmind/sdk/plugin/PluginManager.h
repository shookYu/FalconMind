/**
 * @file PluginManager.h
 * @brief 插件管理器 - Linux专用真实动态加载实现
 * 
 * 提供插件的动态加载、版本管理、热更新功能
 * 使用Linux dlopen/dlsym实现真正的动态库加载
 */

#pragma once

#include "falconmind/sdk/plugin/IPlugin.h"
#include "falconmind/sdk/core/ErrorCode.h"
#include <dlfcn.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <filesystem>
#include <chrono>
#include <thread>
#include <functional>
#include <atomic>

namespace falconmind {
namespace sdk {
namespace plugin {

/**
 * @brief Linux动态库句柄封装
 */
class LibraryHandle {
public:
    LibraryHandle() : handle_(nullptr) {}
    explicit LibraryHandle(void* h) : handle_(h) {}
    
    ~LibraryHandle() {
        if (handle_) {
            dlclose(handle_);
        }
    }
    
    LibraryHandle(const LibraryHandle&) = delete;
    LibraryHandle& operator=(const LibraryHandle&) = delete;
    
    LibraryHandle(LibraryHandle&& other) noexcept : handle_(other.handle_) {
        other.handle_ = nullptr;
    }
    
    LibraryHandle& operator=(LibraryHandle&& other) noexcept {
        if (this != &other) {
            if (handle_) {
                dlclose(handle_);
            }
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }
    
    void* get() const { return handle_; }
    bool isValid() const { return handle_ != nullptr; }
    
    template<typename T>
    T* getSymbol(const std::string& name) const {
        return reinterpret_cast<T*>(dlsym(handle_, name.c_str()));
    }
    
    std::string getError() const {
        return std::string(dlerror());
    }

private:
    void* handle_;
};

/**
 * @brief 加载的插件实例
 */
struct LoadedPlugin {
    std::string name;
    std::string path;
    std::string version;
    PluginType type;
    PluginState state;
    LibraryHandle library;
    IPlugin* instance;
    std::filesystem::file_time_type lastModified;
    std::chrono::steady_clock::time_point loadTime;
    
    CreatePluginFunc createFunc;
    DestroyPluginFunc destroyFunc;
    std::function<const char*()> getSDKVersionFunc;
    
    LoadedPlugin() : instance(nullptr), state(PluginState::Unloaded) {}
};

/**
 * @brief 插件加载配置
 */
struct PluginLoadConfig {
    std::string pluginDir = "./plugins";
    bool enableHotReload = true;
    int hotReloadIntervalMs = 5000;
    bool strictVersionCheck = true;
    std::string requiredSDKVersion = "1.0.0";
    std::vector<PluginType> autoLoadTypes;
    std::unordered_map<std::string, std::string> defaultConfigs;
};

/**
 * @brief 插件加载结果
 */
struct PluginLoadResult {
    bool success;
    std::string name;
    std::string version;
    std::string errorMessage;
    std::chrono::milliseconds loadTime;
};

/**
 * @brief Linux插件管理器
 * 
 * 使用dlopen/dlsym实现真实动态库加载
 */
class PluginManager {
public:
    PluginManager();
    ~PluginManager();
    
    /**
     * @brief 初始化插件管理器
     */
    Result<void> initialize(const PluginLoadConfig& config);
    
    /**
     * @brief 关闭所有插件并清理
     */
    void shutdown();
    
    /**
     * @brief 从.so文件加载插件
     */
    Result<PluginLoadResult> loadPlugin(
        const std::filesystem::path& path,
        const core::ConfigManager& config = core::ConfigManager()
    );
    
    /**
     * @brief 卸载插件
     */
    Result<void> unloadPlugin(const std::string& name);
    
    /**
     * @brief 重新加载插件（热更新）
     */
    Result<PluginLoadResult> reloadPlugin(
        const std::string& name,
        const core::ConfigManager& config = core::ConfigManager()
    );
    
    /**
     * @brief 获取插件实例
     */
    IPlugin* getPlugin(const std::string& name) const;
    
    template<typename T>
    T* getPlugin(const std::string& name) const {
        IPlugin* plugin = getPlugin(name);
        return plugin ? dynamic_cast<T*>(plugin) : nullptr;
    }
    
    std::vector<std::string> getLoadedPlugins() const;
    std::vector<std::string> getPluginsByType(PluginType type) const;
    LoadedPlugin* getPluginInfo(const std::string& name);
    bool isPluginLoaded(const std::string& name) const;
    bool isPluginActive(const std::string& name) const;
    
    /**
     * @brief 扫描目录并加载所有.so插件
     */
    std::vector<PluginLoadResult> loadAllPluginsFromDirectory(
        const std::filesystem::path& dir
    );
    
    /**
     * @brief 启用/禁用热更新
     */
    void enableHotReload(bool enable);
    
    /**
     * @brief 手动检查插件更新
     */
    std::vector<PluginLoadResult> checkForUpdates();
    
    using StateChangeCallback = std::function<void(
        const std::string& name, 
        PluginState oldState, 
        PluginState newState
    )>;
    void onStateChange(StateChangeCallback callback);
    
    struct Statistics {
        size_t totalPlugins;
        size_t activePlugins;
        size_t failedPlugins;
        std::chrono::steady_clock::time_point startTime;
        size_t hotReloadCount;
    };
    Statistics getStatistics() const;
    
    std::vector<std::pair<std::string, std::string>> getErrorHistory() const;

private:
    mutable std::mutex mutex_;
    PluginLoadConfig config_;
    std::unordered_map<std::string, std::unique_ptr<LoadedPlugin>> plugins_;
    std::vector<StateChangeCallback> stateCallbacks_;
    std::atomic<bool> running_{false};
    std::thread hotReloadThread_;
    Statistics stats_;
    std::vector<std::pair<std::string, std::string>> errorHistory_;
    
    bool checkVersionCompatibility(const std::string& pluginVersion) const;
    int compareVersions(const std::string& v1, const std::string& v2) const;
    Result<void> initializePlugin(LoadedPlugin& plugin, const core::ConfigManager& config);
    void shutdownPlugin(LoadedPlugin& plugin);
    void hotReloadLoop();
    bool hasPluginFileChanged(const LoadedPlugin& plugin) const;
    void notifyStateChange(const std::string& name, PluginState oldState, PluginState newState);
    Result<LibraryHandle> loadDynamicLibrary(const std::filesystem::path& path);
};

PluginManager& globalPluginManager();

} // namespace plugin
} // namespace sdk
} // namespace falconmind
