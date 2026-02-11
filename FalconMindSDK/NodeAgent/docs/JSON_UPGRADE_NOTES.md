# JSON 解析升级说明

> **最后更新**: 2024-01-30

## 📚 相关文档

- **../README.md** - NodeAgent 总体说明
- **Doc/07_NodeAgent_Cluster_Design.md** - NodeAgent 和 Cluster Center 设计


# JSON 解析升级说明

## 概述

已将 NodeAgent 中的所有 JSON 操作从简单字符串解析升级为使用 `nlohmann/json` 库。

## 升级内容

### 1. 集成 nlohmann/json

在 `CMakeLists.txt` 中使用 `FetchContent` 自动下载和集成：

```cmake
include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)
```

### 2. 更新的文件

- **CommandHandler.cpp**：使用 `nlohmann::json::parse()` 解析命令 JSON
- **MissionHandler.cpp**：使用 `nlohmann::json::parse()` 解析任务 JSON，支持嵌套参数
- **UplinkClient.cpp**：使用 `nlohmann::json` 序列化 Telemetry
- **DownlinkClient.cpp**：使用 `nlohmann::json::parse()` 解析下行消息，提取 uavId 和 requestId
- **MqttUplinkClient.cpp**：使用 `nlohmann::json` 序列化（与 TCP 版本一致）

### 3. 改进点

#### 更健壮的解析
- 自动处理 JSON 格式错误
- 支持嵌套 JSON 结构
- 类型安全的字段访问

#### 更好的错误处理
- 捕获 `nlohmann::json::parse_error`（JSON 格式错误）
- 捕获 `nlohmann::json::type_error`（类型不匹配）
- 提供详细的错误信息

#### 支持更复杂的结构
- 嵌套对象（如 `params.takeoffAlt`）
- 可选字段（使用 `value()` 或 `contains()` 检查）
- 数组和对象数组

### 4. 使用示例

#### 解析命令 JSON
```cpp
auto json = nlohmann::json::parse(jsonPayload);
std::string type = json["type"].get<std::string>();
double targetAlt = json.value("targetAlt", 0.0);  // 可选字段，默认值 0.0
```

#### 解析任务 JSON（支持嵌套参数）
```cpp
auto json = nlohmann::json::parse(jsonPayload);
std::string task = json["task"].get<std::string>();

if (json.contains("params") && json["params"].is_object()) {
    auto params = json["params"];
    double takeoffAlt = params.value("takeoffAlt", 10.0);
    int hoverDuration = params.value("hoverDuration", 5);
}
```

#### 序列化 Telemetry
```cpp
nlohmann::json json;
json["uav_id"] = msg.uavId;
json["position"]["lat"] = msg.lat;
json["position"]["lon"] = msg.lon;
// ...
std::string serialized = json.dump();  // 紧凑格式，单行
```

## 测试

运行 JSON 解析功能测试：

```bash
cd NodeAgent/build
cmake --build . --target test_json_parsing
./test_json_parsing
```

## 兼容性

- **向后兼容**：支持原有的 JSON 格式
- **扩展性**：支持更复杂的 JSON 结构
- **性能**：nlohmann/json 是 header-only 库，编译时优化，运行时性能优秀

## 注意事项

1. **CMake 配置**：首次运行 `cmake ..` 时会自动下载 nlohmann/json（需要网络连接）
2. **版本**：当前使用 v3.11.2，如需更新版本，修改 `CMakeLists.txt` 中的 `GIT_TAG`
3. **错误处理**：所有 JSON 操作都应包含 try-catch 块以处理解析错误

## 后续改进

- [ ] 添加 JSON Schema 验证（可选）
- [ ] 支持 JSON 格式化输出（用于调试）
- [ ] 性能基准测试
