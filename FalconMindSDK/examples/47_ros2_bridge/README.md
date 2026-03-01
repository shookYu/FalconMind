# FalconMindSDK 示例47：ROS2 桥接

## 概述

本示例演示如何使用 `FalconMindROS2Node` 将 FalconMindSDK 的数据发布到 ROS2 话题。

## 功能

- 将 PerceptionPipeline 检测结果发布到 ROS2
- 将 FlightPipeline 状态发布到 ROS2
- 订阅 ROS2 命令并控制无人机
- 支持 ROS2 Humble/Iron/Jazzy

## 编译运行

### 前提条件

```bash
# 安装 ROS2 (Humble)
sudo apt install ros-humble-desktop
source /opt/ros/humble/setup.bash
```

### 编译

```bash
cd FalconMindSDK
cd build
cmake .. -DFALCONMINDSDK_BUILD_ROS2=ON
make -j4
```

### 运行

```bash
# 终端1：启动 ROS2
source /opt/ros/humble/setup.bash

# 终端2：运行示例
source /opt/ros/humble/setup.bash
cd FalconMindSDK/examples/47_ros2_bridge/x86/build
./47_ros2_bridge_x86

# 终端3：查看 ROS2 话题
ros2 topic list
ros2 topic echo /detections
ros2 topic echo /vehicle/status
```

## 代码示例

```cpp
#include "falconmind/sdk/ros2/FalconMindROS2Node.h"
#include <rclcpp/rclcpp.hpp>

int main(int argc, char** argv) {
    // 初始化 ROS2
    rclcpp::init(argc, argv);
    
    // 创建 FalconMind-ROS2 桥接节点
    auto node = std::make_shared<falconmind::sdk::ros2::FalconMindROS2Node>("falconmind_bridge");
    
    // 创建感知流水线
    auto perception_result = falconmind::sdk::high_level::PerceptionPipeline::create()
        .withCamera(640, 480, 30)
        .withDetector("yolov8n.onnx")
        .build();
    
    if (perception_result) {
        auto pipeline = perception_result.value();
        
        // 桥接感知流水线到 ROS2
        node->bridgePerceptionPipeline(pipeline, "detections");
        
        // 启动流水线
        pipeline->start();
    }
    
    // 创建飞控连接
    auto flight_result = falconmind::sdk::high_level::FlightPipeline::create()
        .withConnection("udp://127.0.0.1:14550")
        .build();
    
    if (flight_result) {
        auto flight = flight_result.value();
        
        // 桥接飞控到 ROS2
        node->bridgeFlightPipeline(flight, "vehicle");
    }
    
    // 订阅 ROS2 命令
    node->subscribeCommand("/falconmind/command", [](const auto& cmd) {
        std::cout << "Received command: " << cmd.command << std::endl;
    });
    
    // 运行 ROS2 节点
    rclcpp::spin(node);
    rclcpp::shutdown();
    
    return 0;
}
```

## ROS2 话题

### 发布的话题

| 话题名 | 类型 | 说明 |
|--------|------|------|
| `/detections` | std_msgs/String | 检测结果（JSON格式） |
| `/vehicle/status` | std_msgs/String | 飞行器状态 |
| `/vehicle/pose` | geometry_msgs/Pose | 位姿 |
| `/vehicle/odom` | nav_msgs/Odometry | 里程计 |
| `/mission/progress` | std_msgs/String | 任务进度 |

### 订阅的话题

| 话题名 | 类型 | 说明 |
|--------|------|------|
| `/falconmind/command` | std_msgs/String | 命令 |
| `/cmd_vel` | geometry_msgs/Twist | 速度指令 |
| `/goal_pose` | geometry_msgs/Pose | 目标位置 |

## 与 ROS2 生态系统集成

### 与 RViz 集成

```bash
# 发布 TF 转换
ros2 run tf2_ros static_transform_publisher 0 0 0 0 0 0 map base_link

# 启动 RViz
rviz2
```

### 与 Nav2 集成

```bash
# 使用 Nav2 进行路径规划
ros2 launch nav2_bringup navigation_launch.py
```

### 记录数据

```bash
# 录制 ROS2 bag
ros2 bag record /detections /vehicle/status /vehicle/pose
```

## 要求

- ROS2 Humble/Iron/Jazzy
- FalconMindSDK 编译时启用 `-DFALCONMINDSDK_BUILD_ROS2=ON`

## 注意事项

- ROS2 桥接是可选功能，需要显式启用
- 需要 source ROS2 环境变量
- 支持 ROS2 QoS 配置
- 可以与其他 ROS2 节点无缝集成

## 参考

- [ROS2 文档](https://docs.ros.org/)
- [FalconMindSDK ROS2 API](Doc/API_ROS2.md)
