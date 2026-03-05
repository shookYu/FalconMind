/**
 * @file GPSDefender.h
 * @brief GPS欺骗检测与防护系统
 * 
 * 提供多源融合的GPS欺骗检测能力：
 * - RAIM (Receiver Autonomous Integrity Monitoring) 一致性检查
 * - IMU速度一致性验证
 * - VINS位置交叉验证
 * - 自动导航源切换
 * 
 * @author FalconMind SDK Team
 * @version 1.0.0
 * @date 2026-03-04
 */

#pragma once

#include <memory>
#include <vector>
#include <chrono>
#include <array>
#include <string>
#include <optional>
#include <falconmind/sdk/core/Node.h>
#include <falconmind/sdk/sensors/SensorTypes.h>

namespace falconmind {
namespace sdk {
namespace navigation {

/**
 * @brief 欺骗警报级别
 */
enum class SpoofingAlertLevel {
    NONE = 0,           /// 无异常
    SUSPECTED = 1,      /// 可疑（需持续监控）
    DETECTED = 2,       /// 确认欺骗（拒绝GNSS）
    CRITICAL = 3        /// 严重欺骗（立即告警）
};

/**
 * @brief GNSS测量数据
 */
struct GNSSMeasurement {
    std::chrono::steady_clock::time_point timestamp;
    double latitude{0.0};           /// 纬度 (度)
    double longitude{0.0};          /// 经度 (度)
    double altitude{0.0};           /// 高度 (m)
    double velocity_north{0.0};     /// 北向速度 (m/s)
    double velocity_east{0.0};      /// 东向速度 (m/s)
    double velocity_down{0.0};      /// 下向速度 (m/s)
    int num_satellites{0};          /// 卫星数量
    double hdop{99.9};              /// 水平精度因子
    double vdop{99.9};              /// 垂直精度因子
    std::vector<double> pseudoranges; /// 伪距测量值
};

/**
 * @brief IMU测量数据
 */
struct IMUMeasurement {
    std::chrono::steady_clock::time_point timestamp;
    std::array<double, 3> accel{{0.0, 0.0, 0.0}};  /// 加速度 (m/s^2)
    std::array<double, 3> gyro{{0.0, 0.0, 0.0}};    /// 角速度 (rad/s)
};

/**
 * @brief 视觉位置（VINS输出）
 */
struct VisualPosition {
    std::chrono::steady_clock::time_point timestamp;
    double north{0.0};              /// 北向位置 (m)
    double east{0.0};               /// 东向位置 (m)
    double down{0.0};               /// 下向位置 (m)
    double confidence{1.0};         /// 置信度 0-1
};

/**
 * @brief 欺骗检测报告
 */
struct SpoofingReport {
    SpoofingAlertLevel level{SpoofingAlertLevel::NONE};
    double confidence{0.0};         /// 置信度 0-1
    std::string reason;             /// 检测原因
    std::string recommended_action; /// 推荐操作
    std::chrono::steady_clock::time_point timestamp;
    
    /// 详细检测信息
    struct Details {
        bool raim_check_passed{true};
        bool imu_consistency_passed{true};
        bool vins_consistency_passed{true};
        double max_pseudorange_residual{0.0};
        double velocity_difference{0.0};
        double position_difference{0.0};
    } details;
};

/**
 * @brief GPS防护配置
 */
struct GPSDefenderConfig {
    /// RAIM检测阈值 (米)
    double raim_threshold{5.0};
    
    /// IMU速度差阈值 (m/s)
    double velocity_diff_threshold{3.0};
    
    /// VINS位置差阈值 (米)
    double position_diff_threshold{10.0};
    
    /// 卫星数跳变阈值
    int satellite_jump_threshold{3};
    
    /// DOP跳变阈值
    double dop_jump_threshold{2.0};
    
    /// 检测窗口大小（用于滑动平均）
    int detection_window_size{5};
    
    /// 触发DETECTED所需的连续异常次数
    int consecutive_anomaly_threshold{3};
};

/**
 * @brief GPS欺骗防护器
 * 
 * 多源融合GPS欺骗检测系统，集成RAIM、IMU一致性、VINS交叉验证
 * 三种检测算法，提供可靠的GPS欺骗识别能力。
 * 
 * 使用示例：
 * @code
 * GPSDefender defender(config);
 * 
 * // 处理GNSS数据
 * auto report = defender.processGNSS(gnss_data);
 * if (report.level == SpoofingAlertLevel::DETECTED) {
 *     // 切换到VINS导航
 *     navigation_source = NavigationSource::VINS_ONLY;
 * }
 * 
 * // 更新参考源
 * defender.updateVINSPosition(vins_position);
 * defender.updateIMU(imu_data);
 * @endcode
 */
class GPSDefender : public core::Node {
public:
    /**
     * @brief 构造函数
     * @param config 防护配置
     */
    explicit GPSDefender(const GPSDefenderConfig& config = GPSDefenderConfig{});
    
    ~GPSDefender() override = default;
    
    /**
     * @brief 初始化节点
     * @return 是否成功
     */
    bool initialize() override;
    
    /**
     * @brief 处理GNSS测量数据
     * 
     * 执行完整的欺骗检测流程：
     * 1. RAIM一致性检查
     * 2. IMU速度一致性验证
     * 3. VINS位置交叉验证
     * 4. 历史数据异常检测
     * 
     * @param gnss GNSS测量数据
     * @return 检测报告
     */
    SpoofingReport processGNSS(const GNSSMeasurement& gnss);
    
    /**
     * @brief 处理IMU数据（用于一致性验证）
     * @param imu IMU测量数据
     */
    void processIMU(const IMUMeasurement& imu);
    
    /**
     * @brief 更新VINS位置（用于交叉验证）
     * @param position VINS视觉位置
     */
    void updateVINSPosition(const VisualPosition& position);
    
    /**
     * @brief 获取当前警报级别
     */
    SpoofingAlertLevel getAlertLevel() const;
    
    /**
     * @brief 获取最后有效GNSS时间
     */
    std::optional<std::chrono::steady_clock::time_point> getLastValidGNSSTime() const;
    
    /**
     * @brief 是否信任当前GNSS数据
     */
    bool isGNSSReliable() const;
    
    /**
     * @brief 重置检测状态
     */
    void reset();

protected:
    /**
     * @brief RAIM一致性检查
     * @param gnss GNSS数据
     * @return 是否通过检查
     */
    bool checkRAIM(const GNSSMeasurement& gnss);
    
    /**
     * @brief IMU一致性检查
     * @param gnss GNSS数据
     * @return 是否通过检查
     */
    bool checkIMUConsistency(const GNSSMeasurement& gnss);
    
    /**
     * @brief VINS交叉验证
     * @param gnss GNSS数据
     * @return 是否通过检查
     */
    bool checkVINSConsistency(const GNSSMeasurement& gnss);
    
    /**
     * @brief 跳变检测
     * @param gnss GNSS数据
     * @return 是否通过检查
     */
    bool checkJumpDetection(const GNSSMeasurement& gnss);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief 便捷函数：创建标准配置的GPS防护器
 */
std::shared_ptr<GPSDefender> createGPSDefender(const GPSDefenderConfig& config = GPSDefenderConfig{});

/**
 * @brief 便捷函数：创建严格模式GPS防护器（更敏感的检测）
 */
std::shared_ptr<GPSDefender> createStrictGPSDefender();

} // namespace navigation
} // namespace sdk
} // namespace falconmind
