// FalconMindSDK - LiDAR SLAM Node
// 基于LOAM/LIO-SAM的LiDAR SLAM实现

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/perception/PoseTypes.h"

namespace falconmind::sdk::perception {

struct Point3D {
    float x, y, z;
    float intensity;
    uint32_t ring;
    double timestamp;
};

using PointCloud = std::vector<Point3D>;

/**
 * @brief LiDAR SLAM节点 - 基于LOAM算法实现
 * 
 * 功能：
 * - 点云特征提取（边缘/平面）
 * - 帧间匹配与位姿估计
 * - 地图构建与优化
 * - 回环检测（可选）
 * 
 * 输入Pad：
 * - pointcloud_in: PointCloud (点云数据)
 * 
 * 输出Pad：
 * - pose_out: Pose3D (位姿)
 */
class LidarSlamNode : public core::Node {
public:
    LidarSlamNode();
    ~LidarSlamNode() override;
    
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;
    
    /// 无SLAM数据时是否输出默认位姿，默认true
    void setOutputWhenNoClient(bool v) { outputWhenNoClient_ = v; }

private:
    void onPointCloudData(const void* data, size_t size);
    void processPointCloud(const PointCloud& cloud);
    void extractFeatures(const PointCloud& cloud, 
                         PointCloud& edgeFeatures, 
                         PointCloud& planarFeatures);
    bool estimatePose(const PointCloud& edgeFeatures,
                      const PointCloud& planarFeatures,
                      Pose3D& pose);
    void publishPose(const Pose3D& pose);

private:
    bool started_{false};
    bool outputWhenNoClient_{true};
    uint64_t defaultPoseTimestampNs_{0};
    
    // SLAM状态
    bool isInitialized_{false};
    Pose3D lastPose_;
    PointCloud localMap_;
    
    // 配置参数
    int scanLineCount_{16};  // LiDAR线数
    float edgeThreshold_{0.1f};
    float planarThreshold_{0.1f};
    int maxIterations_{10};
    
    // 统计
    uint64_t processedScanCount_{0};
};

} // namespace falconmind::sdk::perception
// 输入点云，输出位姿；可注入 ISlamServiceClient 对接算法容器（如 SLAMService.GetPose）。
#pragma once

#include <cstdint>
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/perception/PoseTypes.h"
#include "falconmind/sdk/perception/ISlamServiceClient.h"

namespace falconmind::sdk::perception {

class LidarSlamNode : public core::Node {
public:
    LidarSlamNode();
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

    void setSlamServiceClient(SlamServiceClientPtr client) { slamClient_ = std::move(client); }
    void setOutputWhenNoClient(bool v) { outputWhenNoClient_ = v; }

private:
    bool started_{false};
    bool outputWhenNoClient_{true};
    std::uint64_t defaultPoseTimestampNs_{0};
    SlamServiceClientPtr slamClient_;
};

} // namespace falconmind::sdk::perception
