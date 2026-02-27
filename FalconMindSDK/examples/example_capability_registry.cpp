/**
 * @file example_capability_registry.cpp
 * @brief 能力注册中心完整使用示例
 * 
 * 展示如何通过插件系统动态扩展所有核心能力
 */

#include "falconmind/sdk/plugin/CapabilityRegistry.h"
#include "falconmind/sdk/plugin/BuiltinPlugins.h"
#include "falconmind/sdk/core/Logger.h"
#include <iostream>

using namespace falconmind::sdk;

// ========================================================================
// 1. 使用默认能力（内建插件）
// ========================================================================
void example_use_default_capabilities() {
    LOG_INFO("Example") << "=== Using Default Capabilities ===";
    
    // 初始化注册中心（自动加载内建插件）
    plugin::registerBuiltinPlugins();
    
    // 获取默认检测器
    auto detector = plugin::registry().getDefaultDetector();
    if (detector) {
        LOG_INFO("Example") << "Default detector: " << detector->getMetadata().name;
        
        // 加载模型并使用
        detector->loadModel("models/yolov8n.onnx");
        // auto result = detector->detect(image);
    }
    
    // 获取默认跟踪器
    auto tracker = plugin::registry().getDefaultTracker();
    if (tracker) {
        LOG_INFO("Example") << "Default tracker: " << tracker->getMetadata().name;
    }
    
    // 获取默认导航
    auto navigation = plugin::registry().getDefaultNavigation();
    if (navigation) {
        LOG_INFO("Example") << "Default navigation: " << navigation->getMetadata().name;
    }
    
    // 获取抗欺骗导航（如果可用）
    auto antiSpoof = plugin::registry().getAntiSpoofNavigation();
    if (antiSpoof) {
        LOG_INFO("Example") << "Anti-spoof navigation available";
    }
    
    // 获取拒止环境导航（如果可用）
    auto gnssDenied = plugin::registry().getGNSSDeniedNavigation();
    if (gnssDenied) {
        LOG_INFO("Example") << "GNSS-denied navigation available";
    }
    
    // 获取默认任务规划器
    auto planner = plugin::registry().getDefaultMissionPlanner();
    if (planner) {
        LOG_INFO("Example") << "Default mission planner: " << planner->getMetadata().name;
    }
}

// ========================================================================
// 2. 动态加载插件扩展能力
// ========================================================================
void example_load_plugin_capabilities() {
    LOG_INFO("Example") << "=== Loading Plugin Capabilities ===";
    
    auto& registry = plugin::registry();
    
    // 初始化插件系统
    registry.initialize("./plugins");
    
    // 加载检测器插件
    auto result = registry.loadCapabilityFromPlugin("./plugins/libyolo26_detector.so");
    if (result) {
        LOG_INFO("Example") << "Loaded YOLO26 detector plugin";
        
        // 使用新加载的检测器
        core::ConfigManager config;
        config.set<std::string>("model_path", "models/yolov8n.onnx");
        config.set<float>("confidence_threshold", 0.7f);
        
        auto yolo26 = registry.createDetector("yolo26_detector", config);
        if (yolo26) {
            LOG_INFO("Example") << "Created YOLO26 detector instance";
            yolo26->loadModel("models/yolov8n.onnx");
        }
    } else {
        LOG_WARN("Example") << "Failed to load YOLO26: " << result.error().message();
    }
    
    // 加载自定义跟踪器插件
    result = registry.loadCapabilityFromPlugin("./plugins/libdeepsort_tracker.so");
    if (result) {
        LOG_INFO("Example") << "Loaded DeepSORT tracker plugin";
        
        auto tracker = registry.createTracker("deepsort_tracker");
        if (tracker) {
            tracker->init(100);  // 最大跟踪100个目标
        }
    }
    
    // 加载拒止导航插件
    result = registry.loadCapabilityFromPlugin("./plugins/libvisual_inertial_nav.so");
    if (result) {
        LOG_INFO("Example") << "Loaded Visual-Inertial Navigation plugin";
    }
    
    // 扫描并加载所有插件
    LOG_INFO("Example") << "Scanning plugins directory...";
    auto loaded = registry.loadAllCapabilitiesFromDirectory("./plugins");
    LOG_INFO("Example") << "Loaded " << loaded.size() << " capabilities from plugins";
    for (const auto& name : loaded) {
        LOG_INFO("Example") << "  - " << name;
    }
}

// ========================================================================
// 3. 切换默认能力实现
// ========================================================================
void example_switch_default_capability() {
    LOG_INFO("Example") << "=== Switching Default Capability ===";
    
    auto& registry = plugin::registry();
    
    // 列出所有可用的检测器
    LOG_INFO("Example") << "Available detectors:";
    auto detectors = registry.listDetectors();
    for (const auto& info : detectors) {
        LOG_INFO("Example") << "  - " << info.name 
                  << " v" << info.version
                  << (info.isBuiltin ? " [builtin]" : " [plugin]");
    }
    
    // 切换到YOLO26作为默认检测器
    if (registry.hasCapability("yolo26_detector")) {
        registry.setDefaultDetector("yolo26_detector");
        LOG_INFO("Example") << "Default detector switched to yolo26_detector";
    }
    
    // 切换跟踪器
    if (registry.hasCapability("deepsort_tracker")) {
        registry.setDefaultTracker("deepsort_tracker");
        LOG_INFO("Example") << "Default tracker switched to deepsort_tracker";
    }
    
    // 现在获取默认会使用新的实现
    auto detector = registry.getDefaultDetector();
    if (detector) {
        LOG_INFO("Example") << "Current default detector: " 
                  << detector->getMetadata().name;
    }
}

// ========================================================================
// 4. 运行时替换能力（热更新）
// ========================================================================
void example_hot_reload() {
    LOG_INFO("Example") << "=== Hot Reload Capability ===";
    
    auto& registry = plugin::registry();
    
    // 启用热更新
    registry.enableHotReload(true);
    LOG_INFO("Example") << "Hot reload enabled";
    
    // 注册能力变更回调
    registry.onCapabilityChange([](const std::string& name, 
                                   plugin::PluginType type,
                                   bool added) {
        if (added) {
            LOG_INFO("HotReload") << "Capability added: " << name;
        } else {
            LOG_INFO("HotReload") << "Capability removed: " << name;
        }
    });
    
    // 手动检查更新
    LOG_INFO("Example") << "Checking for updates...";
    // 这会检查插件文件是否被修改并重新加载
    
    // 在实际应用中，热更新会在后台自动进行
    // 当.so文件被替换时，新版本的插件会自动加载
}

// ========================================================================
// 5. 查询所有可用能力
// ========================================================================
void example_query_capabilities() {
    LOG_INFO("Example") << "=== Querying All Capabilities ===";
    
    auto& registry = plugin::registry();
    
    // 列出所有能力
    LOG_INFO("Example") << "All registered capabilities:";
    auto all = registry.listAllCapabilities();
    for (const auto& info : all) {
        LOG_INFO("Example") << "  [" << static_cast<int>(info.type) << "] " 
                  << info.name 
                  << " v" << info.version
                  << " - " << info.description;
    }
    
    // 按类型查询
    LOG_INFO("Example") << "\nNavigation capabilities:";
    auto navs = registry.listNavigation();
    for (const auto& info : navs) {
        LOG_INFO("Example") << "  - " << info.name;
    }
    
    LOG_INFO("Example") << "\nMission planners:";
    auto planners = registry.listMissionPlanners();
    for (const auto& info : planners) {
        LOG_INFO("Example") << "  - " << info.name;
    }
    
    // 检查特定能力是否存在
    if (registry.hasCapability("yolo26_detector")) {
        LOG_INFO("Example") << "\nYOLO26 detector is available";
        
        auto type = registry.getCapabilityType("yolo26_detector");
        if (type) {
            LOG_INFO("Example") << "  Type: " << static_cast<int>(*type);
        }
    }
}

// ========================================================================
// 6. 卸载不需要的能力
// ========================================================================
void example_unload_capability() {
    LOG_INFO("Example") << "=== Unloading Capability ===";
    
    auto& registry = plugin::registry();
    
    // 卸载特定能力
    if (registry.hasCapability("old_detector")) {
        auto result = registry.unloadCapability("old_detector");
        if (result) {
            LOG_INFO("Example") << "Unloaded old_detector";
        } else {
            LOG_WARN("Example") << "Failed to unload: " << result.error().message();
        }
    }
}

// ========================================================================
// 主函数
// ========================================================================
int main() {
    // 初始化日志
    core::Logger::init({
        .level = core::LogLevel::Info,
        .enableConsole = true
    });
    
    LOG_INFO("Main") << "=== FalconMindSDK Capability Registry Demo ===";
    
    try {
        // 1. 使用内建默认能力
        example_use_default_capabilities();
        
        // 2. 从插件加载新能力
        example_load_plugin_capabilities();
        
        // 3. 切换默认实现
        example_switch_default_capability();
        
        // 4. 热更新
        example_hot_reload();
        
        // 5. 查询能力
        example_query_capabilities();
        
        // 6. 卸载能力
        example_unload_capability();
        
        LOG_INFO("Main") << "\n=== Demo Completed Successfully ===";
        
    } catch (const std::exception& e) {
        LOG_ERROR("Main") << "Error: " << e.what();
        return 1;
    }
    
    // 清理
    plugin::unregisterBuiltinPlugins();
    core::Logger::shutdown();
    
    return 0;
}
