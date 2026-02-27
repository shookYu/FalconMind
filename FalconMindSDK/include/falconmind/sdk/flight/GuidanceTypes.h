/**
 * @file GuidanceTypes.h
 * @brief 制导控制相关类型定义
 */

#pragma once

#include "falconmind/sdk/sensors/NavigationTypes.h"
#include <cstdint>
#include <string>
#include <optional>

namespace falconmind {
namespace sdk {
namespace flight {

/**
 * @brief 制导模式
 */
enum class GuidanceMode {
    POSITION,           ///< 位置控制（航点跟踪）
    VELOCITY,           ///< 速度控制
    ATTITUDE,           ///< 姿态控制
    RATE,               ///< 角速度控制
    ACCELERATION        ///< 加速度控制
};

/**
 * @brief 制导命令
 * 
 * 视觉制导系统输出的控制指令
 */
struct GuidanceCommand {
    GuidanceMode mode{GuidanceMode::VELOCITY};
    
    // 目标位置（位置模式）
    std::optional<sensors::GeoPoint> targetPosition;
    std::optional<sensors::LocalPosition> targetLocalPosition;
    
    // 目标速度（速度模式）
    float velocityX{0.0f};              ///< 前向速度（m/s）
    float velocityY{0.0f};              ///< 右向速度（m/s）
    float velocityZ{0.0f};              ///< 垂直速度（m/s），正为上升
    float yawRate{0.0f};                ///< 偏航角速度（度/s）
    
    // 目标姿态（姿态模式）
    std::optional<sensors::Attitude> targetAttitude;
    
    // 目标加速度（加速度模式）
    float accelX{0.0f};                 ///< 前向加速度（m/s^2）
    float accelY{0.0f};                 ///< 右向加速度（m/s^2）
    float accelZ{0.0f};                 ///< 垂直加速度（m/s^2）
    
    // 执行参数
    float maxSpeed{10.0f};              ///< 最大执行速度
    float maxAcceleration{2.0f};        ///< 最大加速度
    float maxYawRate{30.0f};            ///< 最大偏航角速度（度/s）
    
    // 控制标志
    bool useHeading{false};             ///< 是否控制航向
    float targetHeading{0.0f};          ///< 目标航向（度）
    
    // 有效性
    bool valid{true};                   ///< 命令是否有效
    std::string errorMessage;           ///< 错误信息（如果无效）
    
    // 时间戳
    uint64_t timestampNs{0};
    
    // 便捷构造函数
    static GuidanceCommand createVelocityCommand(float vx, float vy, float vz, float yawRate = 0) {
        GuidanceCommand cmd;
        cmd.mode = GuidanceMode::VELOCITY;
        cmd.velocityX = vx;
        cmd.velocityY = vy;
        cmd.velocityZ = vz;
        cmd.yawRate = yawRate;
        return cmd;
    }
    
    static GuidanceCommand createPositionCommand(const sensors::GeoPoint& pos) {
        GuidanceCommand cmd;
        cmd.mode = GuidanceMode::POSITION;
        cmd.targetPosition = pos;
        return cmd;
    }
    
    static GuidanceCommand createAttitudeCommand(const sensors::Attitude& att) {
        GuidanceCommand cmd;
        cmd.mode = GuidanceMode::ATTITUDE;
        cmd.targetAttitude = att;
        return cmd;
    }
    
    static GuidanceCommand createHoverCommand() {
        GuidanceCommand cmd;
        cmd.mode = GuidanceMode::VELOCITY;
        cmd.velocityX = 0;
        cmd.velocityY = 0;
        cmd.velocityZ = 0;
        cmd.yawRate = 0;
        return cmd;
    }
    
    static GuidanceCommand createInvalidCommand(const std::string& error) {
        GuidanceCommand cmd;
        cmd.valid = false;
        cmd.errorMessage = error;
        return cmd;
    }
    
    // 便捷方法
    float getSpeedMagnitude() const {
        return std::sqrt(velocityX * velocityX + velocityY * velocityY + velocityZ * velocityZ);
    }
    
    bool isZeroCommand() const {
        return std::abs(velocityX) < 0.01f && 
               std::abs(velocityY) < 0.01f && 
               std::abs(velocityZ) < 0.01f && 
               std::abs(yawRate) < 0.1f;
    }
};

/**
 * @brief 制导状态
 */
struct GuidanceState {
    bool targetLocked{false};           ///< 是否锁定目标
    float targetBearing{0.0f};          ///< 目标方位（度）
    float targetElevation{0.0f};        ///< 目标仰角（度）
    float targetDistance{0.0f};         ///< 目标距离（米）
    
    float lineOfSightRateX{0.0f};       ///< 视线角速度X（度/s）
    float lineOfSightRateY{0.0f};       ///< 视线角速度Y（度/s）
    
    bool approaching{false};            ///< 是否正在接近
    float timeToImpact{0.0f};           ///< 预计到达时间（秒）
    
    float trackingErrorX{0.0f};         ///< 跟踪误差X（像素或米）
    float trackingErrorY{0.0f};         ///< 跟踪误差Y（像素或米）
    
    uint64_t timestampNs{0};
};

/**
 * @brief 制导配置
 */
struct GuidanceConfig {
    // PID参数
    float kpX{1.0f}, kiX{0.0f}, kdX{0.1f};
    float kpY{1.0f}, kiY{0.0f}, kdY{0.1f};
    float kpZ{1.0f}, kiZ{0.0f}, kdZ{0.1f};
    
    // 前馈增益
    float kffVelocity{0.5f};
    float kffAcceleration{0.1f};
    
    // 限幅
    float maxVelocityX{10.0f};          ///< 最大前向速度
    float maxVelocityY{5.0f};           ///< 最大横向速度
    float maxVelocityZ{3.0f};           ///< 最大垂直速度
    
    // 目标跟踪参数
    float minTargetDistance{3.0f};      ///< 最小目标距离（安全距离）
    float maxTargetDistance{100.0f};    ///< 最大目标距离
    float desiredTargetDistance{10.0f}; ///< 期望跟踪距离
    
    // 跟随模式参数
    float followDistance{10.0f};        ///< 跟随距离
    float followHeight{15.0f};          ///< 跟随高度
    float followSpeed{5.0f};            ///< 跟随速度
};

} // namespace flight
} // namespace sdk
} // namespace falconmind
