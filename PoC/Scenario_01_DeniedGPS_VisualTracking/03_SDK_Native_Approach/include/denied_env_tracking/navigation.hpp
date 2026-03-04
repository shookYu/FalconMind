/**
 * @file navigation.hpp
 * @brief Navigation Module for Denied Environment
 * 
 * VINS导航、GPS欺骗防护
 */

#pragma once

#include <math>
#include <array㳮rmanent_task;
#include <chrono>

namespace falconmind {
namespace denied_env_tracking {

/**
 * @brief 地理坐标位置 (WGS84)
 */
struct GeodeticPosition {
    double latitude{0.0};    ///< 纬度 (度)
    double longitude{0.0};   ///< 经度 (度)
    double altitude{0.0};    ///< 椭球高 (m)
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief 视觉定位位置 (NED坐标系，相对于起飞点)
 */
struct VisualPosition {
    double north{0.0};       ///< 北向 (m)
    double east{0.0};        ///< 东向 (m)
    double down{0.0};        ///< 下向 (m，负值为高度)
    double confidence{1.0};  ///< 置信度 0-1
    std::chrono::steady_clock::time_point timestamp;
    
    /**
     * @brief 计算水平距离
     */
    double horizontalDistance() const {
        return std::sqrt(north * north + east * east);
    }
    
    /**
     * @brief 获取高度 (正值)
     */
    double height() const {
        return -down;
    }
};

/**
 * @brief GNSS状态
 */
enum class GNSSStatus {
    HEALTHY,              ///< 正常
    DEGRADED,            ///< 降级
    SPOOFING_DETECTED,   ///< 检测到欺骗
    DENIED,              ///< 无信号
    FUSION_ONLY          ///< 仅使用融合定位
};

/**
 * @brief IMU数据
 */
struct IMUData {
    std::array<double, 3> accel{{0.0, 0.0, 0.0}};  ///< 加速度 (m/s^2)
    std::array<double, 3> gyro{{0.0, 0.0, 0.0}};   ///< 角速度 (rad/s)
    double temperature{25.0};
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief 相机帧
 */
struct CameraFrame {
    std::vector<uint8_t> image_data;
    int width{0};
    int height{0};
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief VINS初始化器
 * 
 * 视觉惯性导航系统初始化
 */
class VINSInitializer {
public:
    struct Config {
        double calibration_time{3.0};       ///< IMU校准时间 (s)
        int required_features{150};         ///< 所需特征点数量
        double min_parallax{10.0};          ///< 最小视差 (像素)
        double convergence_time{5.0};       ///< 收敛时间 (s)
    };
    
    explicit VINSInitializer(const Config& config);
    
    /**
     * @brief 开始初始化 (阻塞)
     * @return 是否成功
     */
    bool initialize();
    
    /**
     * @brief 开始初始化 (非阻塞)
     */
    void startAsync();
    
    /**
     * @brief 检查是否完成
     */
    bool isReady() const;
    
    /**
     * @brief 获取进度 0-1
     */
    double getProgress() const;
    
    /**
     * @brief 处理IMU数据
     */
    void processIMU(const IMUData& imu);
    
    /**
     * @brief 处理图像帧
     */
    void processImage(const CameraFrame& frame);
    
    /**
     * @brief 获取初始化后的位置
     */
    VisualPosition getInitialPosition() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief GNSS测量数据
 */
struct GNSSMeasurement {
    GeodeticPosition position;
    double velocity_north{0.0};
    double velocity_east{0.0};
    double velocity_down{0.0};
    int num_satellites{0};
    double hdop{99.9};
    double vdop{99.9};
    std::vector<double> pseudoranges;
};

/**
 * @brief 欺骗警报级别
 */
enum class SpoofingAlertLevel {
    NONE,
    SUSPECTED,
    DETECTED,
    CRITICAL
};

/**
 * @brief 欺骗检测报告
 */
struct SpoofingReport {
    SpoofingAlertLevel level{SpoofingAlertLevel::NONE};
    double confidence{0.0};
    std::string reason;
    std::string recommended_action;
    std::chrono::steady_clock::time_point timestamp;
};

/**
 * @brief GPS欺骗防护器
 */
class GPSDefender {
public:
    struct Config {
        double raim_threshold{5.0};              ///< RAIM阈值 (m)
        double velocity_diff_threshold{3.0};     ///< 速度差阈值 (m/s)
        double position_diff_threshold{10.0};    ///< 位置差阈值 (m)
        int satellite_jump_threshold{3};         ///< 卫星数跳变阈值
        double check_interval{1.0};              ///< 检查间隔 (s)
    };
    
    explicit GPSDefender(const Config& config);
    
    /**
     * @brief 处理GNSS数据
     * @return 检测报告
     */
    SpoofingReport processGNSS(const GNSSMeasurement& gnss);
    
    /**
     * @brief 处理IMU数据
     */
    void processIMU(const IMUData& imu);
    
    /**
     * @brief 更新视觉里程计位置
     */
    void updateVisualOdometry(const VisualPosition& position);
    
    /**
     * @brief 获取当前GNSS状态
     */
    GNSSStatus getStatus() const;
    
    /**
     * @brief 获取最后有效GNSS时间
     */
    std::chrono::steady_clock::time_point getLastValidGNSSTime() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief 定位融合器 (EKF)
 */
class PositionFusion {
public:
    /**
     * @brief 更新GNSS测量
     */
    void updateGNSS(const GNSSMeasurement& gnss, double confidence);
    
    /**
     * @brief 更新视觉位置
     */
    void updateVisual(const VisualPosition& visual);
    
    /**
     * @brief 预测 (IMU传播)
     */
    void predict(const IMUData& imu, double dt);
    
    /**
     * @brief 获取融合位置
     */
    VisualPosition getFusedPosition() const;
    
    /**
     * @brief 获取位置协方差
     */
    double getPositionCovariance() const;

private:
    VisualPosition fused_position_;
    double position_covariance_{1.0};
};

} // namespace denied_env_tracking
} // namespace falconmind
