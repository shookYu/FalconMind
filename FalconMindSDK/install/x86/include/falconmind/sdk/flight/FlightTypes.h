// FalconMindSDK - Flight related types
#pragma once

#include <string>
#include <vector>

namespace falconmind::sdk::flight {

enum class MavlinkVersion {
    V1 = 1,
    V2 = 2
};

struct FlightConnectionConfig {
    std::string   linkType{"UDP"};    // 先支持 UDP，后续扩展串口
    std::string   remoteAddress{"127.0.0.1"};
    int           remotePort{14540};  // PX4-SITL 默认端口之一
    MavlinkVersion mavlinkVersion{MavlinkVersion::V2}; // 默认使用 MAVLink v2
};

// FlightState 与 MAVLink 典型字段映射关系：
// - lat/lon/alt  ← GLOBAL_POSITION_INT.lat/lon/alt（1e-7 deg → deg，mm → m）
// - roll/pitch/yaw ← ATTITUDE.roll/pitch/yaw
// - vx/vy/vz ← GLOBAL_POSITION_INT.vx/vy/vz
// - batteryPercent/batteryVoltageMv ← BATTERY_STATUS
// - gpsFixType/numSat ← GPS_RAW_INT 或 GLOBAL_POSITION_INT 相关字段
// - gx/gy/gz ← HIGHRES_IMU.xgyro/ygyro/zgyro (rad/s)
// - ax/ay/az ← HIGHRES_IMU.xacc/yacc/zacc (m/s^2)
struct FlightState {
    // Position (from GLOBAL_POSITION_INT)
    double lat{0.0};
    double lon{0.0};
    double alt{0.0};
    
    // Attitude (from ATTITUDE)
    double roll{0.0};
    double pitch{0.0};
    double yaw{0.0};
    
    // Velocity (from GLOBAL_POSITION_INT)
    double vx{0.0};
    double vy{0.0};
    double vz{0.0};
    
    // IMU data (from HIGHRES_IMU)
    double gx{0.0};  // x angular velocity (rad/s)
    double gy{0.0};  // y angular velocity (rad/s)
    double gz{0.0};  // z angular velocity (rad/s)
    double ax{0.0};  // x acceleration (m/s^2)
    double ay{0.0};  // y acceleration (m/s^2)
    double az{0.0};  // z acceleration (m/s^2)
    
    // Battery (from BATTERY_STATUS)
    double batteryPercent{0.0};
    int    batteryVoltageMv{0};
    
    // GPS status (from GPS_RAW_INT)
    int    gpsFixType{0};
    int    numSat{0};
    double hdop{99.0};  // Horizontal dilution of precision
    
    // Vehicle status
    bool armed{false};
    bool inAir{false};
    int flightMode{0};  // 当前飞行模式
};

// FlightCommand 与 MAVLink COMMAND_LONG 的典型映射：
// - Arm           → MAV_CMD_COMPONENT_ARM_DISARM
// - Disarm        → MAV_CMD_COMPONENT_ARM_DISARM
// - Takeoff       → MAV_CMD_NAV_TAKEOFF
// - Land          → MAV_CMD_NAV_LAND
// - ReturnToLaunch→ MAV_CMD_NAV_RETURN_TO_LAUNCH
enum class FlightCommandType {
    Arm,
    Disarm,
    Takeoff,
    Land,
    ReturnToLaunch,
    NavigateTo,         // 导航到指定位置
    Orbit,              // 绕点盘旋
    Hover,              // 悬停
    StartMission,       // 开始执行任务
    SetManualMode,      // 设置手动模式
    SetPositionMode,    // 设置定点模式
    SetOffboardMode,    // 设置外部控制模式
    SendMavlinkRaw      // 发送原始MAVLink消息
};

struct FlightCommand {
    FlightCommandType type{FlightCommandType::Arm};
    double targetAlt{0.0};              // 目标高度
    double targetLat{0.0};              // 目标纬度
    double targetLon{0.0};              // 目标经度
    double orbitRadius{0.0};            // 盘旋半径
    double orbitVelocity{0.0};          // 盘旋速度
    std::vector<uint8_t> mavlinkData;   // 原始MAVLink消息数据
};

} // namespace falconmind::sdk::flight
