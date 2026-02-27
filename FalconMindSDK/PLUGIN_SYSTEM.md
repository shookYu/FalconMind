# FalconMindSDK 插件系统架构

## 概述

FalconMindSDK 采用全插件化架构，所有核心能力（目标检测、跟踪、视觉制导、导航、任务规划）都支持动态扩展和更新，无需修改SDK代码。

## 核心组件

### 1. 插件接口 (IPlugin.h)

定义所有可插拔能力的标准接口：

```cpp
IDetectorPlugin       - 目标检测器接口
ITrackerPlugin        - 目标跟踪器接口
IVisualGuidancePlugin - 视觉制导接口
INavigationPlugin     - 导航接口（含拒止导航、防GPS欺骗）
IMissionPlannerPlugin - 任务规划器接口
```

### 2. 能力注册中心 (CapabilityRegistry)

统一管理所有能力的注册、创建和获取：

```cpp
// 注册能力
registry.registerDetector("yolo26", factory);

// 获取能力实例
auto detector = registry.createDetector("yolo26");

// 设置默认实现
registry.setDefaultDetector("yolo26");

// 获取默认实现
auto detector = registry.getDefaultDetector();
```

### 3. 插件管理器 (PluginManager)

Linux真实动态加载实现：

```cpp
// 从.so文件加载
pluginManager.loadPlugin("./plugins/libyolo26.so");

// 热更新
pluginManager.enableHotReload(true);
```

## 使用模式

### 模式1：动态加载新能力

```cpp
// 1. 编译插件
g++ -std=c++17 -shared -fPIC -o libyolo26.so yolo26_plugin.cpp

// 2. 放入plugins目录

// 3. SDK自动加载
registry.loadCapabilityFromPlugin("./plugins/libyolo26.so");

// 4. 使用新能力
auto detector = registry.createDetector("yolo26_detector");
```

### 模式2：运行时替换默认实现

```cpp
// 切换检测器
registry.setDefaultDetector("yolo26_detector");

// 切换跟踪器
registry.setDefaultTracker("deepsort_tracker");

// 切换导航（抗欺骗）
registry.setDefaultNavigation("visual_inertial_nav");
```

### 模式3：热更新（无需重启）

```cpp
// 启用热更新
registry.enableHotReload(true);

// 替换.so文件后自动重新加载
```

## 核心能力扩展示例

### 目标检测

```cpp
// 当前：YOLOv8
auto detector = registry.getDefaultDetector();

// 新增：YOLOv26（不修改SDK）
// 1. 开发 yolo26_plugin.cpp
// 2. 编译成 libyolo26.so
// 3. 放入 plugins/
// 4. SDK自动识别并可用
```

### 拒止导航

```cpp
// GPS正常时
auto nav = registry.getDefaultNavigation();

// GPS拒止环境（自动或手动切换）
auto nav = registry.getGNSSDeniedNavigation();
// 或
auto nav = registry.createNavigation("visual_inertial_nav");
```

### 防GPS欺骗

```cpp
// 使用抗欺骗导航
auto nav = registry.getAntiSpoofNavigation();

// 检查是否被欺骗
if (nav->isGNSSSpoofed()) {
    // 切换到备用导航
    nav = registry.getGNSSDeniedNavigation();
}
```

### 任务规划

```cpp
// 标准规划器
auto planner = registry.getDefaultMissionPlanner();

// 自定义规划器（如：电力巡检专用）
auto planner = registry.createMissionPlanner("powerline_inspection_planner");
```

## 插件开发流程

### 1. 实现接口

```cpp
#include "falconmind/sdk/plugin/IPlugin.h"

class MyDetector : public IDetectorPlugin {
public:
    PluginMetadata getMetadata() const override {
        return {
            .name = "my_detector",
            .version = "1.0.0",
            .type = PluginType::Detector,
            .capabilities = PluginCapability::RealTime | PluginCapability::GPUAccelerated
        };
    }
    
    bool initialize(const ConfigManager& config) override {
        // 初始化
        return true;
    }
    
    DetectionResult detect(const ImageView& image) override {
        // 实现检测逻辑
        return result;
    }
    
    // ... 其他接口方法
};

EXPORT_PLUGIN(MyDetector)
```

### 2. 编译插件

```bash
g++ -std=c++17 -shared -fPIC \
    -o libmy_detector.so \
    my_detector.cpp \
    -I/path/to/FalconMindSDK/include \
    -lonnxruntime -lopencv_core
```

### 3. 部署使用

```bash
# 放入插件目录
cp libmy_detector.so ./plugins/

# SDK自动加载并使用
```

## 架构优势

1. **零代码修改扩展**：新增YOLO26、新跟踪算法等无需改动SDK
2. **运行时切换**：根据场景动态选择最佳实现
3. **热更新**：不停机更新算法模型
4. **第三方扩展**：合作伙伴可开发私有插件
5. **版本隔离**：不同任务可使用不同版本实现
6. **Linux原生**：使用dlopen/dlsym，无模拟层

## 文件结构

```
FalconMindSDK/
├── include/falconmind/sdk/plugin/
│   ├── IPlugin.h              # 插件接口定义
│   ├── PluginManager.h        # Linux动态加载
│   ├── CapabilityRegistry.h   # 能力注册中心
│   └── BuiltinPlugins.h       # 内建插件
├── src/plugin/
│   ├── PluginManager.cpp      # 插件管理实现
│   └── CapabilityRegistry.cpp # 注册中心实现
└── plugins/                   # 插件目录
    ├── libyolo26_detector.so
    ├── libdeepsort_tracker.so
    └── libvisual_inertial_nav.so
```

## 下一步

1. 实现内建插件（BuiltinPlugins.cpp）
2. 重构现有模块使用CapabilityRegistry
3. 创建更多插件开发示例
4. 添加插件签名验证（安全）
5. 实现插件依赖管理
