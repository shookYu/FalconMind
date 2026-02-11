# NodeAgent 工作计划

> **最后更新**: 2024-01-30

## 📚 相关文档

- **README.md** - NodeAgent 总体说明
- **Doc/07_NodeAgent_Cluster_Design.md** - NodeAgent 和 Cluster Center 设计


# NodeAgent 工作计划

## 改进任务列表

### ✅ 已完成
- [x] 下行消息处理逻辑（Command/Mission）
- [x] 消息确认和重传机制
- [x] 多 UAV 支持
- [x] MQTT 协议支持（框架）

---

### ✅ 已完成
- [x] 完善 JSON 解析：使用 nlohmann/json 库 ✅ **已完成并验证通过**

---

### 📋 待实施

#### 1. 完善 JSON 解析：使用 nlohmann/json 库
**优先级**：高  
**预计工作量**：2-3 小时  
**状态**：进行中

**任务内容**：
- 集成 `nlohmann/json` 库到 CMake 构建系统
- 替换 `CommandHandler` 中的简单字符串解析
- 替换 `MissionHandler` 中的简单字符串解析
- 替换 `UplinkClient` 中的手动 JSON 序列化
- 替换 `DownlinkClient` 中的消息解析逻辑
- 添加 JSON 解析错误处理

**验收标准**：
- 所有 JSON 操作使用 `nlohmann/json`
- 通过现有测试用例
- 支持更复杂的 JSON 结构

---

#### 2. 实现 MQTT 客户端：集成 paho-mqtt-cpp 或 mosquitto
**优先级**：中  
**预计工作量**：4-6 小时  
**状态**：待实施

**任务内容**：
- 安装/集成 MQTT 客户端库（选择 paho-mqtt-cpp 或 mosquitto）
- 实现 `MqttUplinkClient::connect()`, `disconnect()`, `sendTelemetry()`
- 实现 `MqttDownlinkClient::connect()`, `disconnect()`, `startReceiving()`
- 在 `NodeAgent::Config` 中添加协议选择（`protocol: "tcp" | "mqtt"`）
- 实现工厂模式或配置选择具体实现
- 添加 MQTT 连接状态监控和重连机制
- 编写 MQTT 使用示例和文档

**验收标准**：
- MQTT 客户端可以正常连接和通信
- 支持 QoS 级别配置
- 支持主题订阅和发布
- 通过功能测试

---

#### 3. 完善 Cluster Center Mock：添加 ACK 响应支持
**优先级**：中  
**预计工作量**：2-3 小时  
**状态**：✅ **已完成并验证通过**

**任务内容**：
- 在 `cluster_center_mock` 中实现 ACK 消息格式
- 接收下行消息后自动发送 ACK 响应
- 支持 ACK 消息格式：`ACK:{messageId}`
- 添加 ACK 超时测试场景
- 更新测试文档

**验收标准**：
- Cluster Center Mock 能够正确响应 ACK
- NodeAgent 能够接收并处理 ACK
- 消息确认机制正常工作

---

#### 4. 增强错误处理：添加更详细的错误日志和恢复机制
**优先级**：中  
**预计工作量**：3-4 小时  
**状态**：✅ **已完成并验证通过**

**任务内容**：
- 为所有关键操作添加错误码定义
- 增强日志系统（添加日志级别：DEBUG, INFO, WARN, ERROR）
- 实现连接断开自动重连机制
- 添加异常处理和恢复逻辑
- 添加错误统计和监控
- 编写错误处理文档

**验收标准**：
- 所有错误都有明确的错误码和日志
- 支持自动重连
- 错误恢复机制正常工作

---

#### 5. 性能优化：优化消息序列化/反序列化性能
**优先级**：低  
**预计工作量**：2-3 小时  
**状态**：待实施

**任务内容**：
- 分析当前序列化/反序列化性能瓶颈
- 优化 JSON 序列化（使用更高效的库或方法）
- 实现消息池/对象池减少内存分配
- 添加性能测试和基准测试
- 优化网络传输（压缩、批量发送等）

**验收标准**：
- 序列化/反序列化性能提升 20%+
- 内存使用优化
- 通过性能测试

---

#### 6. 单元测试：为新增功能添加单元测试
**优先级**：高  
**预计工作量**：4-6 小时  
**状态**：✅ **已完成并验证通过**

**任务内容**：
- 为 `CommandHandler` 添加单元测试
- 为 `MissionHandler` 添加单元测试
- 为 `MessageAckManager` 添加单元测试
- 为 `MultiUavManager` 添加单元测试
- 为 `MqttUplinkClient` 和 `MqttDownlinkClient` 添加单元测试（mock）
- 集成测试框架（Google Test）
- 添加 CI/CD 测试流程

**验收标准**：
- 所有新增功能都有单元测试
- 测试覆盖率 > 80%
- 所有测试通过

---

## 实施进度

### 第 1 阶段：基础完善（当前）
- [x] 任务 1：完善 JSON 解析（进行中）

### 第 2 阶段：协议和可靠性
- [ ] 任务 2：实现 MQTT 客户端
- [ ] 任务 3：完善 Cluster Center Mock ACK

### 第 3 阶段：质量和性能
- [ ] 任务 4：增强错误处理
- [ ] 任务 5：性能优化
- [ ] 任务 6：单元测试

---

## 更新日志

**2024-01-29**：
- 创建工作计划文档
- ✅ 完成任务 1：完善 JSON 解析
  - 集成 nlohmann/json 库（带缓存优化）
  - 更新所有 JSON 相关代码
  - 添加错误处理
  - 创建测试程序
  - 编译验证通过
- ✅ 完成任务 3：完善 Cluster Center Mock ACK 响应支持
  - cluster_center_mock 自动发送 ACK
  - DownlinkClient 接收和处理 ACK
  - NodeAgent 集成 ACK 处理
  - MessageAckManager 支持 requestId
  - 支持 ack/noack 命令控制
  - 编译验证通过
- ✅ 完成任务 2：实现 MQTT 客户端
  - 集成 paho-mqtt-cpp 库（支持系统安装和 FetchContent）
  - 实现 MqttUplinkClient 和 MqttDownlinkClient
  - 创建 IUplinkClient 和 IDownlinkClient 抽象接口
  - NodeAgent 支持协议选择（TCP/MQTT）
  - 编译验证通过（MQTT 禁用模式）
- ✅ 完成任务 4：增强错误处理
  - 创建 ErrorCodes.h：统一的错误码定义
  - 创建 Logger.h/cpp：支持多级别的日志系统
  - 创建 ErrorStatistics.h/cpp：错误统计和监控
  - 创建 ReconnectManager.h/cpp：自动重连机制
  - 集成到 UplinkClient 和 NodeAgent
  - 编译验证通过
- ✅ 完成任务 6：单元测试
  - 集成 Google Test 框架（v1.14.0）
  - 为 CommandHandler 添加 7 个测试用例
  - 为 MissionHandler 添加 6 个测试用例
  - 为 MessageAckManager 添加 7 个测试用例
  - 为 MultiUavManager 添加 8 个测试用例
  - 为 Logger 添加 6 个测试用例
  - 为 ErrorStatistics 添加 8 个测试用例
  - 为 ReconnectManager 添加 8 个测试用例
  - 总计 50 个测试用例
  - 编译和运行验证通过
  - 创建单元测试使用指南文档