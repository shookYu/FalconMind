# FalconMindSDK - 智能无人机AI感知与任务编排系统

## 简介

FalconMindSDK 是面向无人机的高性能 AI 感知与任务编排框架，支持多种 Rockchip AI 芯片平台。

## 快速开始

```bash
# 编译 SDK
cd FalconMindSDK/build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
sudo make install

# 运行 Easy API 示例
cd ../examples/45_easy_api_demo/x86
mkdir -p build && cd build
cmake .. && make -j4
./45_easy_api_demo_x86
```

## 核心特性

- **🚀 高性能 AI 推理** - 支持 RK3588/RK3576/RV1126B NPU
- **🔧 Easy API** - 几行代码启动完整感知流水线
- **📦 多平台支持** - x86/ARM64 统一开发体验
- **🌐 飞控集成** - MAVLink 协议支持

## 两种 API 风格

### Easy API (推荐)
```cpp
auto result = PerceptionPipeline::create()
    .withCamera(640, 480, 30)
    .withDetector("yolov8.onnx")
    .withTracker(TrackerType::DEEPSORT)
    .build();

if (result) {
    auto pipeline = result.value();
    pipeline->onDetection([](const auto& dets) {
        for (const auto& d : dets) {
            std::cout << d.className << std::endl;
        }
    });
    pipeline->start();
}
```

### Core API
```cpp
auto pipeline = std::make_shared<Pipeline>(config);
auto source = std::make_shared<SourceNode>("source");
auto detector = std::make_shared<DetectorNode>("det");
pipeline->addNode(source);
pipeline->addNode(detector);
pipeline->link("source", "out", "det", "in");
```

## 文档

- [快速入门指南](Doc/GETTING_STARTED.md) - 30 分钟上手
- [改进总结](IMPROVEMENT_SUMMARY.md) - 本次改进详情
- [API 参考](docs/api/html/index.html) - 完整 API 文档

## 示例

41 个示例程序涵盖：
- 核心 API 使用
- AI 推理和跟踪
- 传感器集成
- 飞控通信
- 任务规划

## 许可证

Apache License 2.0
