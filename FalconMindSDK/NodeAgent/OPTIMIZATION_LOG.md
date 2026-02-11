# JSON 接收逻辑优化日志

> **最后更新**: 2024-01-30

## 📚 相关文档

- **README.md** - NodeAgent 总体说明
- **Doc/07_NodeAgent_Cluster_Design.md** - NodeAgent 和 Cluster Center 设计


# JSON 接收逻辑优化日志

## 优化时间
2024-01-29

## 问题描述

**原始问题**：
- JSON 消息被逐行打印，而不是作为完整消息一次性打印
- 输出格式混乱，难以阅读

**根本原因**：
1. JSON 序列化使用了多行格式（包含内部换行符 `\n`）
2. TCP 流式传输时，数据可能分片到达
3. 接收逻辑在遇到第一个 `\n` 时就打印，导致只打印了 JSON 的第一行

## 优化方案

### 方案 1：改进 JSON 序列化（采用）

**改动**：
- 将 `UplinkClient::serializeTelemetryToJson()` 改为单行格式（压缩 JSON）
- 移除所有内部换行符和多余空格
- 保持 JSON 结构完整，仅改变格式

**优点**：
- 简单直接，不需要复杂的接收逻辑
- 单行 JSON 更适合网络传输
- 接收端可以简单地通过换行符（消息分隔符）识别完整消息

**代码改动**：
```cpp
// 优化前：多行格式
oss << "{\n"
    << "  \"uav_id\": \"" << msg.uavId << "\",\n"
    << ...

// 优化后：单行格式
oss << "{\"uav_id\":\"" << msg.uavId << "\","
    << "\"timestamp_ns\":" << msg.timestampNs << ","
    << ...
```

### 方案 2：改进接收逻辑（辅助）

**改动**：
- 优化 `cluster_center_mock.cpp` 的接收逻辑
- 使用 `append(buffer, n)` 而不是字符串拼接，避免 `\0` 截断
- 通过换行符（消息分隔符）识别完整消息

**代码改动**：
```cpp
// 优化前：可能被 '\0' 截断
buffer[n] = '\0';
messageBuffer += buffer;

// 优化后：二进制方式接收
messageBuffer.append(buffer, n);
```

## 优化效果

### 优化前
```
[Cluster Center] Received Telemetry #1 from NodeAgent:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
{
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
[Cluster Center] Received Telemetry #2 from NodeAgent:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
  "uav_id": "uav0",
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
...
```

### 优化后
```
[Cluster Center] Received Telemetry #1 from NodeAgent:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
{"uav_id":"uav0","timestamp_ns":1769685253634414066,"position":{"lat":39.9042000,"lon":116.4074000,"alt":100.00},"attitude":{"roll":0.100,"pitch":0.200,"yaw":1.570},"velocity":{"vx":5.000,"vy":0.000,"vz":0.000},"battery":{"percent":85.0,"voltage_mv":12600},"gps":{"fix_type":3,"num_sat":12},"link_quality":95.0,"flight_mode":"OFFBOARD"}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## 测试验证

✅ **测试通过**：
- 3 条 Telemetry 消息全部成功接收
- 每条消息以完整 JSON 形式一次性打印
- 消息计数器正常工作
- 输出格式清晰易读

## 相关文件

- `src/UplinkClient.cpp`：JSON 序列化改为单行格式
- `demo/cluster_center_mock.cpp`：接收逻辑优化

## 后续建议

如果需要更易读的 JSON 输出，可以在 Cluster Center Mock 中添加 JSON 格式化功能：
- 使用 JSON 库（如 nlohmann/json）解析和格式化
- 或添加简单的缩进逻辑

但对于网络传输和日志记录，单行 JSON 格式已经足够。
