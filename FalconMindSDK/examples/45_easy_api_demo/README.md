# FalconMindSDK 示例45：Easy API 高级封装演示

## 概述

本示例演示如何使用 **FalconMind::Easy API**（即 `falconmind::sdk::high_level` 命名空间）
大幅简化无人机感知流水线的创建和使用。

## 核心改进

### 1. 代码量对比

**传统 API（示例01）**：
```cpp
// 需要 ~50 行代码
auto pipeline = std::make_shared<Pipeline>(config);
auto source = std::make_shared<SourceNode>("source");
auto detector = std::make_shared<RknnDetectorBackend>();
detector->configure({{"model", "yolov8.rknn"}});
pipeline->addNode(source);
pipeline->addNode(detector);
pipeline->link("source", "out", "detector", "in");
bool success = pipeline->setState(PipelineState::Running);
if (!success) {
    // 不知道具体错误原因...
}
```

**新的 Easy API（本示例）**：
```cpp
// 仅需 ~10 行代码
auto result = PerceptionPipeline::create()
    .withCamera(640, 480, 30)
    .withDetector("yolov8n.onnx", DetectorBackend::ONNX_RUNTIME)
    .withTracker(TrackerType::DEEPSORT)
    .build();

if (result.isError()) {
    std::cerr << "错误: " << result.errorMessage() << std::endl;
    return;
}

auto pipeline = result.value();
pipeline->onDetection([](const auto& dets) {
    for (const auto& d : dets) {
        std::cout << d.className << std::endl;
    }
});
pipeline->start();
```

### 2. 主要特性

| 特性 | 说明 |
|------|------|
| **Builder 模式** | 流式 API，链式配置 |
| **Result<T>** | 强类型错误处理，替代 bool 返回值 |
| **自动资源管理** | RAII 模式，无需手动清理 |
| **便捷函数** | `createMinimalPipeline()` 一行代码启动 |

## 演示内容

### 示例1：Builder 模式创建流水线
展示如何使用流式 API 配置完整的感知流水线。

### 示例2：便捷函数快速创建
使用 `createPerceptionPipeline()` 便捷函数快速启动。

### 示例3：极简模式
使用 `createMinimalPipeline()` 一行代码启动，自动选择最佳后端。

### 示例4：特定对象检测
演示如何设置回调只关注特定类别（如 person、car）。

### 示例5：错误处理演示
展示 `Result<T>` 的各种错误处理方式。

## 编译运行

### x86 平台
```bash
cd FalconMindSDK/examples/45_easy_api_demo/x86
mkdir -p build && cd build
cmake ..
make -j4
./45_easy_api_demo_x86
```

## 关键 API 说明

### PerceptionPipelineBuilder

```cpp
auto builder = PerceptionPipeline::create();
builder.withCamera(width, height, fps)                    // 配置相机
       .withCameraDevice(device)                          // 设备路径
       .withDetector(modelPath, backend)                  // 检测器
       .withTracker(type)                                 // 跟踪器
       .withLowLightEnhancement(enable)                   // 低光增强
       .build();                                          // 构建
```

### Result<T> 错误处理

```cpp
// 方式1: 显式检查
if (result.isSuccess()) {
    auto value = result.value();
} else {
    std::cerr << result.errorMessage();
}

// 方式2: bool 转换
if (result) {
    auto value = result.value();
}

// 方式3: 安全获取（带默认值）
auto value = result.valueOr(defaultValue);

// 方式4: 函数式处理
result.map([](auto value) { 
    return process(value); 
});
```

### 回调设置

```cpp
// 批量检测回调
pipeline->onDetection([](const std::vector<Detection>& detections) {
    // 处理所有检测结果
});

// 单个检测回调
pipeline->onDetection([](const Detection& detection) {
    // 逐个处理检测结果
});

// 特定类别回调
pipeline->onObjectDetected("person", [](const Detection& det) {
    // 只处理 person 类别
});
```

## 新 API vs 传统 API

| 功能 | 传统 API | Easy API |
|------|----------|----------|
| 创建流水线 | ~20 行 | ~5 行 |
| 错误处理 | bool + 日志 | Result<T> + 错误信息 |
| 配置相机 | 手动创建 Node | `.withCamera()` |
| 配置检测器 | 手动初始化 | `.withDetector()` |
| 启动 | `setState(Running)` | `start()` |
| 获取错误原因 | 无法获取 | `errorMessage()` |

## 相关文档

- [high_level/PerceptionPipeline.h](../../../include/falconmind/sdk/high_level/PerceptionPipeline.h)
- [high_level/Result.h](../../../include/falconmind/sdk/high_level/Result.h)
- [high_level/ErrorCode.h](../../../include/falconmind/sdk/high_level/ErrorCode.h)

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0.0 | 2026-02-26 | 初始版本，演示 Easy API 使用 |
