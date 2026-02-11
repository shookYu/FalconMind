# FalconMindSDK 示例03：Pad数据传输

## 概述

本示例演示如何使用FalconMindSDK的Pad进行数据端口连接和数据传输。Pad是SDK中连接各个节点的核心组件，负责在节点之间传递数据流。

## 测试的SDK API

本示例测试了以下核心SDK API：

### Pad核心API
| API | 功能 | 验证 |
|-----|------|------|
| `Pad::name()` | 获取Pad名称 | ✅ |
| `Pad::type()` | 获取Pad类型(Source/Sink) | ✅ |
| `Pad::connectTo()` | 连接到目标Pad | ✅ |
| `Pad::disconnect()` | 断开连接 | ✅ |
| `Pad::isConnected()` | 检查连接状态 | ✅ |
| `Pad::connections()` | 获取所有连接 | ✅ |

## 架构图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       Pad 数据传输架构                                   │
│                                                                         │
│   ┌─────────────────────────────────────────────────────────────┐      │
│   │                     数据流向图                                │      │
│   │                                                              │      │
│   │    [Node A]           [Node B]           [Node C]           │      │
│   │   ┌───────┐          ┌───────┐          ┌───────┐           │      │
│   │   │  out  │────────▶│  in   │────────▶│  in   │           │      │
│   │   │ (Src) │   ──▶   │(Src/Snk)│   ──▶   │ (Snk) │           │      │
│   │   └───────┘          └───────┘          └───────┘           │      │
│   │                                                              │      │
│   │   数据: Pad → Pad连接 → 数据传输 → 节点处理                  │      │
│   └─────────────────────────────────────────────────────────────┘      │
│                                                                         │
│   ┌─────────────────────────────────────────────────────────────┐      │
│   │                    Pad 类型说明                              │      │
│   │                                                              │      │
│   │   Source Pad:  数据源端口，只能发送数据                      │      │
│   │   Sink Pad:    数据接收端口，只能接收数据                    │      │
│   │   Both Pad:    双向端口，可发送和接收                        │      │
│   └─────────────────────────────────────────────────────────────┘      │
│                                                                         │
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
│    ./03_pad_data_transfer_x86                                  │
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
03_pad_data_transfer/
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
cd 03_pad_data_transfer/tests

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
cd 03_pad_data_transfer/x86

# 创建构建目录并编译
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 运行测试
./03_pad_data_transfer
```

## 示例功能详解

本示例演示了Pad的完整使用流程，包括Pad创建、类型判断、连接管理和数据传输。

### 1. Pad类型说明

```cpp
// Source类型Pad：数据输出端口
auto srcPad = std::make_shared<Pad>("output", PadType::Source);

// Sink类型Pad：数据输入端口
auto snkPad = std::make_shared<Pad>("input", PadType::Sink);

// 双向类型Pad：可输入输出
auto bothPad = std::make_shared<Pad>("io", PadType::Both);
```

### 2. Pad属性获取

```cpp
// 获取Pad名称
std::cout << "Pad名称: " << pad->name() << std::endl;

// 获取Pad类型
if (pad->type() == PadType::Source) {
    std::cout << "这是一个Source Pad" << std::endl;
}
```

### 3. Pad连接

```cpp
// 连接两个Pad
bool connected = srcPad->connectTo(dstPad);

// 检查连接状态
if (srcPad->isConnected()) {
    std::cout << "已连接到目标Pad" << std::endl;
}

// 获取所有连接
auto connections = srcPad->connections();
```

### 4. Pad断开连接

```cpp
// 断开所有连接
srcPad->disconnect();

// 或断开特定连接
srcPad->disconnect(dstPad);
```

## 完整执行流程图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       Pad 数据传输执行流程                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                         │
│  1. Pad创建阶段                                                         │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ Pad::Pad(name, type)                                 │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     name_ = name                                    │             │
│     │     type_ = type                                    │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
│  2. Pad连接阶段                                                         │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ Pad::connectTo(otherPad)                             │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     connections_.push_back(otherPad)                │             │
│     │     return true                                     │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
│  3. 连接检查阶段                                                       │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ Pad::isConnected()                                   │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     return !connections_.empty()                     │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
│  4. 断开连接阶段                                                       │
│     ┌──────────────────────────────────────────────────────┐             │
│     │ Pad::disconnect()                                    │             │
│     │         │                                           │             │
│     │         ▼                                           │             │
│     │     connections_.clear()                             │             │
│     └──────────────────────────────────────────────────────┘             │
│                                                                         │
└─────────────────────────────────────────────────────────────────────────┘
```

## 代码结构说明

### 头文件包含
```cpp
#include "falconmind/sdk/core/Pad.h"    // Pad核心类
```

### Pad类定义

**Pad类型枚举**:
```cpp
enum class PadType {
    Source,    // 输出端口
    Sink,      // 输入端口
    Both       // 双向端口
};
```

**Pad核心方法**:
```
Pad职责:
├── name()              - 获取Pad名称
├── type()              - 获取Pad类型
├── connectTo(pad)      - 连接到目标Pad
├── disconnect()        - 断开所有连接
├── disconnect(pad)     - 断开特定连接
├── isConnected()       - 检查是否已连接
└── connections()       - 获取所有连接列表
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

### Q4: Pad连接失败
```bash
# 检查Pad类型是否兼容
// Source Pad只能连接Sink Pad
// Sink Pad只能连接Source Pad

# 检查是否已连接
if (srcPad->isConnected()) {
    // 先断开现有连接
    srcPad->disconnect();
}
```

### Q5: 运行时崩溃
```bash
# 检查Pad生命周期
// 确保Pad在连接期间保持有效
// 使用shared_ptr管理Pad生命周期
```

## 输出示例

### x86平台运行输出
```
================================================================================
                FalconMindSDK 示例03: Pad数据传输 (x86)
================================================================================

[1] 创建Source Pad
    Pad名称: output
    Pad类型: Source = 1

[2] 创建Sink Pad
    Pad名称: input
    Pad类型: Sink = 1

[3] 创建双向Pad
    Pad名称: bidirectional
    Pad类型: Both = 1

[4] 连接Source到Sink
    连接状态: 1

[5] 检查连接状态
    Source Pad已连接: 1

[6] 获取连接列表
    连接数量: 1

[7] 断开连接
    断开成功

================================================================================
                    测试通过: Pad核心API验证成功
================================================================================
```

## 注意事项

1. **SDK依赖**: 确保FalconMindSDK核心库已编译
2. **平台选择**: 根据目标平台选择对应的交叉编译工具链
3. **Pad类型**: Source Pad只能连接Sink Pad，反之亦然
4. **生命周期**: 确保Pad在连接期间保持有效
5. **重复连接**: 同一时间一个Source Pad只能连接一个Sink Pad

## 相关文档

- [FalconMindSDK核心API文档](../SDK_core_API.md)
- [Pad设计文档](../Doc/Pad_Design.md)
- [CMake交叉编译配置](../Doc/Cross_Compilation.md)

## 版本历史

| 版本 | 日期 | 变更 |
|------|------|------|
| v1.0.0 | 2026-02-09 | 初始版本，支持4个平台 |
