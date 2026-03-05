/**
 * @file IBVSController.cpp
 * @brief Image-Based Visual Servoing 控制器实现
 */

#include <falconmind/sdk/control/IBVSController.h>
#include <falconmind/sdk/core/Logger.h>
#include <math>

namespace falconmind {
namespace sdk {
namespace control {

class IBVSController::Impl {
public:
    IBVSConfig config;
    
    // PID状态
    double integral_error{0.0};
    double prev_error{0.0};
    std::chrono::steady_clock::time_point last_update;
    
    // 统计
    int update_count{0};
    double total_error{0.0};
    
    explicit Impl(const IBVSConfig& cfg) : config(cfg) {
        last_update = std::chrono::steady_clock::now();
    }
    
    /**
     * @brief 计算时间差
     */
    double getDeltaTime() {
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_update).count() / 1000.0;
        last_update = now;
        return std::max(dt, 0.001);  // 最小1ms
    }
};

IBVSController::IBVSController(const IBVSConfig& config)
    : pImpl(std::make_unique<Impl>(config)) {
}

bool IBVSController::initialize() {
    FALCONMIND_LOG_INFO("Initializing IBVS Controller");
    FALCONMIND_LOG_INFO("  Desired distance: {}m", pImpl->config.desired_distance);
    FALCONMIND_LOG_INFO("  Desired height: {}m", pImpl->config.desired_height);
    FALCONMIND_LOG_INFO("  Max speed: {}m/s", pImpl->config.max_speed);
    FALCONMIND_LOG_INFO("  PID: Kp={}, Ki={}, Kd={}",
        pImpl->config.kp_distance,
        pImpl->config.ki_distance,
        pImpl->config.kd_distance);
    
    return Node::initialize();
}

VelocityCommand IBVSController::computeControl(
    const ImageSpaceTarget& target,
    double current_distance,
    double current_height
) {
    VelocityCommand cmd;
    cmd.timestamp = std::chrono::steady_clock::now();
    
    double dt = pImpl->getDeltaTime();
    const auto& cfg = pImpl->config;
    
    // ============ 距离控制 ============
    double distance_error = current_distance - cfg.desired_distance;
    
    // PID计算
    double vx = computeDistanceControl(distance_error, dt);
    
    // ============ 水平位置控制 ============
    // 图像水平误差 → 左右运动
    // 当目标在图像右侧(u>0)，需要向右移动(vy<0)
    double vy = -cfg.kp_position * target.u * current_distance;
    
    // ============ 高度控制 ============
    // 图像垂直误差 + 高度误差
    double height_error = current_height - cfg.desired_height;
    double vz = -cfg.kp_position * target.v * current_distance 
                - cfg.kp_height * height_error;
    
    // ============ 偏航控制 ============
    // 图像水平误差 → 偏航角速度
    // 保持机头指向目标
    double yaw_rate = -cfg.kp_yaw * target.u;
    
    // ============ 自适应增益 ============
    if (cfg.enable_adaptive_gain) {
        double gain = computeAdaptiveGain(current_distance);
        vx *= gain;
        vy *= gain;
        vz *= gain;
    }
    
    // ============ 饱和保护 ============
    cmd.vx = vx;
    cmd.vy = vy;
    cmd.vz = vz;
    cmd.yaw_rate = yaw_rate;
    cmd.saturate(cfg.max_speed, cfg.max_vertical_speed, cfg.max_yaw_rate);
    
    // 统计
    pImpl->update_count++;
    pImpl->total_error += std::abs(distance_error);
    
    return cmd;
}

double IBVSController::computeDistanceControl(double error, double dt) {
    const auto& cfg = pImpl->config;
    
    // 积分项
    pImpl->integral_error += error * dt;
    
    // 抗积分饱和
    double max_integral = 10.0 / cfg.ki_distance;
    pImpl->integral_error = std::max(-max_integral, 
        std::min(max_integral, pImpl->integral_error));
    
    // 微分项
    double derivative = (error - pImpl->prev_error) / dt;
    pImpl->prev_error = error;
    
    // PID输出
    double output = -(cfg.kp_distance * error + 
                      cfg.ki_distance * pImpl->integral_error +
                      cfg.kd_distance * derivative);
    
    return output;
}

double IBVSController::computeAdaptiveGain(double distance) const {
    if (!pImpl->config.enable_adaptive_gain) {
        return 1.0;
    }
    
    // 远距离时降低增益（避免超调）
    // 近距离时提高增益（快速接近）
    if (distance > pImpl->config.min_distance_for_full_speed) {
        return 1.0;
    } else {
        // 线性插值
        return 0.3 + 0.7 * (distance / pImpl->config.min_distance_for_full_speed);
    }
}

TrackingQuality IBVSController::computeQuality(
    double current_distance,
    double current_height,
    const ImageSpaceTarget& target
) const {
    TrackingQuality quality;
    const auto& cfg = pImpl->config;
    
    // 距离误差
    quality.distance_error = current_distance - cfg.desired_distance;
    quality.is_distance_good = std::abs(quality.distance_error) <= cfg.distance_tolerance;
    
    // 位置误差
    quality.position_error_u = target.u;
    quality.position_error_v = target.v;
    quality.is_position_good = std::abs(target.u) < 0.1 && std::abs(target.v) < 0.1;
    
    // 综合质量评分
    double distance_score = 1.0 - std::min(1.0, 
        std::abs(quality.distance_error) / cfg.desired_distance);
    double position_score = 1.0 - std::min(1.0, 
        std::sqrt(target.u * target.u + target.v * target.v));
    
    quality.quality_score = 0.5 * distance_score + 0.5 * position_score;
    
    return quality;
}

void IBVSController::setDesiredDistance(double distance) {
    pImpl->config.desired_distance = distance;
    FALCONMIND_LOG_INFO("Desired distance updated: {}m", distance);
}

void IBVSController::setDesiredHeight(double height) {
    pImpl->config.desired_height = height;
    FALCONMIND_LOG_INFO("Desired height updated: {}m", height);
}

IBVSConfig IBVSController::getConfig() const {
    return pImpl->config;
}

void IBVSController::updateConfig(const IBVSConfig& config) {
    pImpl->config = config;
    FALCONMIND_LOG_INFO("IBVS config updated");
}

void IBVSController::reset() {
    pImpl->integral_error = 0.0;
    pImpl->prev_error = 0.0;
    pImpl->update_count = 0;
    pImpl->total_error = 0.0;
    pImpl->last_update = std::chrono::steady_clock::now();
    
    FALCONMIND_LOG_INFO("IBVS Controller reset");
}

bool IBVSController::isAtTarget(
    double current_distance,
    double current_height,
    const ImageSpaceTarget& target
) const {
    const auto& cfg = pImpl->config;
    
    bool distance_ok = std::abs(current_distance - cfg.desired_distance) 
                       <= cfg.distance_tolerance;
    bool height_ok = std::abs(current_height - cfg.desired_height) 
                     <= cfg.height_tolerance;
    bool position_ok = std::abs(target.u) < 0.05 && std::abs(target.v) < 0.05;
    
    return distance_ok && height_ok && position_ok;
}

std::shared_ptr<IBVSController> createIBVSController(
    const IBVSConfig& config
) {
    return std::make_shared<IBVSController>(config);
}

std::shared_ptr<IBVSController> createConservativeIBVSController() {
    IBVSConfig config;
    config.kp_distance = 0.3;
    config.ki_distance = 0.05;
    config.kd_distance = 0.3;
    config.kp_position = 0.008;
    config.max_speed = 5.0;
    return createIBVSController(config);
}

std::shared_ptr<IBVSController> createAggressiveIBVSController() {
    IBVSConfig config;
    config.kp_distance = 0.8;
    config.ki_distance = 0.15;
    config.kd_distance = 0.1;
    config.kp_position = 0.015;
    config.max_speed = 10.0;
    return createIBVSController(config);
}

} // namespace control
} // namespace sdk
} // namespace falconmind
