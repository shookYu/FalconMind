/**
 * @file test_hot_reload.cpp
 * @brief 插件热更新工程测试用例
 * 
 * 这是一个完整的工程级测试，演示：
 * 1. 加载检测器插件
 * 2. 后台持续进行检测
 * 3. 检测到插件文件变更时自动重新加载
 * 4. 无缝切换到新版本，不中断任务
 * 
 * 使用方法：
 * 1. 编译并运行此测试
 * 2. 在另一个终端编译新版本的插件
 * 3. 覆盖plugins目录下的.so文件
 * 4. 观察测试输出，看到热更新过程
 */

#include "falconmind/sdk/plugin/CapabilityRegistry.h"
#include "falconmind/sdk/plugin/BuiltinPlugins.h"
#include "falconmind/sdk/core/Logger.h"
#include "falconmind/sdk/core/ConfigManager.h"
#include <iostream>
#include <syncstream>
#include <atomic>
#include <thread>
#include <chrono>
#include <csignal>

using namespace falconmind::sdk;

// 全局标志
std::atomic<bool> g_running{true};
std::atomic<int> g_detectionCount{0};
std::atomic<int> g_pluginReloadCount{0};
std::string g_currentPluginVersion{"unknown"};

// 信号处理
void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        LOG_INFO("Test") << "Received shutdown signal";
        g_running = false;
    }
}

// ========================================================================
// 1. 模拟检测线程（持续运行）
// ========================================================================
void detectionWorker() {
    LOG_INFO("Worker") << "Detection worker started";
    
    while (g_running) {
        // 获取当前检测器
        auto detector = plugin::registry().getDefaultDetector();
        
        if (!detector) {
            LOG_WARN("Worker") << "No detector available, waiting...";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            continue;
        }
        
        // 获取插件元数据
        auto metadata = detector->getMetadata();
        
        // 检测版本变化
        if (g_currentPluginVersion != metadata.version) {
            LOG_INFO("Worker") << "=== Using NEW plugin version: " 
                      << metadata.name << " v" << metadata.version << " ===";
            g_currentPluginVersion = metadata.version;
        }
        
        // 模拟检测任务
        // 在实际场景中这里会读取相机图像并进行推理
        perception::ImageView dummyImage{};
        dummyImage.width = 640;
        dummyImage.height = 480;
        dummyImage.channels = 3;
        
        // auto result = detector->detect(dummyImage);
        
        g_detectionCount++;
        
        // 每100次检测输出一次状态
        if (g_detectionCount % 100 == 0) {
            LOG_INFO("Worker") << "Detection count: " << g_detectionCount.load()
                      << " (Plugin: " << metadata.name << " v" << metadata.version << ")";
        }
        
        // 模拟30fps检测
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    LOG_INFO("Worker") << "Detection worker stopped";
}

// ========================================================================
// 2. 监控和统计线程
// ========================================================================
void monitorWorker() {
    LOG_INFO("Monitor") << "Monitor worker started";
    
    int lastDetectionCount = 0;
    int lastReloadCount = 0;
    
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        if (!g_running) break;
        
        int currentDetections = g_detectionCount.load();
        int currentReloads = g_pluginReloadCount.load();
        
        int detectionsInPeriod = currentDetections - lastDetectionCount;
        
        LOG_INFO("Monitor") << "=== Status Report ===";
        LOG_INFO("Monitor") << "  Total detections: " << currentDetections
                  << " (+" << detectionsInPeriod << " in last 5s)";
        LOG_INFO("Monitor") << "  Plugin reloads: " << currentReloads;
        LOG_INFO("Monitor") << "  Current version: " << g_currentPluginVersion;
        
        // 列出所有可用的检测器
        auto detectors = plugin::registry().listDetectors();
        LOG_INFO("Monitor") << "  Available detectors (" << detectors.size() << "):";
        for (const auto& info : detectors) {
            LOG_INFO("Monitor") << "    - " << info.name 
                      << " v" << info.version
                      << (info.isBuiltin ? " [builtin]" : " [plugin]");
        }
        
        lastDetectionCount = currentDetections;
        lastReloadCount = currentReloads;
    }
    
    LOG_INFO("Monitor") << "Monitor worker stopped";
}

// ========================================================================
// 3. 热更新回调
// ========================================================================
void onPluginReloaded(const std::string& name, 
                      const std::string& oldVersion,
                      const std::string& newVersion) {
    g_pluginReloadCount++;
    
    LOG_INFO("HotReload") << "╔════════════════════════════════════════════════╗";
    LOG_INFO("HotReload") << "║       PLUGIN HOT RELOAD DETECTED              ║";
    LOG_INFO("HotReload") << "╚════════════════════════════════════════════════╝";
    LOG_INFO("HotReload") << "  Plugin: " << name;
    LOG_INFO("HotReload") << "  Old version: " << oldVersion;
    LOG_INFO("HotReload") << "  New version: " << newVersion;
    LOG_INFO("HotReload") << "  Detection count at reload: " << g_detectionCount.load();
    LOG_INFO("HotReload") << "  Note: Detection thread continues without interruption";
}

// ========================================================================
// 主函数
// ========================================================================
int main(int argc, char* argv[]) {
    // 设置信号处理
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    std::cout << R"(
╔═══════════════════════════════════════════════════════════════╗
║          FalconMindSDK Plugin Hot Reload Test                 ║
╚═══════════════════════════════════════════════════════════════╝

This test demonstrates zero-downtime plugin hot reloading:

1. A detection worker thread runs continuously (30fps)
2. A monitor thread reports status every 5 seconds
3. When you replace the plugin .so file, it auto-reloads
4. Detection continues seamlessly during reload

Usage:
  1. Run this test: ./test_hot_reload
  2. In another terminal, rebuild the plugin with a new version
  3. Copy the new .so to plugins/ directory
  4. Watch the hot reload happen automatically

Press Ctrl+C to stop.

)";
    
    // 初始化日志
    core::Logger::init({
        .level = core::LogLevel::Info,
        .outputFile = "hot_reload_test.log",
        .enableConsole = true
    });
    
    LOG_INFO("Main") << "Hot reload test starting...";
    
    try {
        // 1. 注册内建插件
        LOG_INFO("Main") << "Registering builtin plugins...";
        plugin::registerBuiltinPlugins();
        
        // 2. 初始化能力注册中心（启用热更新）
        LOG_INFO("Main") << "Initializing capability registry with hot reload...";
        plugin::registry().initialize("./plugins");
        plugin::registry().enableHotReload(true);
        
        // 3. 尝试加载插件
        LOG_INFO("Main") << "Loading initial plugin...";
        
        // 先尝试加载内建插件作为默认值
        auto detectors = plugin::registry().listDetectors();
        if (!detectors.empty()) {
            LOG_INFO("Main") << "Using builtin detector: " << detectors[0].name;
            plugin::registry().setDefaultDetector(detectors[0].name);
            g_currentPluginVersion = detectors[0].version;
        }
        
        // 尝试从插件目录加载外部插件
        auto pluginsDir = std::filesystem::path("./plugins");
        if (std::filesystem::exists(pluginsDir)) {
            LOG_INFO("Main") << "Scanning plugins directory...";
            auto loaded = plugin::registry().loadAllCapabilitiesFromDirectory(pluginsDir);
            
            if (!loaded.empty()) {
                LOG_INFO("Main") << "Loaded plugins: " << loaded.size();
                for (const auto& name : loaded) {
                    LOG_INFO("Main") << "  - " << name;
                }
                
                // 如果有外部插件，设为默认
                for (const auto& name : loaded) {
                    auto type = plugin::registry().getCapabilityType(name);
                    if (type == plugin::PluginType::Detector) {
                        LOG_INFO("Main") << "Setting default detector to: " << name;
                        plugin::registry().setDefaultDetector(name);
                        
                        auto detector = plugin::registry().getDefaultDetector();
                        if (detector) {
                            g_currentPluginVersion = detector->getMetadata().version;
                        }
                        break;
                    }
                }
            } else {
                LOG_WARN("Main") << "No plugins found in directory, using builtin";
            }
        } else {
            LOG_WARN("Main") << "Plugins directory not found, using builtin";
        }
        
        // 4. 注册能力变更回调
        std::string lastVersion = g_currentPluginVersion;
        plugin::registry().onCapabilityChange(
            [&lastVersion
](const std::string& name, plugin::PluginType type, bool added) {
                if (added && type == plugin::PluginType::Detector) {
                    auto detector = plugin::registry().getDefaultDetector();
                    if (detector) {
                        std::string newVersion = detector->getMetadata().version;
                        if (newVersion != lastVersion) {
                            onPluginReloaded(name, lastVersion, newVersion);
                            lastVersion = newVersion;
                        }
                    }
                }
            }
        );
        
        // 5. 启动工作线程
        LOG_INFO("Main") << "Starting worker threads...";
        
        std::thread detectionThread(detectionWorker);
        std::thread monitorThread(monitorWorker);
        
        // 6. 主循环（处理命令）
        LOG_INFO("Main") << "Test running. Press Ctrl+C to stop.";
        LOG_INFO("Main") << "";
        LOG_INFO("Main") << "To test hot reload:";
        LOG_INFO("Main") << "  1. Edit your plugin source code";
        LOG_INFO("Main") << "  2. Increment version number";
        LOG_INFO("Main") << "  3. Rebuild: g++ -shared -fPIC -o libdetector.so ...";
        LOG_INFO("Main") << "  4. Copy: cp libdetector.so ./plugins/";
        LOG_INFO("Main") << "  5. Watch auto-reload happen!";
        LOG_INFO("Main") << "";
        
        // 等待信号
        while (g_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // 7. 优雅关闭
        LOG_INFO("Main") << "Shutting down...";
        
        // 等待线程结束
        if (detectionThread.joinable()) {
            detectionThread.join();
        }
        if (monitorThread.joinable()) {
            monitorThread.join();
        }
        
        // 统计
        LOG_INFO("Main") << "=== Final Statistics ===";
        LOG_INFO("Main") << "  Total detections: " << g_detectionCount.load();
        LOG_INFO("Main") << "  Plugin reloads: " << g_pluginReloadCount.load();
        LOG_INFO("Main") << "  Final version: " << g_currentPluginVersion;
        
        // 清理
        plugin::registry().shutdown();
        plugin::unregisterBuiltinPlugins();
        core::Logger::shutdown();
        
        LOG_INFO("Main") << "Test completed successfully!";
        
        return 0;
        
    } catch (const std::exception& e) {
        LOG_ERROR("Main") << "Fatal error: " << e.what();
        return 1;
    }
}
