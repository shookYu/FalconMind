# NodeAgent 单元测试指南

> **最后更新**: 2024-01-30

## 📚 相关文档

- **../README.md** - NodeAgent 总体说明
- **Doc/07_NodeAgent_Cluster_Design.md** - NodeAgent 和 Cluster Center 设计


# NodeAgent 单元测试指南

## 概述

NodeAgent 使用 **Google Test (GTest)** 作为单元测试框架，为所有核心组件提供了全面的单元测试覆盖。

## 测试框架

- **框架**：Google Test (GTest) v1.14.0
- **测试发现**：使用 CMake 的 `gtest_discover_tests` 自动发现测试
- **Mock 支持**：Google Mock (GMock) 可用于创建模拟对象

## 测试结构

```
NodeAgent/
├── tests/
│   ├── unit_tests.cpp              # 主测试入口
│   ├── command_handler_tests.cpp   # CommandHandler 测试
│   ├── mission_handler_tests.cpp   # MissionHandler 测试
│   ├── message_ack_tests.cpp       # MessageAckManager 测试
│   ├── multi_uav_tests.cpp         # MultiUavManager 测试
│   ├── logger_tests.cpp            # Logger 测试
│   ├── error_statistics_tests.cpp  # ErrorStatistics 测试
│   └── reconnect_manager_tests.cpp # ReconnectManager 测试
```

## 编译和运行测试

### 编译测试

```bash
cd NodeAgent/build
cmake .. -DNODEAGENT_BUILD_TESTS=ON
cmake --build . --target nodeagent_unit_tests
```

### 运行所有测试

```bash
./nodeagent_unit_tests
```

### 运行特定测试

```bash
# 运行特定测试套件
./nodeagent_unit_tests --gtest_filter=CommandHandlerTest.*

# 运行特定测试
./nodeagent_unit_tests --gtest_filter=CommandHandlerTest.HandleArmCommand
```

### 使用 CMake CTest

```bash
# 运行所有测试（通过 CTest）
ctest

# 详细输出
ctest --verbose

# 运行特定测试
ctest -R CommandHandlerTest
```

## 测试覆盖

### CommandHandler 测试

- ✅ 基本初始化
- ✅ 无 FlightService 时的处理
- ✅ ARM 命令处理
- ✅ TAKEOFF 命令处理
- ✅ RTL 命令处理
- ✅ 无效 JSON 处理
- ✅ 未知命令类型处理

### MissionHandler 测试

- ✅ 基本初始化
- ✅ 无 FlightService 时的处理
- ✅ takeoff_and_hover 任务处理
- ✅ 无效 JSON 处理
- ✅ 未知任务类型处理
- ✅ 无活动任务时的更新

### MessageAckManager 测试

- ✅ 基本初始化
- ✅ 注册待确认消息（带 requestId）
- ✅ 注册消息（无 requestId，自动生成）
- ✅ 消息确认
- ✅ 消息超时检测
- ✅ 最大重试次数限制
- ✅ 无待确认消息时的更新

### MultiUavManager 测试

- ✅ 基本初始化
- ✅ 添加 UAV
- ✅ 添加重复 UAV（应失败）
- ✅ 移除 UAV
- ✅ 移除不存在的 UAV（应失败）
- ✅ 添加多个 UAV
- ✅ UAV 运行状态检查
- ✅ 空列表时的操作

### Logger 测试

- ✅ 基本初始化
- ✅ 设置和获取日志级别
- ✅ 日志级别过滤
- ✅ 控制台输出控制
- ✅ 日志宏测试
- ✅ 线程安全性测试

### ErrorStatistics 测试

- ✅ 基本初始化
- ✅ 记录错误
- ✅ 记录多个错误
- ✅ 记录不同错误码
- ✅ 获取总错误数
- ✅ 获取所有统计
- ✅ 重置统计
- ✅ 线程安全性测试

### ReconnectManager 测试

- ✅ 基本初始化
- ✅ 初始状态检查
- ✅ 触发重连
- ✅ 重连失败处理
- ✅ 最大重试次数
- ✅ 停止重连
- ✅ 重置状态
- ✅ 禁用重连

## 编写新测试

### 基本测试结构

```cpp
#include <gtest/gtest.h>
#include "nodeagent/YourComponent.h"

TEST(YourComponentTest, TestName) {
    // Arrange: 设置测试环境
    YourComponent component;
    
    // Act: 执行被测试的操作
    bool result = component.doSomething();
    
    // Assert: 验证结果
    EXPECT_TRUE(result);
}
```

### 测试命名规范

- 测试套件名：`ComponentNameTest`（例如：`CommandHandlerTest`）
- 测试用例名：描述性名称，使用下划线分隔（例如：`HandleArmCommand`）

### 常用断言

```cpp
// 布尔断言
EXPECT_TRUE(condition);
EXPECT_FALSE(condition);

// 相等性断言
EXPECT_EQ(expected, actual);
EXPECT_NE(expected, actual);

// 数值比较
EXPECT_GT(a, b);  // a > b
EXPECT_LT(a, b);  // a < b
EXPECT_GE(a, b);  // a >= b
EXPECT_LE(a, b);  // a <= b

// 字符串断言
EXPECT_STREQ(str1, str2);
EXPECT_STRNE(str1, str2);
```

### 测试 Fixture

```cpp
class YourComponentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试前的设置
    }
    
    void TearDown() override {
        // 每个测试后的清理
    }
    
    YourComponent component_;
};

TEST_F(YourComponentTest, TestName) {
    // 可以使用 component_
}
```

## 测试最佳实践

1. **独立性**：每个测试应该独立，不依赖其他测试的执行顺序
2. **可重复性**：测试应该可以重复运行，结果一致
3. **快速执行**：单元测试应该快速执行（毫秒级）
4. **清晰命名**：测试名称应该清楚地描述测试的内容
5. **单一职责**：每个测试只测试一个功能点
6. **错误处理**：测试正常路径和错误路径
7. **边界条件**：测试边界条件和极端情况

## 测试覆盖率

目标：**测试覆盖率 > 80%**

当前覆盖的组件：
- ✅ CommandHandler
- ✅ MissionHandler
- ✅ MessageAckManager
- ✅ MultiUavManager
- ✅ Logger
- ✅ ErrorStatistics
- ✅ ReconnectManager

## CI/CD 集成

### GitHub Actions 示例

```yaml
name: Unit Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Build tests
        run: |
          cd NodeAgent/build
          cmake .. -DNODEAGENT_BUILD_TESTS=ON
          cmake --build . --target nodeagent_unit_tests
      - name: Run tests
        run: |
          cd NodeAgent/build
          ./nodeagent_unit_tests
```

## 故障排除

### 测试编译失败

1. 确保已启用测试：`-DNODEAGENT_BUILD_TESTS=ON`
2. 检查 Google Test 是否正确下载和编译
3. 检查所有依赖项是否正确链接

### 测试运行失败

1. 检查测试输出中的错误信息
2. 使用 `--gtest_filter` 运行单个测试进行调试
3. 使用 `--gtest_repeat` 重复运行测试以检测间歇性故障

### 内存泄漏检测

```bash
# 使用 Valgrind（如果可用）
valgrind --leak-check=full ./nodeagent_unit_tests
```

## 相关文档

- [Google Test 文档](https://google.github.io/googletest/)
- [CMake Testing](https://cmake.org/cmake/help/latest/manual/ctest.1.html)
- `ERROR_HANDLING_GUIDE.md`：错误处理系统使用指南
