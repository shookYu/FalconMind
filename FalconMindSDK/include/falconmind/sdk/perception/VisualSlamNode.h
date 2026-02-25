// FalconMindSDK - Visual SLAM Node
// 基于VINS-Fusion的视觉惯性SLAM实现
// 输入图像+IMU，输出位姿

#pragma once

#include <cstdint>
#include <memory>
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/perception/PoseTypes.h"

// 前向声明VINS-Fusion适配器
namespace falconmind::sdk::perception {
    class VinsFusionAdapter;
    struct VinsOutput;
}

namespace falconmind::sdk::perception {

/**
 * @brief 视觉SLAM节点 - 基于VINS-Fusion实现
 * 
 * 功能：
 * - 单目/双目相机输入
 * - IMU数据融合
 * - 实时位姿估计
 * - 回环检测（可选）
 * 
 * 输入Pad：
 * - image_in: CameraFramePacket (图像数据)
 * - imu_in: ImuSample (IMU数据，可选)
 * 
 * 输出Pad：
 * - pose_out: Pose3D (位姿)
 */
class VisualSlamNode : public core::Node {
public:
    VisualSlamNode();
    ~VisualSlamNode() override;
    
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;
    
    /// 无SLAM数据时是否输出默认位姿（单位阵），默认true
    void setOutputWhenNoClient(bool v) { outputWhenNoClient_ = v; }

private:
    void onImageData(const void* data, size_t size);
    void onImuData(const void* data, size_t size);
    void onVinsOutput(const VinsOutput& output);

private:
    bool started_{false};
    bool outputWhenNoClient_{true};
    std::uint64_t defaultPoseTimestampNs_{0};
    
    // VINS-Fusion适配器
    std::unique_ptr<VinsFusionAdapter> vinsAdapter_;
};

} // namespace falconmind::sdk::perception
