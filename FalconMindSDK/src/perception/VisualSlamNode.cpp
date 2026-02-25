// FalconMindSDK - Visual SLAM Node Implementation
// 基于VINS-Fusion的视觉SLAM实现

#include "falconmind/sdk/perception/VisualSlamNode.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/sensors/CameraFramePacket.h"
#include "falconmind/sdk/sensors/ImuSourceNode.h"

#include <iostream>
#include <chrono>
#include <cmath>

// 包含VINS-Fusion适配器
#include "3rd/vins_fusion/VinsFusionAdapter.h"

namespace falconmind::sdk::perception {

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::sensors;

VisualSlamNode::VisualSlamNode() : Node("visual_slam") {
    addPad(std::make_shared<Pad>("image_in", PadType::Sink));
    addPad(std::make_shared<Pad>("imu_in", PadType::Sink));
    addPad(std::make_shared<Pad>("pose_out", PadType::Source));
}

VisualSlamNode::~VisualSlamNode() {
    stop();
}

bool VisualSlamNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("output_when_no_client");
    if (it != params.end())
        outputWhenNoClient_ = (it->second == "1" || it->second == "true" || it->second == "yes");
    
    // VINS-Fusion配置
    auto configIt = params.find("vins_config");
    if (configIt != params.end()) {
        std::cout << "[VisualSlamNode] Using VINS-Fusion config: " << configIt->second << std::endl;
    }
    
    // 相机参数
    int width = 640, height = 480;
    auto widthIt = params.find("camera_width");
    if (widthIt != params.end()) width = std::stoi(widthIt->second);
    auto heightIt = params.find("camera_height");
    if (heightIt != params.end()) height = std::stoi(heightIt->second);
    
    // 初始化VINS-Fusion
    VinsFusionConfig vinsConfig;
    vinsConfig.camera.imageWidth = width;
    vinsConfig.camera.imageHeight = height;
    vinsConfig.camera.cameraCount = 1; // 默认单目
    vinsConfig.configFile = (configIt != params.end()) ? configIt->second : "";
    
    vinsAdapter_ = std::make_unique<VinsFusionAdapter>(vinsConfig);
    
    // 设置输出回调
    vinsAdapter_->setOutputCallback([this](const VinsOutput& output) {
        this->onVinsOutput(output);
    });
    
    return true;
}

bool VisualSlamNode::start() {
    if (!vinsAdapter_) {
        std::cerr << "[VisualSlamNode] VINS adapter not initialized" << std::endl;
        return false;
    }
    
    if (!vinsAdapter_->initialize()) {
        std::cerr << "[VisualSlamNode] Failed to initialize VINS-Fusion" << std::endl;
        return false;
    }
    
    started_ = true;
    defaultPoseTimestampNs_ = 0;
    
    // 设置Pad数据回调
    auto imagePad = getPad("image_in");
    if (imagePad) {
        imagePad->setDataCallback([this](const void* data, size_t size) {
            this->onImageData(data, size);
        });
    }
    
    auto imuPad = getPad("imu_in");
    if (imuPad) {
        imuPad->setDataCallback([this](const void* data, size_t size) {
            this->onImuData(data, size);
        });
    }
    
    std::cout << "[VisualSlamNode] Started with VINS-Fusion backend" << std::endl;
    return true;
}

void VisualSlamNode::stop() {
    started_ = false;
    if (vinsAdapter_) {
        vinsAdapter_->shutdown();
    }
    std::cout << "[VisualSlamNode] Stopped" << std::endl;
}

void VisualSlamNode::process() {
    if (!started_) return;
    
    // VINS-Fusion在后台线程处理，这里只是定期检查输出
    // 输出通过回调函数onVinsOutput处理
}

void VisualSlamNode::onImageData(const void* data, size_t size) {
    if (!vinsAdapter_ || !started_) return;
    
    // 解析CameraFramePacket
    if (size < sizeof(CameraFramePacket)) return;
    
    const auto* packet = static_cast<const CameraFramePacket*>(data);
    const uint8_t* imageData = static_cast<const uint8_t*>(data) + sizeof(CameraFramePacket);
    
    double timestamp = packet->.timestampNs / 1e9;
    
    // 输入到VINS-Fusion
    vinsAdapter_->inputImage(timestamp, imageData, nullptr);
}

void VisualSlamNode::onImuData(const void* data, size_t size) {
    if (!vinsAdapter_ || !started_) return;
    
    if (size < sizeof(ImuSample)) return;
    
    const auto* imu = static_cast<const ImuSample*>(data);
    vinsAdapter_->inputImu(*imu);
}

void VisualSlamNode::onVinsOutput(const VinsOutput& output) {
    auto outPad = getPad("pose_out");
    if (!outPad) return;
    
    Pose3D pose;
    pose.x = output.pw_x;
    pose.y = output.pw_y;
    pose.z = output.pw_z;
    pose.qx = output.qx;
    pose.qy = output.qy;
    pose.qz = output.qz;
    pose.qw = output.qw;
    pose.timestampNs = static_cast<uint64_t>(output.timestamp * 1e9);
    
    outPad->pushToConnections(&pose, sizeof(pose));
    
    // 调试输出
    static int outputCount = 0;
    if (++outputCount % 30 == 0) {
        std::cout << "[VisualSlamNode] Pose #" << outputCount 
                  << " | Pos: (" << pose.x << ", " << pose.y << ", " << pose.z << ")"
                  << " | Features: " << output.trackedFeatures << std::endl;
    }
}

} // namespace falconmind::sdk::perception
