# FalconMindSDK 示例01：Pipeline基础流程编排

## 概述

本示例演示如何使用FalconMindSDK创建一个基础的数据处理Pipeline流程。这是整个SDK的核心使用模式，所有复杂的AI感知、传感器融合等功能都建立在Pipeline架构之上。

## 测试的SDK API

本示例测试了以下核心SDK API：

### Pipeline核心API
| API | 功能 | 验证 |
|-----|------|------|
| `Pipeline::addNode()` | 向Pipeline添加处理节点 | ✅ |
| `Pipeline::link()` | 连接两个节点的Pad实现数据流 | ✅ |
| `Pipeline::setState()` | 设置Pipeline的运行状态 | ✅ |
| `Pipeline::state()` | 获取当前Pipeline状态 | ✅ |
| `Pipeline::getNode()` | 获取指定ID的节点对象 | ✅ |
| `Pipeline::getLinks()` | 获取所有连接信息 | ✅ |

### Node核心API
| API | 功能 | 验证 |
|-----|------|------|
| `Node::id()` | 获取节点的唯一标识符 | ✅ |
| `Node::process()` | 执行节点的数据处理逻辑 | ✅ |
| `Node::getPad()` | 获取节点指定名称的Pad | ✅ |

### Pad核心API
| API | 功能 | 验证 |
|-----|------|------|
| `Pad::type()` | 获取Pad类型(Source/Sink) | ✅ |
| `Pad::name()` | 获取Pad名称 | ✅ |

## 架构图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    Pipeline 数据流架构                              │
│                                                                 │
│   ┌──────────┐     ┌──────────┐     ┌──────────┐              │
│   │  Source  │────▶│ Processor │────▶│   Sink   │              │
│   │  Node    │     │  Node    │     │  Node    │              │
│   └──────────┘     └──────────┘     └──────────┘              │
│        │                │                │                       │
│       src             sink            sink                       │
│       out              in               in                       │
│                         out                                    │
│                                                                 │
│   数据流向: Source → Processor → Sink                          │
│                                                                 │
└─────────────────────────────────────────────────────────────────────────┘
```

## 支持的平台

### x86平台
```
┌─────────────────────────────────────────────────────────────────┐
│                      x86 平台配置                           │
├─────────────────────────────────────────────────────────────────┤
│  处理器架构:      x86_64                                    │
│  推理后端:        ONNX Runtime                              │
│  适用场景:        开发、测试环境                            │
│  编译方式:        本地编译 (gcc/g++)                       │
│  模型格式:        ONNX (.onnx)                             │
│                                                                 │
│  编译命令:                                                        │
│    cd x86                                                       │
│    mkdir -p build && cd build                                   │
│    cmake -DCMAKE_BUILD_TYPE=Release ..                          │
│    make -j4                                                    │
│                                                                 │
│  运行命令:                                                        │
│    ./01_pipeline_basic_x86                                     │
└─────────────────────────────────────────────────────────────────┘
```

### RK3576平台
```
┌─────────────────────────────────────────────────────────────────┐
│                     RK3576 平台配置                           │
├─────────────────────────────────────────────────────────────────┤
│  处理器架构:      ARM Cortex-A76 (64-bit)                      │
│  工艺制程:        22nm                                         │
│  NPU算力:        6 TOPS (INT8)                                │
│  推理后端:        RKNN                                         │
│  适用场景:        边缘端AI部署                                 │
│  编译方式:        交叉编译 (aarch64-linux-gnu)               │
│  模型格式:        RKNN (.rknn)                                │
│  推荐模型:        YOLOv26s (640×640输入尺寸)                   │
│                                                                 │
│  交叉编译命令:                                                   │
│    export TOOLCHAIN=/opt/aarch64-linux-gnu                    │
│    cd rk3576                                                   │
│    mkdir -p build && cd build                                 │
│    cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..        │
│    make -j4                                                    │
└─────────────────────────────────────────────────────────────────┘
```

### RV1126B平台
```
┌─────────────────────────────────────────────────────────────────┐
│                     RV1126B 平台配置                           │
├─────────────────────────────────────────────────────────────────┤
│  处理器架构:      ARM Cortex-A53 (64-bit)                      │
│  工艺制程:        22nm                                          │
│  NPU算力:        3.0 TOPS (INT8/INT16)                        │
│  推理后端:        RKNN                                         │
│  适用场景:        AI视觉处理器，入门级边缘设备                  │
│  编译方式:        交叉编译 (aarch64-linux-gnu)               │
│  模型格式:        RKNN (.rknn)                                │
│  推荐模型:        YOLOv26n (416×416输入尺寸，小型模型)          │
│                                                                 │
│  交叉编译命令:                                                   │
│    export TOOLCHAIN=/opt/aarch64-linux-gnu                    │
│    cd rv1126b                                                  │
│    mkdir -p build && cd build                                 │
│    cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..        │
│    make -j4                                                    │
│                                                                 │
│  特点: RV1126升级版，NPU从2TOPS提升到3TOPS                     │
└─────────────────────────────────────────────────────────────────┘
```

### RK3588平台
```
┌─────────────────────────────────────────────────────────────────┐
│                     RK3588 平台配置                           │
├─────────────────────────────────────────────────────────────────┤
│  处理器架构:      ARM Cortex-A76×4 + Cortex-A55×4              │
│  工艺制程:        8nm                                          │
│  NPU算力:        6 TOPS (集成3核NPU, INT4/INT8/INT16)         │
│  推理后端:        RKNN                                         │
│  适用场景:        高性能边缘计算                               │
│  编译方式:        交叉编译 (aarch64-linux-gnu)               │
│  模型格式:        RKNN (.rknn)                                │
│  推荐模型:        YOLOv26m (1280×1280输入尺寸，中型模型)        │
│  GPU:            ARM Mali-G610 MP4                             │
│  特殊功能:        支持8K视频编解码                             │
│                                                                 │
│  交叉编译命令:                                                   │
│    export TOOLCHAIN=/opt/aarch64-linux-gnu                    │
│    cd rk3588                                                   │
│    mkdir -p build && cd build                                 │
│    cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..        │
│    make -j4                                                    │
│                                                                 │
│  NPU核心配置:                                                   │
│    核心0: 预处理 (图像归一化、Resize)                          │
│    核心1: 主体推理 (YOLO检测)                                  │
│    核心2: 后处理 (NMS、坐标转换)                               │
└─────────────────────────────────────────────────────────────────┘
```

## 目录结构

```
01_pipeline_basic/
├── README.md                    # 本文档
├── CMakeLists.txt               # 主CMake配置（可选）
│
├── x86/                         # x86平台
│   ├── CMakeLists.txt           # x86平台CMake配置
│   ├── src/
│   │   └── main.cpp            # x86平台源代码
│   └── build/                  # 构建目录（编译后生成）
│
├── rk3576/                      # RK3576平台
│   ├── CMakeLists.txt           # RK3576交叉编译配置
│   ├── toolchain.cmake         # 交叉编译工具链文件
│   ├── src/
│   │   └── main.cpp            # RK3576平台源代码
│   └── build/                  # 构建目录
│
├── rv1126b/                     # RV1126B平台
│   ├── CMakeLists.txt           # RV1126B交叉编译配置
│   ├── toolchain.cmake         # 交叉编译工具链文件
│   ├── src/
│   │   └── main.cpp            # RV1126B平台源代码
│   └── build/                  # 构建目录
│
├── rk3588/                      # RK3588平台
│   ├── CMakeLists.txt           # RK3588交叉编译配置
│   ├── toolchain.cmake         # 交叉编译工具链文件
│   ├── src/
│   │   └── main.cpp            # RK3588平台源代码
│   └── build/                  # 构建目录
│
└── tests/                      # 测试脚本
    ├── build_all.sh            # 一键构建所有平台
    ├── build_x86.sh            # x86本地编译脚本
    ├── build_rv1126b.sh        # RV1126B交叉编译脚本
    ├── build_rk3576.sh         # RK3576交叉编译脚本
    └── build_rk3588.sh         # RK3588交叉编译脚本
```

## 快速开始

### 使用自动构建脚本（推荐）

```bash
# 进入测试目录
cd 01_pipeline_basic/tests

# 添加执行权限
chmod +x build_all.sh

# 一键构建所有平台
./build_all.sh

# 或单独构建某个平台
./build_x86.sh      # x86本地编译
./build_rv1126b.sh  # RV1126B交叉编译
./build_rk3576.sh  # RK3576交叉编译
./build_rk3588.sh  # RK3588交叉编译
```

### x86平台（本地编译测试）

```bash
# 进入x86平台目录
cd 01_pipeline_basic/x86

# 创建构建目录并编译
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 运行测试
./01_pipeline_basic
```

### 下载模型和测试图片

```bash
# 下载YOLOv26模型
cd 01_pipeline_basic/tests
chmod +x download_models.sh
./download_models.sh

# 下载测试图片
chmod +x download_images.sh
./download_images.sh
```

## 示例功能详解

本示例创建了一个完整的数据处理Pipeline流程，包含以下功能：

### 1. Pipeline实例创建
```cpp
// 创建Pipeline配置
PipelineConfig cfg{
    "pipeline_001",           // pipelineId
    "基础数据处理流程",        // name
    "演示Pipeline基本创建和连接流程"  // description
};

// 创建Pipeline实例
auto pipeline = std::make_shared<Pipeline>(cfg);
```

### 2. 节点创建与添加
```cpp
// 创建源节点（数据生产者）
auto sourceNode = std::make_shared<SourceNode>("source");

// 创建处理节点（数据处理器）
auto processorNode = std::make_shared<ProcessNode>("processor");

// 创建汇节点（数据消费者）
auto sinkNode = std::make_shared<SinkNode>("sink");

// 将节点添加到Pipeline
pipeline->addNode(sourceNode);
pipeline->addNode(processorNode);
pipeline->addNode(sinkNode);
```

### 3. Pad连接与数据流
```cpp
// 连接源节点 → 处理节点
pipeline->link("source", "out", "processor", "in");

// 连接处理节点 → 汇节点
pipeline->link("processor", "out", "sink", "in");
```

### 4. 状态管理
```cpp
// 设置Pipeline状态为Ready
pipeline->setState(PipelineState::Ready);

// 获取当前状态
auto state = pipeline->state();
```

### 5. 数据处理执行
```cpp
// 依次执行各节点的处理逻辑
sourceNode->process();
processorNode->process();
sinkNode->process();
```

## 完整执行流程图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       Pipeline 执行流程                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. 初始化阶段                                                         │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ Pipeline::Pipeline(config)                            │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │ SourceNode::SourceNode()                            │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │ ProcessNode::ProcessNode()                          │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │   SinkNode::SinkNode()                             │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
│  2. 节点注册阶段                                                       │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ for each node:                                     │             │
│     │     Pipeline::addNode(node)                         │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     nodes.push_back(node)                          │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
│  3. 连接建立阶段                                                       │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ Pipeline::link(srcNode, srcPad, dstNode, dstPad)   │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     links.push_back(LinkInfo)                      │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
│  4. 状态转换阶段                                                       │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ Pipeline::setState(Ready)                           │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     state_ = PipelineState::Ready                  │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
│  5. 数据处理阶段                                                       │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ SourceNode::process()                               │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     ProcessorNode::process()                        │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     SinkNode::process()                             │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## 代码结构说明

### 头文件包含
```cpp
#include "falconmind/sdk/core/Pipeline.h"    // Pipeline核心类
#include "falconmind/sdk/core/Node.h"         // Node基类
#include "falconmind/sdk/core/Pad.h"          // Pad数据端口类
```

### 节点类定义

**SourceNode（源节点）**
```
职责: 生成初始数据
     ├── Pad: out (Source类型)
     └── process(): 生成模拟数据并输出
```

**ProcessNode（处理节点）**
```
职责: 处理输入数据
     ├── Pad: in (Sink类型) - 接收上游数据
     ├── Pad: out (Source类型) - 发送处理结果
     └── process(): 执行数据处理逻辑
```

**SinkNode（汇节点）**
```
职责: 接收并处理最终数据
     └── Pad: in (Sink类型) - 接收输入数据
```

## 常见问题

### Q1: 编译失败，找不到FalconMindSDK头文件
```bash
# 确保FalconMindSDK已编译
cd /path/to/FalconMindSDK
mkdir -p build && cd build
cmake ..
make -j4

# 设置环境变量
export FALCONMIND_SDK=/path/to/FalconMindSDK
```

### Q2: x86平台编译报错找不到onnxruntime
```bash
# 安装ONNX Runtime
sudo apt-get install onnxruntime

# 或使用pip安装
pip3 install onnxruntime
```

### Q3: RK平台交叉编译失败
```bash
# 确认交叉编译工具链已安装
aarch64-linux-gnu-gcc --version

# 安装工具链（Ubuntu）
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
```

### Q4: Pipeline.link()返回false
```bash
# 检查节点是否已添加到Pipeline
pipeline->addNode(sourceNode);  // 必须先添加
pipeline->link("source", "out", "processor", "in");  // 再连接

# 检查Pad名称是否正确
// SourceNode的out Pad名为"out"
// ProcessNode的输入Pad名为"in"
```

### Q5: 运行时崩溃
```bash
# 检查节点生命周期
// Pipeline持有的节点必须是shared_ptr
// 确保节点在Pipeline销毁前保持有效
```

## 输出示例

### x86平台运行输出
```
================================================================================
                    FalconMindSDK 示例01: Pipeline基础流程
================================================================================

    +----------------+----------------+----------------+
    |    [source]    |  [processor]   |     [sink]     |
    |                |                |                |
    |     out ------>| in      out -->| in             |
    |                |                |                |
    +----------------+----------------+----------------+

Pipeline ID: pipeline_001
节点数量: 3
连接数量: 2

当前状态: Ready

执行数据流:
[Source] 生成数据
[Process] 处理数据
[Sink] 接收数据

================================================================================
                        测试通过: Pipeline核心API验证成功
================================================================================
```

## 注意事项

1. **SDK依赖**: 确保FalconMindSDK核心库已编译
2. **平台选择**: 根据目标平台选择对应的交叉编译工具链
3. **模型下载**: AI推理示例需要下载对应的模型文件
4. **资源清理**: Pipeline使用shared_ptr自动管理内存

## 相关文档

- [FalconMindSDK核心API文档](../SDK_core_API.md)
- [Pipeline架构设计文档](../Doc/Pipeline_Design.md)
- [RKNN SDK集成指南](../Doc/RKNN_Integration.md)
- [CMake交叉编译配置](../Doc/Cross_Compilation.md)

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0.0 | 2026-02-09 | 初始版本，支持4个平台 |
