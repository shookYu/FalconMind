// FalconMindSDK - Lightweight LidarSourceNode (no external LiDAR required)
// Provides the required symbols to compile in this environment.

#include "falconmind/sdk/sensors/LidarSourceNode.h"
#include "falconmind/sdk/core/Pad.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

namespace falconmind::sdk::sensors {
using namespace falconmind::sdk::core;

LidarSourceNode::LidarSourceNode() : Node("lidar_source"), config_() {
    addPad(std::make_shared<Pad>("points_out", PadType::Source));
}

LidarSourceNode::LidarSourceNode(const LidarConfig& cfg) : Node("lidar_source"), config_(cfg) {
    addPad(std::make_shared<Pad>("points_out", PadType::Source));
}

LidarSourceNode::~LidarSourceNode() {
    stop();
}

bool LidarSourceNode::configure(const std::unordered_map<std::string, std::string>& params) {
    // Optional: support simple overrides in the future
    (void)params;
    return true;
}

bool LidarSourceNode::start() {
    started_ = true;
    simulationMode_ = true; // default to simulation in this environment
    if (simulationMode_) {
        // nothing to start
    } else {
        // try to open socket if linux
        initSocket();
        if (socketFd_ >= 0) {
            receiveThread_ = std::thread(&LidarSourceNode::receiveThreadFunc, this);
        }
    }
    return true;
}

void LidarSourceNode::stop() {
    stopThread_.store(true);
#ifdef __linux__
    if (receiveThread_.joinable()) receiveThread_.join();
    shutdownSocket();
#endif
    started_ = false;
}

void LidarSourceNode::process() {
    if (!started_) return;
    // In simulation mode, emit a small synthetic frame each cycle
    if (simulationMode_) {
        PointCloud cloud; cloud.reserve(16);
        for (int i=0;i<16;++i){ PointXYZI p; p.x = i*0.1f; p.y = 0.f; p.z = 0.f; p.intensity = 1.0f; cloud.push_back(p);}        
        auto pad = getPad("points_out");
        if (pad && !cloud.empty()) {
            pad->pushToConnections(cloud.data(), cloud.size()*sizeof(PointXYZI));
        }
    }
}

bool LidarSourceNode::initSocket() {
#ifdef __linux__
    // Basic UDP socket for Velodyne-like data
    socketFd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd_ < 0) return false;
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(2368);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (::bind(socketFd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(socketFd_);
        socketFd_ = -1;
        return false;
    }
    return true;
#else
    (void)config_;
    return false;
#endif
}

void LidarSourceNode::shutdownSocket() {
#ifdef __linux__
    if (socketFd_ >= 0) {
        close(socketFd_);
        socketFd_ = -1;
    }
#endif
}

void LidarSourceNode::receiveThreadFunc() {
#ifdef __linux__
    if (socketFd_ < 0) return;
    uint8_t buf[4096];
    while (!stopThread_.load()) {
        ssize_t n = ::recvfrom(socketFd_, buf, sizeof(buf), 0, nullptr, nullptr);
        if (n > 0) {
            // In this minimal stub we push an empty cloud to preserve pipeline flow
            PointCloud cloud; pushPointCloud(cloud);
        } else {
            // Sleep to avoid busy loop
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }
#endif
}

bool LidarSourceNode::parseVelodynePacket(const uint8_t* data, size_t len, PointCloud& cloud) {
    (void)data; (void)len; cloud.clear(); return true;
}

    (void)data; (void)len; cloud.clear(); return true;
}

void LidarSourceNode::generateSimulatedPointCloud(PointCloud& cloud) {
    cloud.clear(); cloud.reserve(32);
    for (int i=0;i<32;++i) { PointXYZI p{static_cast<float>(i)*0.2f, 0.f, 0.f, 1.0f}; cloud.push_back(p);}    
}

void LidarSourceNode::pushPointCloud(const PointCloud& cloud) {
    std::lock_guard<std::mutex> lk(queueMutex_);
    cloudQueue_.push(cloud);
}

} // namespace falconmind::sdk::sensors
