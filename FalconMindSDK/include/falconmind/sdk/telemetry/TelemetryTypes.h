// FalconMindSDK - Telemetry message types for SDK → NodeAgent communication
#pragma once

#include <string>
#include <cstdint>

namespace falconmind::sdk::telemetry {

// SDK 内部 Telemetry 消息结构（对应 Interface_Proto_Draft.md 中的 UavTelemetryMessage）
// 用于 SDK 内部节点发布遥测数据，后续由 NodeAgent 序列化为 Proto 并上报到 Cluster/Viewer
struct TelemetryMessage {
    std::string uavId{"uav0"};  // 当前 UAV 标识（后续从配置读取）

    // 时间戳（Unix epoch nanoseconds）
    int64_t timestampNs{0};

    // 位置（WGS84）
    double lat{0.0};
    double lon{0.0};
    double alt{0.0};

    // 姿态（欧拉角，rad）
    double roll{0.0};
    double pitch{0.0};
    double yaw{0.0};

    // 速度（m/s，ENU 坐标系）
    double vx{0.0};
    double vy{0.0};
    double vz{0.0};

    // 电池状态
    double batteryPercent{0.0};
    int32_t batteryVoltageMv{0};

    // GPS 状态
    int32_t gpsFixType{0};  // 0=None, 1=2D, 2=3D, 3=RTK Float, 4=RTK Fixed
    int32_t numSat{0};

    // 链路质量（0-100）
    double linkQuality{0.0};

    // 飞行模式（字符串，如 "MANUAL", "OFFBOARD", "AUTO_MISSION", "RTL"）
    std::string flightMode{"UNKNOWN"};
};

} // namespace falconmind::sdk::telemetry
