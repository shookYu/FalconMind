/**
 * @file FalconMindROS2Node.h
 * @brief ROS2 桥接节点
 * 
 * 将 FalconMindSDK Pipeline 数据发布为 ROS2 话题，
 * 支持 ROS2 生态系统的集成。
 * 
 * @note 需要 ROS2 环境（Humble/Iron/Jazzy）
 * 
 * @example
 * @code
 * // 初始化 ROS2
 * rclcpp::init(argc, argv);
 * 
 * // 创建 FalconMind-ROS2 桥接节点
 * auto node = std::make_shared<FalconMindROS2Node>("falconmind_bridge");
 * 
 * // 将 PerceptionPipeline 发布到 ROS2
 * auto pipeline = PerceptionPipeline::create()...build().value();
 * node->bridgePerceptionPipeline(pipeline, "detections");
 * 
 * // 订阅 ROS2 命令
 * node->subscribeCommand("/falconmind/command", [](const Command& cmd) {
 *     // 处理 ROS2 发来的命令
 * });
 * 
 * rclcpp::spin(node);
 * rclcpp::shutdown();
 * @endcode
 */

#pragma once

// 仅在 ROS2 环境下编译
#ifdef FALCONMINDSDK_WITH_ROS2

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/twist.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <std_msgs/msg/string.hpp>

#include "falconmind/sdk/high_level/PerceptionPipeline.h"
#include "falconmind/sdk/high_level/FlightPipeline.h"
#include "falconmind/sdk/high_level/MissionPipeline.h"
#include <memory>
#include <string>
#include <functional>

namespace falconmind {
namespace sdk {
namespace ros2 {

// ROS2 消息类型别名
using PoseMsg = geometry_msgs::msg::Pose;
using TwistMsg = geometry_msgs::msg::Twist;
using ImageMsg = sensor_msgs::msg::Image;
using ImuMsg = sensor_msgs::msg::Imu;
using OdometryMsg = nav_msgs::msg::Odometry;
using StringMsg = std_msgs::msg::String;

/**
 * @brief ROS2 命令消息
 */
struct Command {
    std::string command;           ///< 命令名称
    std::vector<double> params;    ///< 参数列表
    std::string sender;            ///< 发送者
    std::chrono::nanoseconds timestamp;
};

/**
 * @brief 检测消息（自定义 ROS2 消息）
 */
struct DetectionMsg {
    int track_id;
    std::string class_name;
    float confidence;
    float x, y, width, height;     ///< 归一化 bbox
    float center_x, center_y;      ///< 归一化中心
    geometry_msgs::msg::Point world_position;  ///< 世界坐标（如果有）
};

/**
 * @brief FalconMind-ROS2 桥接节点
 * 
 * 将 FalconMindSDK 的数据流桥接到 ROS2 话题，
 * 实现与 ROS2 生态的无缝集成。
 */
class FalconMindROS2Node : public rclcpp::Node {
public:
    /**
     * @brief 构造函数
     * @param node_name ROS2 节点名称
     * @param options ROS2 节点选项
     */
    explicit FalconMindROS2Node(
        const std::string& node_name = "falconmind_bridge",
        const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
    
    /**
     * @brief 析构函数
     */
    ~FalconMindROS2Node() override;
    
    // ==================== Perception Pipeline Bridge ====================
    
    /**
     * @brief 桥接感知流水线
     * @param pipeline 感知流水线
     * @param topic_prefix 话题前缀（默认 "detections"）
     */
    void bridgePerceptionPipeline(
        std::shared_ptr<high_level::PerceptionPipeline> pipeline,
        const std::string& topic_prefix = "detections");
    
    /**
     * @brief 发布检测结果
     * @param topic_name 话题名称
     * @param detections 检测列表
     */
    void publishDetections(
        const std::string& topic_name,
        const std::vector<high_level::Detection>& detections);
    
    /**
     * @brief 发布检测图像（带 bbox 叠加）
     */
    void publishDetectionImage(
        const std::string& topic_name,
        const cv::Mat& image,
        const std::vector<high_level::Detection>& detections);
    
    // ==================== Flight Pipeline Bridge ====================
    
    /**
     * @brief 桥接飞控连接
     * @param flight 飞控流水线
     * @param topic_prefix 话题前缀（默认 "vehicle"）
     */
    void bridgeFlightPipeline(
        std::shared_ptr<high_level::FlightPipeline> flight,
        const std::string& topic_prefix = "vehicle");
    
    /**
     * @brief 发布飞行器状态
     */
    void publishVehicleStatus(
        const std::string& topic_name,
        const high_level::VehicleStatus& status);
    
    /**
     * @brief 发布位姿
     */
    void publishPose(
        const std::string& topic_name,
        const geometry_msgs::msg::Pose& pose);
    
    /**
     * @brief 发布里程计
     */
    void publishOdometry(
        const std::string& topic_name,
        const OdometryMsg& odometry);
    
    /**
     * @brief 发布 IMU 数据
     */
    void publishIMU(
        const std::string& topic_name,
        const ImuMsg& imu);
    
    /**
     * @brief 发布电池状态
     */
    void publishBattery(
        const std::string& topic_name,
        float percentage,
        float voltage,
        float current = 0.0f);
    
    // ==================== Mission Bridge ====================
    
    /**
     * @brief 桥接任务流水线
     */
    void bridgeMissionPipeline(
        std::shared_ptr<high_level::MissionPipeline> mission,
        const std::string& topic_prefix = "mission");
    
    /**
     * @brief 发布任务进度
     */
    void publishMissionProgress(
        const std::string& topic_name,
        const high_level::MissionProgress& progress);
    
    // ==================== Command Subscription ====================
    
    /**
     * @brief 订阅 ROS2 命令
     * @param topic_name 话题名称
     * @param callback 命令回调
     */
    void subscribeCommand(
        const std::string& topic_name,
        std::function<void(const Command&)> callback);
    
    /**
     * @brief 订阅速度指令
     */
    void subscribeVelocityCommand(
        const std::string& topic_name,
        std::function<void(const TwistMsg&)> callback);
    
    /**
     * @brief 订阅位置指令
     */
    void subscribePositionCommand(
        const std::string& topic_name,
        std::function<void(const PoseMsg&)> callback);
    
    /**
     * @brief 订阅目标检测请求
     */
    void subscribeDetectionRequest(
        const std::string& topic_name,
        std::function<void(const std::string& target_class)> callback);
    
    // ==================== Service ====================
    
    /**
     * @brief 添加 ROS2 服务
     * @param service_name 服务名称
     * @param handler 服务处理函数
     */
    template<typename RequestT, typename ResponseT>
    void addService(
        const std::string& service_name,
        std::function<void(const RequestT&, ResponseT&)> handler);
    
    // ==================== Utility ====================
    
    /**
     * @brief 获取当前时间戳
     */
    rclcpp::Time now() const;
    
    /**
     * @brief 转换为 ROS2 Pose 消息
     */
    static PoseMsg toROS2Pose(const GeoPoint& position, float roll, float pitch, float yaw);
    
    /**
     * @brief 从 ROS2 Pose 转换
     */
    static GeoPoint fromROS2Pose(const PoseMsg& pose);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief 便捷函数：创建并运行 ROS2 桥接
 */
inline void runROS2Bridge(
    std::shared_ptr<high_level::PerceptionPipeline> perception = nullptr,
    std::shared_ptr<high_level::FlightPipeline> flight = nullptr,
    const std::string& node_name = "falconmind_bridge") {
    
    rclcpp::init(0, nullptr);
    
    auto node = std::make_shared<FalconMindROS2Node>(node_name);
    
    if (perception) {
        node->bridgePerceptionPipeline(perception);
    }
    
    if (flight) {
        node->bridgeFlightPipeline(flight);
    }
    
    rclcpp::spin(node);
    rclcpp::shutdown();
}

} // namespace ros2
} // namespace sdk
} // namespace falconmind

#endif // FALCONMINDSDK_WITH_ROS2
