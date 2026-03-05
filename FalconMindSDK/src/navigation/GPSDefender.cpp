/**
 * @file GPSDefender.cpp
 * @brief GPS欺骗检测与防护系统实现
 */

#include <falconmind/sdk/navigation/GPSDefender.h>
#include <falconmind/sdk/core/Logger.h>
#include <math>
#include <algorithm>
#include <numeric>

namespace falconmind {
namespace sdk {
namespace navigation {

class GPSDefender::Impl {
public:
    GPSDefenderConfig config;
    
    // 历史数据缓冲区
    std::vector<GNSSMeasurement> gnss_history;
    std::vector<IMUMeasurement> imu_history;
    
    // 参考位置
    std::optional<VisualPosition> last_vins_position;
    std::optional<IMUMeasurement> last_imu;
    
    // 状态
    SpoofingAlertLevel current_level{SpoofingAlertLevel::NONE};
    std::optional<std::chrono::steady_clock::time_point> last_valid_gnss_time;
    int consecutive_anomaly_count{0};
    int total_spoofing_detected{0};
    
    // IMU积分速度
    std::array<double, 3> imu_velocity{{0.0, 0.0, 0.0}};
    
    Impl(const GPSDefenderConfig& cfg) : config(cfg) {}
    
    /**
     * @brief 计算伪距残差
     */
    double calculatePseudorangeResidual(const GNSSMeasurement& gnss) {
        if (gnss.pseudoranges.size() < 5) {
            return 0.0;  // 卫星数不足，无法RAIM
        }
        
        // 计算伪距平均值
        double mean_pr = std::accumulate(
            gnss.pseudoranges.begin(), 
            gnss.pseudoranges.end(), 
            0.0
        ) / gnss.pseudoranges.size();
        
        // 计算最大残差
        double max_residual = 0.0;
        for (double pr : gnss.pseudoranges) {
            double residual = std::abs(pr - mean_pr);
            max_residual = std::max(max_residual, residual);
        }
        
        return max_residual;
    }
    
    /**
     * @brief 从IMU积分估算速度
     */
    std::array<double, 3> integrateIMUVelocity(
        const std::chrono::steady_clock::time_point& target_time
    ) {
        if (imu_history.empty() || !last_imu) {
            return {{0.0, 0.0, 0.0}};
        }
        
        // 简化的IMU积分（实际使用更复杂的预积分算法）
        std::array<double, 3> velocity = imu_velocity;
        
        // 积分最近10个IMU样本
        size_t count = std::min(size_t(10), imu_history.size());
        for (size_t i = imu_history.size() - count; i < imu_history.size(); ++i) {
            const auto& imu = imu_history[i];
            velocity[0] += imu.accel[0] * 0.01;  // 假设100Hz
            velocity[1] += imu.accel[1] * 0.01;
            velocity[2] += imu.accel[2] * 0.01;
        }
        
        return velocity;
    }
    
    /**
     * @brief 计算两点的水平距离
     */
    double calculateHorizontalDistance(
        const GNSSMeasurement& a, 
        const GNSSMeasurement& b
    ) {
        // 简化的距离计算（实际使用Haversine公式）
        const double R = 6371000.0;  // 地球半径
        double lat_diff = (a.latitude - b.latitude) * M_PI / 180.0;
        double lon_diff = (a.longitude - b.longitude) * M_PI / 180.0;
        
        double a_val = std::sin(lat_diff/2) * std::sin(lat_diff/2) +
                  std::cos(a.latitude * M_PI / 180.0) * 
                  std::cos(b.latitude * M_PI / 180.0) *
                  std::sin(lon_diff/2) * std::sin(lon_diff/2);
        
        double c = 2 * std::atan2(std::sqrt(a_val), std::sqrt(1-a_val));
        return R * c;
    }
    
    /**
     * @brief 将GNSS位置转换为NED坐标
     */
    std::array<double, 3> gnssToNED(const GNSSMeasurement& gnss) {
        // 简化的转换（实际使用完整的坐标转换）
        return {gnss.velocity_north, gnss.velocity_east, gnss.velocity_down};
    }
};

GPSDefender::GPSDefender(const GPSDefenderConfig& config)
    : pImpl(std::make_unique<Impl>(config)) {
}

bool GPSDefender::initialize() {
    FALCONMIND_LOG_INFO("Initializing GPS Defender");
    FALCONMIND_LOG_INFO("  RAIM threshold: {}m", pImpl->config.raim_threshold);
    FALCONMIND_LOG_INFO("  Velocity diff threshold: {}m/s", pImpl->config.velocity_diff_threshold);
    FALCONMIND_LOG_INFO("  Position diff threshold: {}m", pImpl->config.position_diff_threshold);
    
    return Node::initialize();
}

SpoofingReport GPSDefender::processGNSS(const GNSSMeasurement& gnss) {
    SpoofingReport report;
    report.timestamp = std::chrono::steady_clock::now();
    
    // 执行各项检查
    bool raim_ok = checkRAIM(gnss);
    bool imu_ok = checkIMUConsistency(gnss);
    bool vins_ok = checkVINSConsistency(gnss);
    bool jump_ok = checkJumpDetection(gnss);
    
    report.details.raim_check_passed = raim_ok;
    report.details.imu_consistency_passed = imu_ok;
    report.details.vins_consistency_passed = vins_ok;
    
    // 综合判断
    int failure_count = 0;
    std::vector<std::string> failure_reasons;
    
    if (!raim_ok) {
        failure_count++;
        failure_reasons.push_back("RAIM residual exceeded threshold");
    }
    if (!imu_ok) {
        failure_count++;
        failure_reasons.push_back("IMU velocity inconsistency");
    }
    if (!vins_ok) {
        failure_count++;
        failure_reasons.push_back("VINS position mismatch");
    }
    if (!jump_ok) {
        failure_count++;
        failure_reasons.push_back("Unrealistic position/velocity jump");
    }
    
    // 确定警报级别
    if (failure_count == 0) {
        report.level = SpoofingAlertLevel::NONE;
        report.confidence = 0.0;
        report.reason = "All checks passed";
        report.recommended_action = "Continue normal operation";
        pImpl->consecutive_anomaly_count = std::max(0, pImpl->consecutive_anomaly_count - 1);
        pImpl->last_valid_gnss_time = gnss.timestamp;
    } else {
        pImpl->consecutive_anomaly_count++;
        
        // 构建原因字符串
        report.reason = failure_reasons[0];
        for (size_t i = 1; i < failure_reasons.size(); ++i) {
            report.reason += "; " + failure_reasons[i];
        }
        
        // 计算置信度
        report.confidence = std::min(1.0, 
            (failure_count / 4.0) * 
            (pImpl->consecutive_anomaly_count / static_cast<double>(
                pImpl->config.consecutive_anomaly_threshold)));
        
        // 确定级别
        if (pImpl->consecutive_anomaly_count >= pImpl->config.consecutive_anomaly_threshold * 2) {
            report.level = SpoofingAlertLevel::CRITICAL;
            report.recommended_action = "IMMEDIATE: Reject all GNSS data, switch to VINS-only mode, alert operator";
            pImpl->total_spoofing_detected++;
        } else if (pImpl->consecutive_anomaly_count >= pImpl->config.consecutive_anomaly_threshold) {
            report.level = SpoofingAlertLevel::DETECTED;
            report.recommended_action = "Reject GNSS: Use VINS fusion with low GNSS weight";
            pImpl->total_spoofing_detected++;
        } else {
            report.level = SpoofingAlertLevel::SUSPECTED;
            report.recommended_action = "DEGRADED MODE: Reduce GNSS weight, increase monitoring";
        }
    }
    
    pImpl->current_level = report.level;
    
    // 保存历史
    pImpl->gnss_history.push_back(gnss);
    if (pImpl->gnss_history.size() > static_cast<size_t>(pImpl->config.detection_window_size)) {
        pImpl->gnss_history.erase(pImpl->gnss_history.begin());
    }
    
    // 日志
    if (report.level != SpoofingAlertLevel::NONE) {
        FALCONMIND_LOG_WARN("GPS Spoofing Alert [{}]: {}", 
            static_cast<int>(report.level), report.reason);
    }
    
    return report;
}

bool GPSDefender::checkRAIM(const GNSSMeasurement& gnss) {
    if (gnss.num_satellites < 5) {
        // 卫星数不足，无法RAIM，但不算失败
        return true;
    }
    
    double max_residual = pImpl->calculatePseudorangeResidual(gnss);
    pImpl->imd.max_pseudorange_residual = max_residual;
    
    return max_residual < pImpl->config.raim_threshold;
}

bool GPSDefender::checkIMUConsistency(const GNSSMeasurement& gnss) {
    if (pImpl->imu_history.size() < 10 || !pImpl->last_imu) {
        return true;  // IMU数据不足，跳过检查
    }
    
    // 计算GNSS速度
    std::array<double, 3> gnss_velocity = {
        gnss.velocity_north,
        gnss.velocity_east,
        gnss.velocity_down
    };
    
    // IMU积分估算
    auto imu_velocity = pImpl->integrateIMUVelocity(gnss.timestamp);
    
    // 计算速度差
    double velocity_diff = 0.0;
    for (int i = 0; i < 3; ++i) {
        velocity_diff += std::pow(gnss_velocity[i] - imu_velocity[i], 2);
    }
    velocity_diff = std::sqrt(velocity_diff);
    pImpl->imd.velocity_difference = velocity_diff;
    
    return velocity_diff < pImpl->config.velocity_diff_threshold;
}

bool GPSDefender::checkVINSConsistency(const GNSSMeasurement& gnss) {
    if (!pImpl->last_vins_position) {
        return true;  // 无VINS数据，跳过
    }
    
    // 简化的位置比较（实际需要完整的坐标转换）
    // 这里比较高度作为示例
    double gnss_alt = gnss.altitude;
    double vins_alt = -pImpl->last_vins_position->down;
    
    double alt_diff = std::abs(gnss_alt - vins_alt);
    pImpl->imd.position_difference = alt_diff;
    
    return alt_diff < pImpl->config.position_diff_threshold;
}

bool GPSDefender::checkJumpDetection(const GNSSMeasurement& gnss) {
    if (pImpl->gnss_history.empty()) {
        return true;  // 第一帧，无法判断
    }
    
    const auto& last_gnss = pImpl->gnss_history.back();
    
    // 时间差
    double dt = std::chrono::duration_cast<std::chrono::milliseconds>(
        gnss.timestamp - last_gnss.timestamp).count() / 1000.0;
    
    if (dt <= 0 || dt > 2.0) {
        return true;  // 时间差异常，跳过
    }
    
    // 位置跳变
    double position_change = pImpl->calculateHorizontalDistance(last_gnss, gnss);
    double implied_velocity = position_change / dt;
    
    // 如果速度超过50m/s（180km/h），认为不现实
    if (implied_velocity > 50.0) {
        FALCONMIND_LOG_ERROR("Unrealistic position jump: {}m in {}s ({}m/s)",
            position_change, dt, implied_velocity);
        return false;
    }
    
    // 卫星数跳变
    int sat_change = std::abs(gnss.num_satellites - last_gnss.num_satellites);
    if (sat_change > pImpl->config.satellite_jump_threshold) {
        FALCONMIND_LOG_WARN("Sudden satellite count change: {} -> {}",
            last_gnss.num_satellites, gnss.num_satellites);
        return false;
    }
    
    // DOP跳变
    double hdop_change = std::abs(gnss.hdop - last_gnss.hdop);
    if (hdop_change > pImpl->config.dop_jump_threshold) {
        FALCONMIND_LOG_WARN("Sudden DOP change: {} -> {}",
            last_gnss.hdop, gnss.hdop);
        return false;
    }
    
    return true;
}

void GPSDefender::processIMU(const IMUMeasurement& imu) {
    pImpl->imu_history.push_back(imu);
    pImpl->last_imu = imu;
    
    // 限制历史大小
    if (pImpl->imu_history.size() > 100) {
        pImpl->imu_history.erase(pImpl->imu_history.begin());
    }
}

void GPSDefender::updateVINSPosition(const VisualPosition& position) {
    pImpl->last_vins_position = position;
}

SpoofingAlertLevel GPSDefender::getAlertLevel() const {
    return pImpl->current_level;
}

std::optional<std::chrono::steady_clock::time_point> 
GPSDefender::getLastValidGNSSTime() const {
    return pImpl->last_valid_gnss_time;
}

bool GPSDefender::isGNSSReliable() const {
    return pImpl->current_level == SpoofingAlertLevel::NONE ||
           pImpl->current_level == SpoofingAlertLevel::SUSPECTED;
}

void GPSDefender::reset() {
    pImpl->gnss_history.clear();
    pImpl->imu_history.clear();
    pImpl->last_vins_position.reset();
    pImpl->last_imu.reset();
    pImpl->current_level = SpoofingAlertLevel::NONE;
    pImpl->last_valid_gnss_time.reset();
    pImpl->consecutive_anomaly_count = 0;
    pImpl->imu_velocity = {{0.0, 0.0, 0.0}};
    
    FALCONMIND_LOG_INFO("GPS Defender reset");
}

std::shared_ptr<GPSDefender> createGPSDefender(const GPSDefenderConfig& config) {
    return std::make_shared<GPSDefender>(config);
}

std::shared_ptr<GPSDefender> createStrictGPSDefender() {
    GPSDefenderConfig config;
    config.raim_threshold = 3.0;  // 更严格
    config.velocity_diff_threshold = 2.0;
    config.position_diff_threshold = 5.0;
    config.consecutive_anomaly_threshold = 2;
    return createGPSDefender(config);
}

} // namespace navigation
} // namespace sdk
} // namespace falconmind
