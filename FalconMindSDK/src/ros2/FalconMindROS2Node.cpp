/**
 * @file FalconMindROS2Node.cpp
 * @brief ROS2 bridge implementation
 */

#include "falconmind/sdk/ros2/FalconMindROS2Node.h"

#ifdef FALCONMINDSDK_WITH_ROS2

#include <cv_bridge/cv_bridge.h>
#include <rclcpp/qos.hpp>

namespace falconmind {
namespace sdk {
namespace ros2 {

// ============================================================================
// Implementation
// ============================================================================

class FalconMindROS2Node::Impl {
public:
    // Publishers
    std::map<std::string, rclcpp::PublisherBase::SharedPtr> publishers;
    
    // Subscriptions
    std::map<std::string, rclcpp::SubscriptionBase::SharedPtr> subscriptions;
    
    // Timer for periodic publishing
    rclcpp::TimerBase::SharedPtr timer;
    
    // Callbacks
    std::function<void(const Command&)> commandCallback;
    std::function<void(const TwistMsg&)> velocityCallback;
    std::function<void(const PoseMsg&)> positionCallback;
    
    // Pipeline references
    std::weak_ptr<high_level::PerceptionPipeline> perceptionPipeline;
    std::weak_ptr<high_level::FlightPipeline> flightPipeline;
    std::weak_ptr<high_level::MissionPipeline> missionPipeline;
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

FalconMindROS2Node::FalconMindROS2Node(
    const std::string& node_name,
    const rclcpp::NodeOptions& options)
    : rclcpp::Node(node_name, options),
      impl_(std::make_unique<Impl>()) {
    
    RCLCPP_INFO(get_logger(), "FalconMindROS2Node initialized: %s", node_name.c_str());
}

FalconMindROS2Node::~FalconMindROS2Node() = default;

// ============================================================================
// Perception Pipeline Bridge
// ============================================================================

void FalconMindROS2Node::bridgePerceptionPipeline(
    std::shared_ptr<high_level::PerceptionPipeline> pipeline,
    const std::string& topic_prefix) {
    
    if (!pipeline) {
        RCLCPP_ERROR(get_logger(), "Cannot bridge null perception pipeline");
        return;
    }
    
    impl_->perceptionPipeline = pipeline;
    
    // Create publishers
    auto detection_pub = create_publisher<StringMsg>(
        topic_prefix + "/detections", 10);
    impl_->publishers[topic_prefix + "/detections"] = detection_pub;
    
    // Set up callback from pipeline
    pipeline->onDetection([this, topic_prefix](const auto& detections) {
        publishDetections(topic_prefix + "/detections", detections);
    });
    
    RCLCPP_INFO(get_logger(), "Bridged perception pipeline to topic: %s", topic_prefix.c_str());
}

void FalconMindROS2Node::publishDetections(
    const std::string& topic_name,
    const std::vector<high_level::Detection>& detections) {
    
    auto pub = std::dynamic_pointer_cast<rclcpp::Publisher<StringMsg>>(
        impl_->publishers[topic_name]);
    
    if (!pub) {
        return;
    }
    
    // Convert detections to JSON string
    std::stringstream json;
    json << "{\"detections\": [";
    for (size_t i = 0; i < detections.size(); ++i) {
        const auto& d = detections[i];
        if (i > 0) json << ",";
        json << "{\"id\":" << d.trackId 
             << ",\"class\":\"" << d.className << "\""
             << ",\"conf\":" << d.confidence
             << ",\"bbox\":[" << d.x << "," << d.y 
             << "," << d.width << "," << d.height << "]}";
    }
    json << "]}";
    
    StringMsg msg;
    msg.data = json.str();
    pub->publish(msg);
}

void FalconMindROS2Node::publishDetectionImage(
    const std::string& topic_name,
    const cv::Mat& image,
    const std::vector<high_level::Detection>& detections) {
    
    (void)topic_name;
    (void)image;
    (void)detections;
    
    // TODO: Implement with cv_bridge
    RCLCPP_DEBUG(get_logger(), "publishDetectionImage not yet implemented");
}

// ============================================================================
// Flight Pipeline Bridge
// ============================================================================

void FalconMindROS2Node::bridgeFlightPipeline(
    std::shared_ptr<high_level::FlightPipeline> flight,
    const std::string& topic_prefix) {
    
    if (!flight) {
        RCLCPP_ERROR(get_logger(), "Cannot bridge null flight pipeline");
        return;
    }
    
    impl_->flightPipeline = flight;
    
    // Create publishers with sensor QoS
    rclcpp::QoS qos(10);
    qos.reliable().transient_local();
    
    auto status_pub = create_publisher<StringMsg>(
        topic_prefix + "/status", qos);
    auto pose_pub = create_publisher<PoseMsg>(
        topic_prefix + "/pose", qos);
    auto odom_pub = create_publisher<OdometryMsg>(
        topic_prefix + "/odom", qos);
    auto battery_pub = create_publisher<StringMsg>(
        topic_prefix + "/battery", qos);
    
    impl_->publishers[topic_prefix + "/status"] = status_pub;
    impl_->publishers[topic_prefix + "/pose"] = pose_pub;
    impl_->publishers[topic_prefix + "/odom"] = odom_pub;
    impl_->publishers[topic_prefix + "/battery"] = battery_pub;
    
    // Set up status callback
    flight->onStatusUpdated([this, topic_prefix](const auto& status) {
        publishVehicleStatus(topic_prefix + "/status", status);
    });
    
    RCLCPP_INFO(get_logger(), "Bridged flight pipeline to topic: %s", topic_prefix.c_str());
}

void FalconMindROS2Node::publishVehicleStatus(
    const std::string& topic_name,
    const high_level::VehicleStatus& status) {
    
    auto pub = std::dynamic_pointer_cast<rclcpp::Publisher<StringMsg>>(
        impl_->publishers[topic_name]);
    
    if (!pub) {
        return;
    }
    
    std::stringstream json;
    json << "{\"connected\":" << (status.isConnected ? "true" : "false")
         << ",\"armed\":" << (status.isArmed ? "true" : "false")
         << ",\"flying\":" << (status.isFlying ? "true" : "false")
         << ",\"battery\":" << status.batteryPercent
         << ",\"mode\":\"" << status.flightMode << "\"}";
    
    StringMsg msg;
    msg.data = json.str();
    pub->publish(msg);
}

void FalconMindROS2Node::publishPose(
    const std::string& topic_name,
    const PoseMsg& pose) {
    
    auto pub = std::dynamic_pointer_cast<rclcpp::Publisher<PoseMsg>>(
        impl_->publishers[topic_name]);
    
    if (pub) {
        pub->publish(pose);
    }
}

void FalconMindROS2Node::publishOdometry(
    const std::string& topic_name,
    const OdometryMsg& odometry) {
    
    auto pub = std::dynamic_pointer_cast<rclcpp::Publisher<OdometryMsg>>(
        impl_->publishers[topic_name]);
    
    if (pub) {
        pub->publish(odometry);
    }
}

void FalconMindROS2Node::publishIMU(
    const std::string& topic_name,
    const ImuMsg& imu) {
    
    auto pub = std::dynamic_pointer_cast<rclcpp::Publisher<ImuMsg>>(
        impl_->publishers[topic_name]);
    
    if (!pub) {
        pub = create_publisher<ImuMsg>(topic_name, 10);
        impl_->publishers[topic_name] = pub;
    }
    
    pub->publish(imu);
}

void FalconMindROS2Node::publishBattery(
    const std::string& topic_name,
    float percentage,
    float voltage,
    float current) {
    
    (void)current;
    
    auto pub = std::dynamic_pointer_cast<rclcpp::Publisher<StringMsg>>(
        impl_->publishers[topic_name]);
    
    if (!pub) {
        return;
    }
    
    std::stringstream json;
    json << "{\"percentage\":" << percentage 
         << ",\"voltage\":" << voltage << "}";
    
    StringMsg msg;
    msg.data = json.str();
    pub->publish(msg);
}

// ============================================================================
// Mission Bridge
// ============================================================================

void FalconMindROS2Node::bridgeMissionPipeline(
    std::shared_ptr<high_level::MissionPipeline> mission,
    const std::string& topic_prefix) {
    
    if (!mission) {
        RCLCPP_ERROR(get_logger(), "Cannot bridge null mission pipeline");
        return;
    }
    
    impl_->missionPipeline = mission;
    
    // Create publishers
    auto progress_pub = create_publisher<StringMsg>(
        topic_prefix + "/progress", 10);
    auto status_pub = create_publisher<StringMsg>(
        topic_prefix + "/status", 10);
    
    impl_->publishers[topic_prefix + "/progress"] = progress_pub;
    impl_->publishers[topic_prefix + "/status"] = status_pub;
    
    // Set up callbacks
    mission->onProgress([this, topic_prefix](const auto& progress) {
        publishMissionProgress(topic_prefix + "/progress", progress);
    });
    
    mission->onStatusChanged([this, topic_prefix](auto old_status, auto new_status) {
        (void)old_status;
        auto pub = std::dynamic_pointer_cast<rclcpp::Publisher<StringMsg>>(
            impl_->publishers[topic_prefix + "/status"]);
        if (pub) {
            StringMsg msg;
            msg.data = "{\"status\":" + std::to_string(static_cast<int>(new_status)) + "}";
            pub->publish(msg);
        }
    });
    
    RCLCPP_INFO(get_logger(), "Bridged mission pipeline to topic: %s", topic_prefix.c_str());
}

void FalconMindROS2Node::publishMissionProgress(
    const std::string& topic_name,
    const high_level::MissionProgress& progress) {
    
    auto pub = std::dynamic_pointer_cast<rclcpp::Publisher<StringMsg>>(
        impl_->publishers[topic_name]);
    
    if (!pub) {
        return;
    }
    
    std::stringstream json;
    json << "{\"current\":" << progress.currentWaypoint
         << ",\"total\":" << progress.totalWaypoints
         << ",\"battery\":" << progress.batteryPercent << "}";
    
    StringMsg msg;
    msg.data = json.str();
    pub->publish(msg);
}

// ============================================================================
// Command Subscription
// ============================================================================

void FalconMindROS2Node::subscribeCommand(
    const std::string& topic_name,
    std::function<void(const Command&)> callback) {
    
    impl_->commandCallback = callback;
    
    auto sub = create_subscription<StringMsg>(
        topic_name, 10,
        [this, callback](const StringMsg::SharedPtr msg) {
            Command cmd;
            cmd.command = msg->data;
            cmd.timestamp = std::chrono::nanoseconds(
                now().nanoseconds());
            callback(cmd);
        });
    
    impl_->subscriptions[topic_name] = sub;
    
    RCLCPP_INFO(get_logger(), "Subscribed to command topic: %s", topic_name.c_str());
}

void FalconMindROS2Node::subscribeVelocityCommand(
    const std::string& topic_name,
    std::function<void(const TwistMsg&)> callback) {
    
    impl_->velocityCallback = callback;
    
    auto sub = create_subscription<TwistMsg>(
        topic_name, 10,
        [callback](const TwistMsg::SharedPtr msg) {
            callback(*msg);
        });
    
    impl_->subscriptions[topic_name] = sub;
}

void FalconMindROS2Node::subscribePositionCommand(
    const std::string& topic_name,
    std::function<void(const PoseMsg&)> callback) {
    
    impl_->positionCallback = callback;
    
    auto sub = create_subscription<PoseMsg>(
        topic_name, 10,
        [callback](const PoseMsg::SharedPtr msg) {
            callback(*msg);
        });
    
    impl_->subscriptions[topic_name] = sub;
}

void FalconMindROS2Node::subscribeDetectionRequest(
    const std::string& topic_name,
    std::function<void(const std::string& target_class)> callback) {
    
    auto sub = create_subscription<StringMsg>(
        topic_name, 10,
        [callback](const StringMsg::SharedPtr msg) {
            callback(msg->data);
        });
    
    impl_->subscriptions[topic_name] = sub;
}

// ============================================================================
// Utility
// ============================================================================

rclcpp::Time FalconMindROS2Node::now() const {
    return rclcpp::Node::now();
}

PoseMsg FalconMindROS2Node::toROS2Pose(
    const GeoPoint& position, float roll, float pitch, float yaw) {
    
    PoseMsg pose;
    pose.position.x = position.longitude;
    pose.position.y = position.latitude;
    pose.position.z = position.altitude;
    
    // Convert Euler to quaternion
    float cy = std::cos(yaw * 0.5);
    float sy = std::sin(yaw * 0.5);
    float cp = std::cos(pitch * 0.5);
    float sp = std::sin(pitch * 0.5);
    float cr = std::cos(roll * 0.5);
    float sr = std::sin(roll * 0.5);
    
    pose.orientation.w = cr * cp * cy + sr * sp * sy;
    pose.orientation.x = sr * cp * cy - cr * sp * sy;
    pose.orientation.y = cr * sp * cy + sr * cp * sy;
    pose.orientation.z = cr * cp * sy - sr * sp * cy;
    
    return pose;
}

GeoPoint FalconMindROS2Node::fromROS2Pose(const PoseMsg& pose) {
    GeoPoint gp;
    gp.longitude = pose.position.x;
    gp.latitude = pose.position.y;
    gp.altitude = pose.position.z;
    return gp;
}

} // namespace ros2
} // namespace sdk
} // namespace falconmind

#endif // FALCONMINDSDK_WITH_ROS2
