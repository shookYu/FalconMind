# FalconMindSDK 快速入门指南

> **目标**：让新用户在 **30 分钟内** 跑通第一个 Demo

---

## 📋 目录

1. [一分钟了解 FalconMindSDK](#一分钟了解-falconmindsdk)
2. [五分钟安装](#五分钟安装)
3. [十分钟跑通第一个 Demo](#十分钟跑通第一个-demo)
4. [三十分钟理解核心概念](#三十分钟理解核心概念)
5. [下一步](#下一步)

---

## 一分钟了解 FalconMindSDK

**FalconMindSDK** 是面向无人机的高性能 AI 感知与任务编排框架。

### 核心特性

```
🚀 高性能 AI 推理    - 支持 RK3588/RK3576/RV1126B NPU 加速
🔧 简化 API          - 几行代码启动完整感知流水线  
📦 多平台支持        - x86/ARM64 统一开发体验
🌐 飞控集成          - MAVLink 协议支持
```

### 两种 API 风格

| 风格 | 适用场景 | 代码量 |
|------|----------|--------|
| **Easy API** (推荐) | 快速原型、业务开发 | ~10 行 |
| **Core API** | 底层定制、框架扩展 | ~50 行 |

---

## 五分钟安装

### 环境要求

- **x86 开发**: Ubuntu 20.04+, CMake 3.16+, GCC 9+
- **ARM64 部署**: RK3588/RK3576/RV1126B 开发板

### 1. 克隆代码

```bash
git clone https://github.com/your-org/falconmind.git
cd falconmind/FalconMindSDK
```

### 2. 安装依赖 (x86)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake git

# 安装 RKNN Toolkit2 (用于模型转换)
# 参考: https://github.com/rockchip-linux/rknn-toolkit2

### 3. 编译 SDK

```bash
# x86 平台
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4
sudo make install  # 安装到 install/x86/
```

**验证安装**:
```bash
ls install/x86/lib/libfalconmind_sdk.a
# 应该看到静态库文件
```

---

## 十分钟跑通第一个 Demo

### 使用 Easy API (推荐)

```bash
# 进入 Easy API 示例
cd examples/45_easy_api_demo/x86
mkdir -p build && cd build
cmake ..
make -j4

# 运行
./45_easy_api_demo_x86
```

**预期输出**:
```
================================================================================
                    FalconMindSDK 示例45: Easy API 演示
================================================================================

=== 示例1: Builder模式创建感知流水线 ===
✅ 流水线创建成功
[PerceptionPipeline] Starting with camera: 640x480 @ 30fps
[PerceptionPipeline] Detector: yolov8n.rknn
✅ 流水线已启动
...
✅ 所有演示完成
```

### 理解这段代码

```cpp
#include "falconmind/sdk/high_level/PerceptionPipeline.h"

using namespace falconmind::sdk::high_level;

int main() {
    // 创建流水线（链式配置）
    auto result = PerceptionPipeline::create()
        .withCamera(640, 480, 30)                    // 配置相机
        .withDetector("yolov8n.rknn")                // 加载检测模型
        .withTracker(TrackerType::DEEPSORT)         // 启用跟踪
        .build();
    
    // 错误处理（不再是简单的 bool）
    if (result.isError()) {
        std::cerr << "错误: " << result.errorMessage() << std::endl;
        return 1;
    }
    
    auto pipeline = result.value();
    
    // 设置回调
    pipeline->onDetection([](const std::vector<Detection>& dets) {
        for (const auto& det : dets) {
            std::cout << "检测到: " << det.className << std::endl;
        }
    });
    
    // 启动
    pipeline->start();
    
    // 运行 10 秒
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    pipeline->stop();
    return 0;
}
```

---

## 三十分钟理解核心概念

### 1. Pipeline 架构

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Camera    │────▶│  Detector   │────▶│  Tracker    │
│  (Source)   │     │  (Process)  │     │   (Sink)    │
└─────────────┘     └─────────────┘     └─────────────┘
```

### 2. 三种配置方式

**方式 1: Builder 模式（推荐）**
```cpp
auto result = PerceptionPipeline::create()
    .withCamera(640, 480, 30)
    .withDetector("model.rknn")
    .withTracker(TrackerType::DEEPSORT)
    .build();
```

**方式 2: 便捷函数**
```cpp
auto result = createMinimalPipeline("model.rknn");
```

**方式 3: Core API（底层控制）**
```cpp
auto pipeline = std::make_shared<Pipeline>(config);
auto camera = std::make_shared<CameraSourceNode>("cam");
auto detector = std::make_shared<DetectorNode>("det");
pipeline->addNode(camera);
pipeline->addNode(detector);
pipeline->link("cam", "out", "det", "in");
```

### 3. 错误处理最佳实践

```cpp
// 方式 1: 显式检查
if (result.isSuccess()) {
    auto pipeline = result.value();
}

// 方式 2: bool 转换
if (result) {
    auto pipeline = result.value();
}

// 方式 3: 安全获取（带默认值）
auto pipeline = result.valueOr(nullptr);

// 方式 4: 函数式处理
result.map([](auto p) {
    p->start();
    return p;
});
```

### 4. 回调机制

```cpp
// 批量检测回调
pipeline->onDetection([](const std::vector<Detection>& detections) {
    std::cout << "检测到 " << detections.size() << " 个目标" << std::endl;
});

// 单目标回调
pipeline->onDetection([](const Detection& det) {
    if (det.isTracked()) {
        std::cout << "跟踪 ID: " << det.trackId << std::endl;
    }
});

// 特定类别回调
pipeline->onObjectDetected("person", [](const Detection& det) {
    std::cout << "发现行人!" << std::endl;
});
```

---

## 下一步

### 运行更多示例

```bash
# 基础 API 示例
cd examples/01_pipeline_basic/x86/build
./01_pipeline_basic_x86

# 目标检测示例
cd examples/06_rknn_yolo_inference/x86/build
./06_rknn_yolo_inference_x86

# 多目标跟踪示例
cd examples/15_detection_tracking/x86/build
./15_detection_tracking_x86
```

### 阅读详细文档

- [API 参考文档](API_REFERENCE.md) - 完整 API 文档
- [架构设计文档](../README.md#系统架构) - 深入了解架构
- [示例程序说明](../examples/README.md) - 41 个示例详解

### 开始你的项目

```cpp
// my_first_app.cpp
#include "falconmind/sdk/high_level/PerceptionPipeline.h"

int main() {
    auto result = falconmind::sdk::high_level::createMinimalPipeline(
        "yolov8n.rknn"
    );
    
    if (!result) {
        std::cerr << "启动失败: " << result.errorMessage() << std::endl;
        return 1;
    }
    
    auto pipeline = result.value();
    
    pipeline->onObjectDetected("person", [](const auto& det) {
        std::cout << "检测到行人，置信度: " << det.confidence << std::endl;
    });
    
    pipeline->start();
    pipeline->wait();  // 持续运行直到手动停止
    
    return 0;
}
```

编译：
```bash
g++ -std=c++17 my_first_app.cpp -lfalconmind_sdk -pthread -o my_first_app
./my_first_app
```

---

## 💡 常见问题

**Q: 编译时报错找不到头文件？**  
A: 确保已执行 `sudo make install`，并检查 `install/x86/include/` 是否存在。

**Q: 运行时提示模型文件不存在？**  
A: 下载 YOLO 模型文件到项目目录，或修改代码中的路径。

**Q: 如何在 RK3588 上部署？**  
A: 使用交叉编译工具链，参考 `examples/01_pipeline_basic/rk3588/` 的 CMake 配置。

**Q: Easy API 和 Core API 有什么区别？**  
A: Easy API 是高层封装，适合业务开发；Core API 是底层接口，适合深度定制。

---

## 📞 获取帮助

- **GitHub Issues**: 报告问题或请求功能
- **文档**: 查看 `Doc/` 目录下的详细文档
- **示例**: 参考 `examples/` 目录下的 41 个示例程序

---

<div align="center">

**🎉 恭喜！你已完成 FalconMindSDK 快速入门**

[开始你的第一个项目 →](#开始你的项目)

</div>
