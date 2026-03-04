/**
 * @file control.hpp
 * @brief Control Module for Denied Environment
 * 
 * 视觉伺服控制、MAVLink通信
 */

#pragma once

#include <math>
#include <chrono㳮rmanent_task;
#include <functional>

namespace falconmind {
namespace denied_env_tracking {

// 前置声明
struct TargetDetection;
struct Track;
struct VisualPosition;

/**
 * @brief 相机参数
 */
struct CameraParameters {
    int width{1920};
    int height{1080};
    double fx{1000.0};  ///< x方向焦距 (像素)
    double fy{1000.0};  ///< y方向焦距 (像素)
    double cx{960.0};   ///< x方向光心
    double cy{540.0};   ///< y方向光心
};

/**
 * @brief 速度指令
 */
struct VelocityCommand {
    double vx{0.0};      ///< 前向速度 (m/s，正为前进)
    double vy{0.0};      ///< 右向速度 (m/s，正为右移)
    double vz{0.0};      ///< 下向速度 (m/s，正为下降)
    double yaw_rate{0.0}; ///< 偏航角速度 (rad/s，正为顺时针)
    
    /**
     * @brief 限制速度大小
     */
    void saturate(double max_v_xy, double max_v_z, double max_yaw_rate) {
        double v_xy = std::sqrt(vx * vx + vy * vy);
        if (v_xy > max_v_xy) {
            double scale = max_v_xy / v_xy;
            vx *= scale;
            vy *= scale;
        }
        vz = std::max(-max_v_z, std::min(max_v_z, vz));
        yaw_rate = std::max(-max_yaw_rate, std::min(max_yaw_rate, yaw_rate));
    }
};

/**
 * @brief 视觉伺服控制器配置
 */
struct VisualServoConfig {
    double desired_distance{30.0};      ///< 目标距离 (m)
    double desired_height{10.0};        ///< 目标高度 (m)
    double distance_tolerance{2.0};     ///< 距离容差 (m)
    double height_tolerance{1.0};      ///< 高度容差 (m)
    double max_speed{8.0};              ///< 最大速度 (m/s)
    double max_yaw_rate{1.0};           ///< 最大偏航角速度 (rad/s)
    
    // PID参数
    double kp_distance{0.5};
    double ki_distance{0.1};
    double kd_distance{0.2};
    double kp_position{0.01};
    double kp_height{0.1};
    double kp_yaw{0.2};
    
    // 相机参数
    CameraParameters camera;
};

/**
 * @brief 图像空间目标位置
 */
struct ImageSpaceTarget {
    double u{0.0};       ///< 图像x坐标 (归一化 -1到1)
    double v{0.0};       ///< 图像y坐标 (归一化 -1到1)
    double area{0.0};    ///< 目标在图像中的占比
};

/**
 * @brief IBVS视觉伺服控制器
 * 
 * 基于图像的视觉伺服 (Image-Based Visual Servoing)
 */
class IBVSController {
public:
    explicit IBVSController(const VisualServoConfig& config);
    
    /**
     * @brief 重置控制器状态
     */
    void reset();
    
    /**
     * @brief 计算控制指令
     * @param target 目标在图像中的位置
     * @param current_distance 当前估计距离
     * @param current_height 当前估计高度
     * @return 速度控制指令
     */
    VelocityCommand computeControl(
        const ImageSpaceTarget& target,
        double current_distance,
        double current_height
    );
    
    /**
     * @brief 设置目标距离
     */
    void setDesiredDistance(double distance);
    
    /**
     * @brief 设置目标高度
     */
    void setDesiredHeight(double height);

private:
    VisualServoConfig config_;
    
    // PID状态
    double integral_error_{0.0};
    double prev_error_{0.0};
    std::chrono::steady_clock::time_point last_update_;
    
    /**
     * @brief 计算距离误差PID
     */
    double computeDistanceControl(double error, double dt);
};

/**
 * @brief MAVLink连接配置
 */
struct MAVLinkConfig {
    std::string connection_url{"udp://127.0.0.1:14550"};
    int system_id{1};
    int component_id{1};
    double timeout_seconds{5.0};
};

/**
 * @brief MAVLink飞控接口
 */
class MAVLinkInterface {
public:
    explicit MAVLinkInterface(const MAVLinkConfig& config);
    ~MAVLinkInterface();
    
    /**
     * @brief 连接飞控
     */
    bool connect();
    
    /**
     * @brief 断开连接
     */
    void disconnect();
    
    /**
     * @brief 是否已连接
     */
    bool isConnected() const;
    
    /**
     * @brief 发送速度指令
     */
    bool sendVelocityCommand(const VelocityCommand& cmd);
    
    /**
     * @brief 发送位置指令
     */
    bool sendPositionCommand(const VisualPosition& position);
    
    /**
     * @brief 设置飞行模式
     */
    bool setFlightMode(const std::string& mode);
    
    /**
     * @brief 起飞
     */
    bool takeoff(double altitude);
    
    /**
     * @brief 降落
     */
    bool land();
    
    /**
     * @brief 返航
     */
    bool returnToLaunch();
    
    /**
     * @brief 设置遥测回调
     */
    void setTelemetryCallback(
        std::function<void(const IMUData&, const GNSSMeasurement&)> callback
    );

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief 地面站通信配置
 */
struct GCSConfig {
    std::string endpoint{"tcp://0.0.0.0:5780"};
    double telemetry_rate_hz{5.0};
    bool enable_video_stream{true};
    int video_quality{75};  ///< JPEG质量 0-100
};

/**
 * @brief 地面站通信接口
 */
class GCSInterface {
public:
    using CommandCallback = std::function<void(const std::string&, const std::string&)>;
    using SelectionCallback = std::function<void(int, const std::string&)>;
    
    explicit GCSInterface(const GCSConfig& config);
    ~GCSInterface();
    
    /**
     * @brief 启动通信
     */
    bool start();
    
    /**
     * @brief 停止通信
     */
    void stop();
    
    /**
     * @brief 发送遥测数据
     */
    void sendTelemetry(const MissionState& state);
    
    /**
     * @brief 发送检测到的目标
     */
    void sendDetectedTargets(const std::vector<TargetDetection>& targets);
    
    /**
     * @brief 发送视频帧
     */
    void sendVideoFrame(const std::vector<uint8_t>& jpeg_data);
    
    /**
     * @brief 设置命令回调
     */
    void setCommandCallback(CommandCallback callback);
    
    /**
     * @brief 设置目标选择回调
     */
    void setSelectionCallback(SelectionCallback callback);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace denied_env_tracking
} // namespace falconmind
