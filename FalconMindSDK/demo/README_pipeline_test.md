# Pipeline Test Demo 示例说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/04_FalconMindSDK_Design.md** - SDK 设计说明
- **Doc/09_SDK_Pipeline_DevGuide.md** - SDK Pipeline 开发指南

## `pipeline_test_main` 示例说明

### 1. 用例目的

这个示例演示 FalconMindSDK 中 **Pipeline/Node 核心框架的最小用法**，主要用于：
- 验证 `Pipeline` 能创建并管理节点。  
- 验证 `Node`/`Pad` 的基本接口与连线机制。  
- 作为后续自定义节点开发的最简参考。

示例逻辑非常简单：
- 创建一个 `TestSourceNode`（源节点）和一个 `LogSinkNode`（汇聚节点）。  
- 将它们加入 `Pipeline` 并连接：`TestSourceNode.out → LogSinkNode.in`。  
- 切换 Pipeline 状态到 `READY/PLAYING`，手动调用一次 `process()`，在终端看到日志输出。

### 2. 实现概览

相关文件：
- `demo/TestNodes.h/.cpp`：  
  - `TestSourceNode`：继承自 `core::Node`，在 `process()` 中打印简单日志。  
  - `LogSinkNode`：继承自 `core::Node`，在 `process()` 中打印带前缀的日志。  
- `demo/pipeline_test_main.cpp`：  
  - 构造 `PipelineConfig`；  
  - 创建 `Pipeline` 实例；  
  - 创建并添加 `TestSourceNode`、`LogSinkNode`；  
  - 连接两个节点，并调用 `process()`。

### 3. 如何编译

在 SDK 根目录执行（第一次需要先生成 build 目录）：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK
mkdir -p build
cd build
cmake ..
cmake --build .
```

成功后会在 `build/` 目录下生成可执行文件：
- `falconmind_sdk_demo`

### 4. 如何运行

在 `build` 目录下执行：

```bash
cd /home/shook/work/FalconMind/FalconMindSDK/build
./falconmind_sdk_demo
```

预期输出类似：

```text
[TestSource] process called
[LogSink] process called
Pipeline demo finished.
```

### 5. 适合用来做什么

- 学习 Pipeline/Node 的基本使用方式；  
- 开发新节点前，快速验证节点基类的行为；  
- 集成到 IDE 调试环境中，用断点调试 Node 生命周期与连线逻辑。  

