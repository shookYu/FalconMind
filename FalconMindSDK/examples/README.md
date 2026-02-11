# FalconMindSDK 示例项目

## 概述

本项目包含41个FalconMindSDK示例程序，每个示例支持4个平台（x86/RK3576/RK1126B/RK3588）。

## 平台支持

| 平台 | 推理后端 | NPU算力 | 推荐模型 | 编译方式 |
|------|---------|---------|----------|----------|
| x86 | ONNX Runtime | - | ONNX | 本地编译 |
| RK3576 | RKNN | 6TOPS | YOLOv26s | 交叉编译 |
| RK1126B | RKNN | 1.2TOPS | YOLOv26n | 交叉编译 |
| RK3588 | RKNN | 6TOPS×3 | YOLOv26m | 交叉编译 |

## 示例列表

| 编号 | 示例名称 | SDK API测试 |
|------|---------|------------|
| 01 | Pipeline基础 | Pipeline, Node, Pad, Bus |
| 02 | NodeFactory | NodeFactory动态创建 |
| 03-41 | 基础示例 | 核心API测试 |

## 使用方法

### x86平台（本地编译测试）

```bash
# 进入示例目录
cd examples/01_pipeline_basic/x86

# 创建build目录并编译
mkdir -p build && cd build
cmake ..
make -j4

# 运行测试
./01_pipeline_basic_x86
```

### RK平台（交叉编译）

```bash
# RK3576/RK3588 (aarch64工具链)
cd examples/01_pipeline_basic/rk3576
mkdir -p build && cd build
cmake ..
make -j4

# RK1126B (arm-linux-gnueabihf工具链)
cd examples/01_pipeline_basic/rk1126b
mkdir -p build && cd build
cmake ..
make -j4
```

### 自动化测试

```bash
# 测试x86平台
bash run_all_tests.sh x86

# 测试所有平台
for platform in x86 rk3576 rk1126b rk3588; do
    bash run_all_tests.sh $platform
done
```

## SDK API测试清单

### Core API
- [x] Pipeline::addNode()
- [x] Pipeline::link()
- [x] Pipeline::unlink()
- [x] Pipeline::setState()
- [x] Pipeline::state()
- [x] Pipeline::getNode()
- [x] Pipeline::getLinks()

### Node API
- [x] Node::id()
- [x] Node::setId()
- [x] Node::process()
- [x] Node::getPad()
- [x] Node::configure()

### Pad API
- [x] Pad::connectTo()
- [x] Pad::disconnect()
- [x] Pad::isConnected()
- [x] Pad::connections()

### Bus API
- [x] Bus::subscribe()
- [x] Bus::unsubscribe()
- [x] Bus::post()

### NodeFactory API
- [x] NodeFactory::registerNodeType()
- [x] NodeFactory::createNode()
- [x] NodeFactory::isRegistered()
- [x] NodeFactory::getRegisteredTypes()

## 目录结构

```
examples/
├── 01_pipeline_basic/
│   ├── x86/
│   │   ├── CMakeLists.txt
│   │   ├── src/main.cpp
│   │   └── build/
│   ├── rk3576/
│   ├── rk1126b/
│   └── rk3588/
├── 02_node_factory/
├── 03_example_3/
├── ...
├── 41_example_41/
├── run_all_tests.sh
└── README.md
```

## PX4飞控集成

本示例为基础SDK测试，不需要PX4飞控。

如需集成PX4 SITL模式：

```bash
# 安装PX4
git clone https://github.com/PX4/PX4-Autopilot.git
cd PX4-Autopilot
make px4_sitl_default gazebo
```

## 注意事项

1. x86平台测试需要安装FalconMindSDK库
2. RK平台测试需要配置交叉编译工具链
3. RKNN模型需要单独下载和转换

## 版本历史

- v1.0.0 (2026-02-09) - 初始版本
  - 41个示例
  - 4个平台支持
  - 自动化测试脚本
