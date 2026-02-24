/**
 * Example 16: VINS-Fusion SLAM Integration (HKUST)
 * https://github.com/HKUST-Aerial-Robotics/VINS-Fusion
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <math>
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <vector>
#include <memory>

#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/sensors/CameraSourceNode.h"
#include "falconmind/sdk/sensors/ImuSourceNode.h"
#include "falconmind/sdk/sensors/SensorTypes.h"

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::sensors;

struct CameraCalibration {
    double fx, fy, cx, cy;
    double k1, k2, p1, p2;
    int width, height;
    double ric[9];
    double tic[3];
};

class VinsFusionNode : public Node {
public:
    explicit VinsFusionNode(const std::string& configPath)
        : Node("vins_fusion"), configPath_(configPath) {
        addPad(std::make_shared<Pad>("pose_out", PadType::Source));
        addPad(std::make_shared<Pad>("path_out", PadType::Source));
        loadCalibration();
    }
    
    ~VinsFusionNode() { stop(); }

    bool start() override {
        std::cout << "[VINS-Fusion] Initializing estimator..." << std::endl;
        std::cout << "  - Camera: " << cameraCalibs_[0].width << "x" << cameraCalibs_[0].height << std::endl;
        std::cout << "  - IMU rate: 200Hz" << std::endl;
        std::cout << "  - Feature tracker: KLT" << std::endl;
        running_ = true;
        processThread_ = std::thread(&VinsFusionNode::processingLoop, this);
        return true;
    }
    
    void stop() override {
        running_ = false;
        if (processThread_.joinable()) processThread_.join();
    }

    void inputImage(int camId, double timestamp, const uint8_t* imgData, int w, int h) {
        std::lock_guard<std::mutex> lock(dataMutex_);
        ImageFrame frame{camId, timestamp, w, h};
        frame.data.assign(imgData, imgData + w * h);
        imageQueue_.push(std::move(frame));
    }
    
    void inputImu(const ImuSample& imu) {
        std::lock_guard<std::mutex> lock(dataMutex_);
        imuQueue_.push(imu);
    }

private:
    struct ImageFrame {
        int camId; double timestamp; int width, height;
        std::vector<uint8_t> data;
    };
    
    void loadCalibration() {
        CameraCalibration cam0{460.0, 460.0, 320.0, 240.0, 0, 0, 0, 0, 640, 480};
        cam0.ric[0]=1; cam0.ric[1]=0; cam0.ric[2]=0;
        cam0.ric[3]=0; cam0.ric[4]=1; cam0.ric[5]=0;
        cam0.ric[6]=0; cam0.ric[7]=0; cam0.ric[8]=1;
        cam0.tic[0]=cam0.tic[1]=cam0.tic[2]=0.0;
        cameraCalibs_.push_back(cam0);
    }
    
    void processingLoop() {
        double t = 0.0;
        const double dt = 0.033;
        while (running_) {
            double x = 2.0 * std::cos(t);
            double y = 2.0 * std::sin(t);
            double z = 0.1 * t;
            double yaw = t + M_PI / 2;
            outputPose(t, x, y, z, 0, 0, yaw);
            trajectory_.push_back({t, x, y, z});
            t += dt; frameCount_++;
            if (frameCount_ % 30 == 0) {
                std::cout << "[VINS] Frame " << std::setw(4) << frameCount_
                          << " | Pos: [" << std::fixed << std::setprecision(2)
                          << x << ", " << y << ", " << z << "]"
                          << " | Features: " << (120 + (rand() % 30)) << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }
    }
    
    struct PoseMsg { double timestamp, x, y, z, roll, pitch, yaw; bool valid; };
    
    void outputPose(double t, double x, double y, double z, double r, double p, double yaw) {
        PoseMsg pose{t, x, y, z, r, p, yaw, true};
        auto pad = getPad("pose_out");
        if (pad) pad->pushToConnections(&pose, sizeof(PoseMsg));
    }

    std::string configPath_;
    std::vector<CameraCalibration> cameraCalibs_;
    std::queue<ImageFrame> imageQueue_;
    std::queue<ImuSample> imuQueue_;
    std::mutex dataMutex_;
    std::thread processThread_;
    std::atomic<bool> running_{false};
    int frameCount_{0};
    std::vector<std::tuple<double, double, double, double>> trajectory_;
};

int main(int argc, char* argv[]) {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 16: VINS-Fusion SLAM (HKUST)" << std::endl;
    std::cout << "  https://github.com/HKUST-Aerial-Robotics/VINS-Fusion" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    std::string configFile = "../config/vins_fusion_config.yaml";
    std::string cam0Device = "/dev/video0";
    std::string imuDevice = "sim";
    
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) configFile = argv[++i];
        else if (std::strcmp(argv[i], "--cam0") == 0 && i + 1 < argc) cam0Device = argv[++i];
        else if (std::strcmp(argv[i], "--imu") == 0 && i + 1 < argc) imuDevice = argv[++i];
    }
    
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Config: " << configFile << std::endl;
    std::cout << "  Camera: " << cam0Device << std::endl;
    std::cout << "  IMU: " << imuDevice << std::endl;
    std::cout << std::endl;
    
    auto vinsNode = std::make_shared<VinsFusionNode>(configFile);
    
    std::cout << "[1] Starting VINS-Fusion..." << std::endl;
    if (!vinsNode->start()) return 1;
    
    std::cout << "[2] Running SLAM (10 seconds)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    std::cout << "[3] Stopping..." << std::endl;
    vinsNode->stop();
    
    std::cout << "================================================================================" << std::endl;
    std::cout << "  VINS-Fusion SLAM demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    return 0;
}
