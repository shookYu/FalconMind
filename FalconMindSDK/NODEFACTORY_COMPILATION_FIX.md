# NodeFactory 编译错误修复说明

## 问题描述

在编译 `NodeFactory.cpp` 时遇到以下错误：

1. `FlightStateSourceNode` 需要 `FlightConnectionService&` 参数，但 `make_shared` 试图用无参构造函数创建
2. `FlightCommandSinkNode` 同样需要 `FlightConnectionService&` 参数
3. `CameraSourceNode` 需要 `VideoSourceConfig&` 参数

## 错误信息

```
error: no matching function for call to 'falconmind::sdk::flight::FlightStateSourceNode::FlightStateSourceNode()'
note: candidate expects 1 argument, 0 provided
```

## 修复方案

在 `NodeFactory::initializeDefaultTypes()` 中，为需要依赖的节点创建默认依赖对象：

1. **FlightStateSourceNode 和 FlightCommandSinkNode**：
   - 使用静态 `shared_ptr<FlightConnectionService>` 创建默认服务实例
   - 通过引用传递给节点构造函数

2. **CameraSourceNode**：
   - 使用静态 `VideoSourceConfig` 对象创建默认配置
   - 设置默认值（sensorId="default_camera", device="/dev/video0"）

## 修复后的代码

```cpp
// 注册飞行状态源节点（需要FlightConnectionService）
registerNodeType("flight_state_source",
    [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
        static std::shared_ptr<flight::FlightConnectionService> default_flight_service = 
            std::make_shared<flight::FlightConnectionService>();
        return std::make_shared<flight::FlightStateSourceNode>(*default_flight_service);
    });

// 注册飞行命令接收节点（需要FlightConnectionService）
registerNodeType("flight_command_sink",
    [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
        static std::shared_ptr<flight::FlightConnectionService> default_flight_service = 
            std::make_shared<flight::FlightConnectionService>();
        return std::make_shared<flight::FlightCommandSinkNode>(*default_flight_service);
    });

// 注册相机源节点（需要VideoSourceConfig）
registerNodeType("camera_source",
    [](const std::string& node_id, const void* /*params*/) -> std::shared_ptr<Node> {
        static sensors::VideoSourceConfig default_config;
        default_config.sensorId = "default_camera";
        default_config.device = "/dev/video0";
        return std::make_shared<sensors::CameraSourceNode>(default_config);
    });
```

## 注意事项

1. **默认依赖对象**：使用静态变量确保依赖对象在整个程序生命周期内存在
2. **实际使用**：在实际使用时，应该通过依赖注入机制提供正确的依赖对象，而不是使用默认值
3. **配置**：对于 `CameraSourceNode`，应该通过 `configure()` 方法或参数配置来设置正确的配置

## 验证

编译成功，所有目标都正常构建：

```
[100%] Built target falconmind_sdk_core_tests
```

## 相关文件

- `FalconMindSDK/src/core/NodeFactory.cpp` - 修复后的实现
- `FalconMindSDK/include/falconmind/sdk/core/NodeFactory.h` - 接口定义
