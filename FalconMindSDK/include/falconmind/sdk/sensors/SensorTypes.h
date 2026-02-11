// FalconMindSDK - 传感器通用类型（点云、IMU、GNSS 等）
#pragma once

#include <cstdint>
#include <vector>

namespace falconmind::sdk::sensors {

// 点云点（xyz + 可选强度/环数）
struct PointXYZI {
    float x{0.f}, y{0.f}, z{0.f};
    float intensity{0.f};
};
using PointCloud = std::vector<PointXYZI>;

// IMU 一帧：角速度(rad/s)、加速度(m/s^2)、时间戳
struct ImuSample {
    double gx{0.}, gy{0.}, gz{0.};
    double ax{0.}, ay{0.}, az{0.};
    std::uint64_t timestampNs{0};
};

// GNSS 一帧：经纬度(度)、高度(m)、精度、时间戳
struct GnssSample {
    double latitude{0.};
    double longitude{0.};
    double altitude{0.};
    float hdop{99.f};
    int numSatellites{0};
    std::uint64_t timestampNs{0};
};

} // namespace falconmind::sdk::sensors
