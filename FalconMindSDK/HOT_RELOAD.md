# FalconMindSDK 插件热更新技术文档

## 概述

FalconMindSDK 支持**零停机插件热更新**，允许在系统运行过程中动态替换插件实现（如检测模型、跟踪算法、导航策略等），无需重启服务或中断正在执行的任务。

## 核心特性

### 1. 无缝切换
- 检测线程持续运行，不中断
- 新版本插件加载完成后立即生效
- 旧版本插件安全卸载，无内存泄漏

### 2. 自动检测
- 文件系统监控，自动发现插件变更
- 可配置检查间隔（默认5秒）
- 支持手动触发更新检查

### 3. 版本管理
- 跟踪插件版本信息
- 记录更新历史
- 支持回滚（重新加载旧版本）

### 4. 安全可靠
- 原子性替换，不会同时存在两个版本
- 加载失败时保持当前版本运行
- 线程安全的实现

## 工作原理

```
┌─────────────────────────────────────────────────────────────┐
│                    热更新工作流程                            │
└─────────────────────────────────────────────────────────────┘

    检测线程                     热更新线程                    文件系统
       │                            │                           │
       │  持续检测                    │  监控文件变化              │
       │◄────────────────────────────►│◄─────────────────────────►│
       │                            │                           │
       │                            │  发现新版本.so             │
       │                            │◄──────────────────────────┤
       │                            │                           │
       │                            │  1. 加载新插件             │
       │                            │  2. 初始化新实例           │
       │                            │  3. 验证新插件             │
       │                            │                           │
       │  切换到新版本              │                           │
       │◄───────────────────────────┤                           │
       │                            │                           │
       │  继续使用新版本            │  4. 卸载旧插件             │
       │                            │                           │
```

## 技术实现

### 1. Linux动态库机制

使用 `dlopen`/`dlsym` 实现真正的动态加载：

```cpp
// 加载共享库
void* handle = dlopen("./plugins/libdetector.so", RTLD_NOW | RTLD_LOCAL);

// 获取导出函数
CreatePluginFunc createFunc = dlsym(handle, "createPlugin");

// 创建插件实例
IPlugin* plugin = createFunc();

// 使用完成后卸载
dlclose(handle);
```

### 2. 引用计数管理

```cpp
class LibraryHandle {
public:
    ~LibraryHandle() {
        if (handle_) {
            dlclose(handle_);  // 自动释放
        }
    }
    // 禁止拷贝，允许移动
};
```

### 3. 双缓冲机制

```cpp
// 加载新插件（不影响当前运行的）
auto newPlugin = loadNewPlugin();
if (newPlugin->initialize(config)) {
    // 原子性切换
    auto oldPlugin = currentPlugin_;
    currentPlugin_ = newPlugin;
    
    // 延迟卸载旧插件
    scheduleUnload(oldPlugin);
}
```

## 使用方法

### 1. 启用热更新

```cpp
#include "falconmind/sdk/plugin/CapabilityRegistry.h"

// 初始化并启用热更新
plugin::registry().initialize("./plugins");
plugin::registry().enableHotReload(true);
```

### 2. 监听更新事件

```cpp
// 注册更新回调
plugin::registry().onCapabilityChange(
    [](const std::string& name, plugin::PluginType type, bool added) {
        if (added) {
            std::cout << "Plugin updated: " << name << std::endl;
        }
    }
);
```

### 3. 执行热更新

**方式一：自动检测**
```bash
# 1. 编译新版本插件
g++ -shared -fPIC -o libdetector_v2.so detector_v2.cpp

# 2. 替换文件（自动触发更新）
cp libdetector_v2.so ./plugins/libdetector.so
```

**方式二：手动触发**
```cpp
// 检查更新
auto updates = registry.checkForUpdates();
```

## 完整示例

### 示例：YOLO检测器热更新

#### 1. 初始版本 (v1.0.0)

```cpp
// yolo_detector_v1.cpp
#include "falconmind/sdk/plugin/IPlugin.h"

class YoloDetectorV1 : public IDetectorPlugin {
public:
    PluginMetadata getMetadata() const override {
        return {
            .name = "yolo_detector",
            .version = "1.0.0",
            .type = PluginType::Detector,
            .capabilities = PluginCapability::RealTime
        };
    }
    
    DetectionResult detect(const ImageView& image) override {
        // v1.0.0 检测逻辑
        // 支持80类目标
        return detectWithYoloV5(image);
    }
};

EXPORT_PLUGIN(YoloDetectorV1)
```

编译并部署：
```bash
g++ -std=c++17 -shared -fPIC -o libyolo_detector.so yolo_detector_v1.cpp
cp libyolo_detector.so ./plugins/
```

#### 2. 更新版本 (v2.0.0)

```cpp
// yolo_detector_v2.cpp
#include "falconmind/sdk/plugin/IPlugin.h"

class YoloDetectorV2 : public IDetectorPlugin {
public:
    PluginMetadata getMetadata() const override {
        return {
            .name = "yolo_detector",
            .version = "2.0.0",  // 版本升级
            .type = PluginType::Detector,
            .capabilities = PluginCapability::RealTime | 
                          PluginCapability::GPUAccelerated
        };
    }
    
    DetectionResult detect(const ImageView& image) override {
        // v2.0.0 改进的检测逻辑
        // 支持120类目标，精度提升15%
        // 增加GPU加速
        return detectWithYoloV8(image);
    }
};

EXPORT_PLUGIN(YoloDetectorV2)
```

执行热更新：
```bash
# 编译新版本
g++ -std=c++17 -shared -fPIC -o libyolo_detector_v2.so yolo_detector_v2.cpp

# 替换文件（触发自动热更新）
cp libyolo_detector_v2.so ./plugins/libyolo_detector.so

# 观察日志输出：
# [INFO] Plugin file changed: libyolo_detector.so
# [INFO] Loading new version: 2.0.0
# [INFO] Hot reload complete: v1.0.0 -> v2.0.0
# [INFO] Detection continued seamlessly
```

### 示例：任务规划策略热切换

```cpp
// 飞行中切换导航策略
void onGNSSSignalLost() {
    LOG_WARN("Navigation") << "GPS signal lost, switching to visual navigation";
    
    // 切换到拒止导航插件
    auto nav = registry.getGNSSDeniedNavigation();
    nav->initializePosition(currentPosition);
    
    // 继续飞行，无需返航
}

void onGNSSSignalRestored() {
    LOG_INFO("Navigation") << "GPS signal restored, switching back to GNSS";
    
    // 切换回GNSS导航
    auto nav = registry.getDefaultNavigation();
}
```

## 测试验证

### 1. 运行热更新测试

```bash
cd FalconMindSDK/tests
./test_hot_reload_demo.sh
```

预期输出：
```
[INFO] Test started (PID: 12345)
[INFO] Waiting 10 seconds for v1.0.0 to run...
[INFO] ==========================================
[INFO] PERFORMING HOT UPDATE: v1.0.0 -> v2.0.0
[INFO] ==========================================
[SUCCESS] Plugin file replaced with v2.0.0
[INFO] Hot reload should trigger automatically...
[INFO] ╔════════════════════════════════════════════════╗
[INFO] ║       PLUGIN HOT RELOAD DETECTED              ║
[INFO] ╚════════════════════════════════════════════════╝
[INFO]   Plugin: demo_detector
[INFO]   Old version: 1.0.0
[INFO]   New version: 2.0.0
[INFO]   Note: Detection thread continues without interruption
```

### 2. 性能影响测试

```cpp
// 测量热更新期间的性能
void measureHotReloadImpact() {
    auto start = std::chrono::high_resolution_clock::now();
    
    // 执行热更新
    registry.reloadPlugin("detector");
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 热更新应该 < 100ms
    assert(duration.count() < 100);
}
```

## 最佳实践

### 1. 版本管理

- 使用语义化版本号（Semantic Versioning）
- 在元数据中清晰标注兼容性
- 维护更新日志

```cpp
PluginMetadata getMetadata() const override {
    return {
        .name = "my_detector",
        .version = "2.1.3",
        .description = "Bug fix release - fixed memory leak in batch processing",
        .sdkVersion = "1.0.0",  // 兼容的SDK版本
    };
}
```

### 2. 状态迁移

- 保存必要的状态信息
- 设计可恢复的检查点

```cpp
bool reloadConfig(const ConfigManager& config) override {
    // 保存当前状态
    auto checkpoint = saveCheckpoint();
    
    // 应用新配置
    if (!applyNewConfig(config)) {
        // 失败时恢复
        restoreCheckpoint(checkpoint);
        return false;
    }
    
    return true;
}
```

### 3. 错误处理

```cpp
void onHotReloadError(const Error& error) {
    LOG_ERROR("HotReload") << "Failed to reload: " << error.message();
    
    // 保持当前版本继续运行
    // 发送告警通知
    sendAlert("Plugin hot reload failed", error);
}
```

## 限制和注意事项

### 1. 接口兼容性
- 新版本必须实现相同的接口
- 不能更改虚函数表布局
- 数据类型必须一致

### 2. 资源管理
- 确保旧插件的资源完全释放
- 避免内存泄漏
- 文件句柄必须关闭

### 3. 线程安全
- 热更新期间避免在插件中创建新线程
- 使用原子操作共享状态
- 正确同步资源访问

## 故障排除

### 问题：热更新失败

**症状**：替换.so文件后没有触发更新

**检查**：
1. 热更新是否启用：`registry.enableHotReload(true)`
2. 文件权限是否正确
3. 路径配置是否正确

**解决**：
```cpp
// 手动检查更新
auto updates = registry.checkForUpdates();
for (const auto& result : updates) {
    if (result.success) {
        LOG_INFO("Updated: " << result.name);
    } else {
        LOG_ERROR("Failed: " << result.errorMessage);
    }
}
```

### 问题：符号冲突

**症状**：`dlsym`返回NULL

**原因**：插件未正确导出符号

**解决**：
```cpp
// 确保使用extern "C"和可见性属性
extern "C" {
    __attribute__((visibility("default")))
    IPlugin* createPlugin() {
        return new MyPlugin();
    }
}
```

## 架构对比

| 特性 | 传统方式 | FalconMind热更新 |
|------|---------|-----------------|
| 停机时间 | 需要重启服务 | 零停机 |
| 任务中断 | 是 | 否 |
| 回滚能力 | 困难 | 简单（重新加载旧版本） |
| 版本并行 | 不支持 | 支持（快速切换） |
| 开发效率 | 低（需要重启测试） | 高（实时看到效果） |

## 总结

FalconMindSDK的热更新功能提供了生产级的动态扩展能力：

✅ **零停机更新**：服务持续运行，用户无感知  
✅ **安全可靠**：失败时自动回退，不影响当前运行  
✅ **简单易用**：替换文件即可，无需复杂配置  
✅ **全能力覆盖**：检测、跟踪、导航、规划全部支持  

这使得FalconMindSDK可以灵活适应不同的应用场景和算法迭代需求。
