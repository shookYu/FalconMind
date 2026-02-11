// FalconMindSDK - Flight related types (week2 skeleton)
#pragma once

#include <string>

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

// FlightState 与 MAVLink 典型字段映射关系（未来在 FlightConnectionService 中实现）：
// - lat/lon/alt  ← GLOBAL_POSITION_INT.lat/lon/alt（1e-7 deg → deg，mm → m）
// - roll/pitch/yaw ← ATTITUDE.roll/pitch/yaw
// - vx/vy/vz ← GLOBAL_POSITION_INT.vx/vy/vz
// - batteryPercent/batteryVoltageMv ← BATTERY_STATUS
// - gpsFixType/numSat ← GPS_RAW_INT 或 GLOBAL_POSITION_INT 相关字段
struct FlightState {
    double lat{0.0};
    double lon{0.0};
    double alt{0.0};
    double roll{0.0};
    double pitch{0.0};
    double yaw{0.0};
    double vx{0.0};
    double vy{0.0};
    double vz{0.0};
    double batteryPercent{0.0};
    int    batteryVoltageMv{0};
    int    gpsFixType{0};
    int    numSat{0};
};

// FlightCommand 与 MAVLink COMMAND_LONG 的典型映射（示意）：
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
    ReturnToLaunch
};

struct FlightCommand {
    FlightCommandType type{FlightCommandType::Arm};
    double targetAlt{0.0};  // Takeoff/Land 时可用
};

} // namespace falconmind::sdk::flight

