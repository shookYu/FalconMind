# FalconMindSDK 示例02：NodeFactory节点工厂

## 概述

本示例演示如何使用FalconMindSDK的NodeFactory创建和管理处理节点。NodeFactory是SDK的核心组件之一，提供节点的动态创建、注册和工厂模式支持。

## 测试的SDK API

本示例测试了以下核心SDK API：

### NodeFactory核心API
| API | 功能 | 验证 |
|-----|------|------|
| `NodeFactory::registerNodeType()` | 注册节点类型 | ✅ |
| `NodeFactory::createNode()` | 创建节点实例 | ✅ |
| `NodeFactory::isRegistered()` | 检查节点类型是否已注册 | ✅ |
| `NodeFactory::getRegisteredTypes()` | 获取所有已注册的节点类型 | ✅ |

## 架构图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                    NodeFactory 工厂模式架构                             │
│                                                                 │
│   ┌─────────────────────────────────────────────────────────┐        │
│   │                    NodeFactory                          │        │
│   │  ┌─────────────────────────────────────────────────┐   │        │
│   │  │              节点类型注册表                       │   │        │
│   │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐        │   │        │
│   │  │  │ MyNode  │  │ DataNode│  │ProcessNode│       │   │        │
│   │  │  └────┬────┘  └────┬────┘  └────┬────┘        │   │        │
│   │  └───────┼────────────┼────────────┼──────────────┘   │        │
│   │          │            │             │                  │        │
│   └──────────┼────────────┼─────────────┼──────────────────┘   │
│              │            │             │                        │
│              ▼            ▼             ▼                        │
│         isRegistered  getRegisteredTypes   registerNodeType        │
│                                                                 │
│   数据流向: 代码 → 注册表 → 类型查询                              │
│                                                                 │
└─────────────────────────────────────────────────────────────────────────┘
```
┌─────────────────────────────────────────────────────────────────────────┐
│                    NodeFactory 工厂模式架构                             │
│                                                                 │
│   ┌─────────────────────────────────────────────────────────┐        │
│   │                    NodeFactory                          │        │
│   │  ┌─────────────────────────────────────────────────┐   │        │
│   │  │              节点类型注册表                       │   │        │
│   │  │  ┌─────────┐  ┌─────────┐  ┌─────────┐        │   │        │
│   │  │  │ MyNode  │  │ DataNode│  │ProcessNode│       │   │        │
│   │  │  └────┬────┘  └────┬────┘  └────┬────┘        │   │        │
│   │  │       │             │             │              │   │        │
│   │  │       ▼             ▼             ▼              │   │        │
│   │  │   [Factory Function] ───────────────────────────▶│   │        │
│   │  └─────────────────────────────────────────────────┘   │        │
│   │                        │                               │        │
│   │                        ▼                               │        │
│   │              ┌─────────────────────┐                   │        │
│   │              │   动态节点创建       │                   │        │
│   │              │   createNode()      │                   │        │
│   │              └─────────────────────┘                   │        │
│   └─────────────────────────────────────────────────────────┘        │
│                                                                 │
│   数据流向: 配置文件/代码 → 注册表 → 工厂函数 → 节点实例            │
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
│  适用场景:        开发、测试环境                            │
│  编译方式:        本地编译 (gcc/g++)                       │
│                                                                 │
│  编译命令:                                                        │
│    cd x86                                                       │
│    mkdir -p build && cd build                                   │
│    cmake -DCMAKE_BUILD_TYPE=Release ..                          │
│    make -j4                                                    │
│                                                                 │
│  运行命令:                                                        │
│    ./02_node_factory_x86                                       │
└─────────────────────────────────────────────────────────────────┘
```

### RK3576平台
```
┌─────────────────────────────────────────────────────────────────┐
│                     RK3576 平台配置                           │
├─────────────────────────────────────────────────────────────────┤
│  处理器架构:      ARM Cortex-A76 (64-bit)                      │
│  工艺制程:        22nm                                         │
│  推理后端:        RKNN                                         │
│  适用场景:        边缘端AI部署                                 │
│  编译方式:        交叉编译 (aarch64-linux-gnu)               │
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
└─────────────────────────────────────────────────────────────────┘
```

## 目录结构

```
02_node_factory/
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
cd 02_node_factory/tests

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
cd 02_node_factory/x86

# 创建构建目录并编译
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 运行测试
./02_node_factory
```

## 示例功能详解

本示例演示了NodeFactory的完整使用流程，包括节点类型注册、动态创建和管理。

### 1. 定义自定义节点

```cpp
// 定义数据源节点
class SourceNode : public Node {
public:
    explicit SourceNode(const std::string& nodeId) : Node(nodeId) {
        auto outPad = std::make_shared<Pad>("out", PadType::Source);
        addPad(outPad);
    }
    void process() override { std::cout << "[SourceNode] 生成数据" << std::endl; }
};

// 定义数据处理节点
class ProcessNode : public Node {
public:
    explicit ProcessNode(const std::string& nodeId) : Node(nodeId) {
        auto inPad = std::make_shared<Pad>("in", PadType::Sink);
        auto outPad = std::make_shared<Pad>("out", PadType::Source);
        addPad(inPad);
        addPad(outPad);
    }
    void process() override { std::cout << "[ProcessNode] 处理数据" << std::endl; }
};

// 定义数据汇节点
class SinkNode : public Node {
public:
    explicit SinkNode(const std::string& nodeId) : Node(nodeId) {
        auto inPad = std::make_shared<Pad>("in", PadType::Sink);
        addPad(inPad);
    }
    void process() override { std::cout << "[SinkNode] 接收数据" << std::endl; }
};
```

### 2. 注册节点类型到工厂

```cpp
// 注册自定义节点类型
NodeFactory::registerNodeType("SourceNode",
    [](const std::string& id, const void*) {
        return std::make_shared<SourceNode>(id);
    }
);

NodeFactory::registerNodeType("ProcessNode",
    [](const std::string& id, const void*) {
        return std::make_shared<ProcessNode>(id);
    }
);

NodeFactory::registerNodeType("SinkNode",
    [](const std::string& id, const void*) {
        return std::make_shared<SinkNode>(id);
    }
);
```

### 4. 检查已注册类型

```cpp
// 检查节点类型是否已注册
bool hasSource = NodeFactory::isRegistered("SourceNode");

// 获取所有已注册的节点类型
auto types = NodeFactory::getRegisteredTypes();
std::cout << "已注册的节点类型数量: " << types.size() << std::endl;
```



## 完整执行流程图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       NodeFactory 执行流程                              │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. 初始化阶段                                                         │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ NodeFactory::NodeFactory()                            │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     type_registry_ = empty                           │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
│  2. 节点类型注册阶段                                                   │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ for each node type:                                  │             │
│     │     NodeFactory::registerNodeType(name, factory)    │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     type_registry_[name] = factory                   │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
│  3. 类型查询阶段                                                       │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ NodeFactory::isRegistered(type)                      │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     return type_registry_.count(type) > 0           │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
│  4. 获取类型列表阶段                                                   │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ NodeFactory::getRegisteredTypes()                   │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     return all keys from type_registry_             │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## 代码结构说明

### 头文件包含
```cpp
#include "falconmind/sdk/core/NodeFactory.h"    // NodeFactory工厂类
#include "falconmind/sdk/core/Node.h"            // Node基类
#include "falconmind/sdk/core/Pad.h"             // Pad数据端口类
```

### NodeFactory类定义

**功能**: 提供节点的动态创建和管理
```
NodeFactory职责:
├── registerNodeType(name, factory)  - 注册节点类型
├── createNode(type, id, config)      - 创建节点实例
├── isRegistered(type)               - 检查注册状态
└── getRegisteredTypes()              - 获取所有类型
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

### Q4: NodeFactory::createNode返回nullptr
```bash
# 检查节点类型是否已注册
NodeFactory::isRegistered("MyNode");  // 应返回true

# 检查注册时的工厂函数是否正确
// Lambda必须返回std::shared_ptr<Node>
```

### Q5: 重复注册同一节点类型
```bash
# NodeFactory不允许重复注册同名类型
# 如需重新注册同名类型，需要重启程序或使用新的类型名称
```

## 输出示例

### x86平台运行输出
```
================================================================================
                    FalconMindSDK 示例02: NodeFactory节点工厂 (x86)
================================================================================

[1] 初始化NodeFactory
[2] 注册节点类型
    - SourceNode: 已注册
    - ProcessNode: 已注册
    - SinkNode: 已注册

[3] 检查注册状态
    SourceNode 已注册: 1
    ProcessNode 已注册: 1
    SinkNode 已注册: 1

[4] 检查已注册类型
    SourceNode 已注册: 1
    ProcessNode 已注册: 1
    SinkNode 已注册: 1

[5] 获取已注册类型列表
    已注册的节点类型数量: 3
    类型列表: SinkNode, ProcessNode, SourceNode

[6] 执行节点处理
[SourceNode] 生成数据
[ProcessNode] 处理数据
[SinkNode] 接收数据

================================================================================
                    测试通过: NodeFactory核心API验证成功
================================================================================
```

## 注意事项

1. **SDK依赖**: 确保FalconMindSDK核心库已编译
2. **平台选择**: 根据目标平台选择对应的交叉编译工具链
3. **工厂函数**: 注册时必须提供正确的工厂函数
4. **资源清理**: NodeFactory使用工厂模式自动管理内存
5. **重复注册**: 不允许重复注册同一节点类型

## 相关文档

- [FalconMindSDK核心API文档](../SDK_core_API.md)
- [NodeFactory设计文档](../Doc/NodeFactory_Design.md)
- [CMake交叉编译配置](../Doc/Cross_Compilation.md)

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0.0 | 2026-02-09 | 初始版本，支持4个平台 |
