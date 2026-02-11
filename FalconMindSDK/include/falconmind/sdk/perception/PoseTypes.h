// FalconMindSDK - 位姿类型（供 SLAM / 定位节点输出）
#pragma once

#include <cstdint>

namespace falconmind::sdk::perception {

// 3D 位姿：位置 + 四元数 (qx,qy,qz,qw)
struct Pose3D {
    double x{0.}, y{0.}, z{0.};
    double qx{0.}, qy{0.}, qz{0.}, qw{1.};
    std::uint64_t timestampNs{0};
};

} // namespace falconmind::sdk::perception
