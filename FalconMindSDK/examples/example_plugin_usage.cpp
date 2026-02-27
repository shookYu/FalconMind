/**
 * @file example_plugin_usage.cpp
 * @brief 插件系统使用示例
 * 
 * 展示如何使用PluginManager动态加载和使用插件
 */

#include "falconmind/sdk/plugin/PluginManager.h"
#include "falconmind/sdk/plugin/IPlugin.h"
#include "falconmind/sdk/core/ConfigManager.h"
#include "falconmind/sdk/core/Logger.h"
#include <iostream>

using namespace falconmind::sdk;

int main() {
    // 1. 初始化日志系统
    core::Logger::init({
        .level = core::LogLevel::Info,
        .outputFile = "plugin_demo.log",
        .enableConsole = true
    });
    
    LOG_INFO("Main") << "=== Plugin System Demo ===";
    
    // 2. 初始化插件管理器
    plugin::PluginLoadConfig config;
    config.pluginDir = "./plugins";
    config.enableHotReload = true;
    config.hotReloadIntervalMs = 5000;
    config.strictVersionCheck = true;
    config.requiredSDKVersion = "1.0.0";
    
    auto& pluginManager = plugin::globalPluginManager();
    auto initResult = pluginManager.initialize(config);
    
    if (!initResult) {
        LOG_ERROR("Main") << "Failed to initialize plugin manager: " 
                  << initResult.error().message();
        return 1;
    }
    
    LOG_INFO("Main") << "Plugin manager initialized";
    
    // 3. 手动加载特定插件
    LOG_INFO("Main") << "Loading detector plugin...";
    
    core::ConfigManager pluginConfig;
    pluginConfig.set<std::string>("model_path", "models/yolov8n.onnx");
    pluginConfig.set<float>("confidence_threshold", 0.6f);
    
    auto loadResult = pluginManager.loadPlugin(
        "./plugins/libyolo26_detector.so", 
        pluginConfig
    );
    
    if (!loadResult) {
        LOG_ERROR("Main") << "Failed to load plugin: " << loadResult.error().message();
        return 1;
    }
    
    LOG_INFO("Main") << "Plugin loaded: " << loadResult->name 
              << " v" << loadResult->version 
              << " in " << loadResult->loadTime.count() << "ms";
    
    // 4. 获取插件实例
    auto detector = pluginManager.getPlugin<plugin::IDetectorPlugin>("yolo26_detector");
    
    if (!detector) {
        LOG_ERROR("Main") << "Failed to get detector plugin";
        return 1;
    }
    
    LOG_INFO("Main") << "Got detector plugin instance";
    
    // 5. 使用插件
    LOG_INFO("Main") << "Loading model...";
    if (!detector->loadModel("models/yolov8n.onnx", "cuda")) {
        LOG_ERROR("Main") << "Failed to load model";
        return 1;
    }
    
    LOG_INFO("Main") << "Model loaded successfully";
    
    // 6. 显示插件信息
    auto modelInfo = detector->getModelInfo();
    LOG_INFO("Main") << "Model info:";
    for (const auto& [key, value] : modelInfo) {
        LOG_INFO("Main") << "  " << key << ": " << value;
    }
    
    // 7. 列出支持的类别
    auto classes = detector->getSupportedClasses();
    LOG_INFO("Main") << "Supported classes (" << classes.size() << " total):";
    for (size_t i = 0; i < std::min(classes.size(), size_t(10)); ++i) {
        LOG_INFO("Main") << "  - " << classes[i];
    }
    if (classes.size() > 10) {
        LOG_INFO("Main") << "  ... and " << (classes.size() - 10) << " more";
    }
    
    // 8. 注册状态变更回调
    pluginManager.onStateChange([](const std::string& name, 
                                    plugin::PluginState oldState, 
                                    plugin::PluginState newState) {
        LOG_INFO("PluginState") << "Plugin " << name << " state: " 
                  << static_cast<int>(oldState) << " -> " 
                  << static_cast<int>(newState);
    });
    
    // 9. 查看所有已加载的插件
    LOG_INFO("Main") << "Loaded plugins:";
    auto loadedPlugins = pluginManager.getLoadedPlugins();
    for (const auto& name : loadedPlugins) {
        auto info = pluginManager.getPluginInfo(name);
        if (info) {
            LOG_INFO("Main") << "  - " << name 
                      << " [v" << info->version << "]";
        }
    }
    
    // 10. 按类型查询插件
    LOG_INFO("Main") << "Detector plugins:";
    auto detectors = pluginManager.getPluginsByType(plugin::PluginType::Detector);
    for (const auto& name : detectors) {
        LOG_INFO("Main") << "  - " << name;
    }
    
    // 11. 热更新演示（手动触发）
    LOG_INFO("Main") << "Checking for plugin updates...";
    auto updates = pluginManager.checkForUpdates();
    if (!updates.empty()) {
        LOG_INFO("Main") << "Updated plugins:";
        for (const auto& result : updates) {
            LOG_INFO("Main") << "  - " << result.name 
                      << " reloaded in " << result.loadTime.count() << "ms";
        }
    } else {
        LOG_INFO("Main") << "No updates found";
    }
    
    // 12. 查看统计信息
    auto stats = pluginManager.getStatistics();
    LOG_INFO("Main") << "Plugin statistics:";
    LOG_INFO("Main") << "  Total plugins: " << stats.totalPlugins;
    LOG_INFO("Main") << "  Active plugins: " << stats.activePlugins;
    LOG_INFO("Main") << "  Hot reloads: " << stats.hotReloadCount;
    
    // 13. 卸载插件
    LOG_INFO("Main") << "Unloading plugin...";
    auto unloadResult = pluginManager.unloadPlugin("yolo26_detector");
    if (!unloadResult) {
        LOG_ERROR("Main") << "Failed to unload: " << unloadResult.error().message();
    } else {
        LOG_INFO("Main") << "Plugin unloaded successfully";
    }
    
    // 14. 关闭插件管理器
    LOG_INFO("Main") << "Shutting down...";
    pluginManager.shutdown();
    
    LOG_INFO("Main") << "Demo completed!";
    core::Logger::shutdown();
    
    return 0;
}
