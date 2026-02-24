// FalconMindSDK - LiDAR Source Node with real sensor support
// Supports: Velodyne (VLP-16, VLP-32, HDL-64, VLS-128), Livox, file replay
#pragma once

#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/sensors/SensorTypes.h"

#ifdef __linux__
#include <netinet/in.h>
#include <sys/socket.h>
#endif

namespace falconmind::sdk::sensors {

enum class LidarModel {
    Unknown,
    VelodyneVLP16,    // 16 beams
    VelodyneVLP32,    // 32 beams
    VelodyneHDL64,    // 64 beams
    VelodyneVLS128,   // 128 beams
    LivoxMid40,       // Livox Mid-40
    LivoxMid70,       // Livox Mid-70
    LivoxHorizon,     // Livox Horizon
    LivoxAvia,        // Livox Avia
};

struct LidarConfig {
    LidarModel model = LidarModel::VelodyneVLP16;
    std::string ipAddress = "0.0.0.0";    // Listen IP
    int port = 2368;                       // Default Velodyne port
    bool useDualReturn = false;            // Dual return mode
};

class LidarSourceNode : public core::Node {
public:
    LidarSourceNode();
    explicit LidarSourceNode(const LidarConfig& cfg);
    ~LidarSourceNode() override;
    
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;

    bool isConnected() const { return connected_.load(); }
    size_t getFrameCount() const { return frameCount_.load(); }

private:
    void pushPointCloud(const PointCloud& cloud);
    bool initSocket();
    void shutdownSocket();
    void receiveThreadFunc();
    bool parseVelodynePacket(const uint8_t* data, size_t len, PointCloud& cloud);
    bool parseLivoxPacket(const uint8_t* data, size_t len, PointCloud& cloud);
    void generateSimulatedPointCloud(PointCloud& cloud);

    LidarConfig config_;
    std::string deviceOrUri_;
    bool started_{false};
    std::ifstream replayFile_;
    bool replayMode_{false};
    bool simulationMode_{false};
    
    // Network
    int socketFd_{-1};
    std::atomic<bool> connected_{false};
    std::thread receiveThread_;
    std::atomic<bool> stopThread_{false};
    
    // Thread-safe queue for point clouds
    std::queue<PointCloud> cloudQueue_;
    std::mutex queueMutex_;
    
    // Statistics
    std::atomic<size_t> frameCount_{0};
    std::atomic<size_t> pointCount_{0};
};

} // namespace falconmind::sdk::sensors
