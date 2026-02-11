// FalconMindSDK - 算法容器 SLAM 服务客户端抽象（供 VisualSlamNode / LidarSlamNode 对接）
// 已实现：Stub、FromFile（从文件读位姿）；可选 gRPC 客户端见 SlamServiceGrpcClient。
#pragma once

#include "falconmind/sdk/perception/PoseTypes.h"

#include <memory>
#include <string>

namespace falconmind::sdk::perception {

/** SLAM 服务客户端接口：从算法容器获取当前位姿等 */
class ISlamServiceClient {
public:
    virtual ~ISlamServiceClient() = default;

    /** 获取当前位姿；成功返回 true 并写入 pose */
    virtual bool getPose(Pose3D& pose) = 0;

    /** 是否已连接/可用 */
    virtual bool isAvailable() const = 0;
};

using SlamServiceClientPtr = std::shared_ptr<ISlamServiceClient>;

/** Stub 实现：不连接算法容器，getPose 始终返回 false */
class SlamServiceClientStub : public ISlamServiceClient {
public:
    bool getPose(Pose3D&) override { return false; }
    bool isAvailable() const override { return false; }
};

/** 从文件读取位姿：算法容器将当前位姿写入指定文件（二进制 Pose3D 布局），SDK 周期性读取 */
class SlamServiceClientFromFile : public ISlamServiceClient {
public:
    explicit SlamServiceClientFromFile(std::string path) : poseFilePath_(std::move(path)) {}
    bool getPose(Pose3D& pose) override;
    bool isAvailable() const override;

    void setPoseFilePath(std::string path) { poseFilePath_ = std::move(path); }
    const std::string& getPoseFilePath() const { return poseFilePath_; }

private:
    std::string poseFilePath_;
};

} // namespace falconmind::sdk::perception
