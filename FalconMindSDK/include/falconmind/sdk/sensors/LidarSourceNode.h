// FalconMindSDK - LiDAR Source Node with real sensor support
// Supports: Velodyne (VLP-16, VLP-32, HDL-64, VLS-128), Livox, file replay, simulation
#pragma once

#include <fstream>
#include <string>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/sensors/SensorTypes.h"

#ifdef __linux__
#include <netinet/in.h>
#endif

namespace falconmind::sdk::sensors {

// LiDAR设备类型
enum class LidarDeviceType {
    Unknown = 0,
    Simulation,
    VelodyneVLP16,    // 16线
    VelodyneVLP32,    // 32线
    VelodyneHDL64,    // 64线
    VelodyneVLS128,   // 128线
    LivoxMid40,       // Livox Mid-40
    LivoxMid70,       // Livox Mid-70
    LivoxHorizon,     // Livox Horizon
    LivoxAvia,        // Livox Avia
};

// LiDAR配置结构
struct LidarConfig {
    LidarDeviceType deviceType = LidarDeviceType::VelodyneVLP16;
    std::string ipAddress = "0.0.0.0";      // 监听IP
    uint16_t port = 2368;                    // 默认Velodyne端口
    int scanLineCount = 16;                  // 扫描线数
    float frameRateHz = 10.0f;               // 帧率
    bool enableMotionCompensation = false;   // 运动补偿
    float minRange = 0.3f;                   // 最小距离(m)
    float maxRange = 100.0f;                 // 最大距离(m)
};

// 原始数据包结构
struct RawPacket {
    uint64_t timestamp;
    std::vector<uint8_t> data;
    std::string sourceIp;
    uint16_t sourcePort;
};

/**
 * @brief LiDAR源节点
 * 
 * 支持设备：
 * - Velodyne: VLP-16, VLP-32, HDL-64E, VLS-128
 * - Livox: Mid-40, Mid-70, Horizon, Avia
 * 
 * 输入：UDP数据包（实际硬件）或模拟生成
 * 输出：PointCloud点云数据
 */
class LidarSourceNode : public core::Node {
public:
    LidarSourceNode();
    explicit LidarSourceNode(const LidarConfig& cfg);
    ~LidarSourceNode() override;
    
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;
    
    // 状态查询
    bool isStarted() const { return started_.load(); }
    bool isSimulationMode() const { return simulationMode_; }
    uint64_t getPacketCount() const { return packetCount_.load(); }
    uint64_t getFrameCount() const { return frameCount_.load(); }
    
    // 点云后处理
    void applyMotionCompensation(PointCloud& cloud, const ImuSample& imu, uint64_t targetTime);
    void filterPointCloud(PointCloud& cloud, float minRange, float maxRange);
    void downsamplePointCloud(PointCloud& cloud, float leafSize);

private:
    // Socket操作
    bool initSocket();
    void shutdownSocket();
    
    // 线程函数
    void receiveThreadFunc();
    void assemblyThreadFunc();
    
    // 数据包解析
    bool parseVelodynePacket(const uint8_t* data, size_t len, PointCloud& cloud);
    bool parseLivoxPacket(const uint8_t* data, size_t len, PointCloud& cloud);
    void generateSimulatedPointCloud(PointCloud& cloud);
    
    // 配置
    LidarConfig config_;
    
    // 状态
    std::atomic<bool> started_{false};
    std::atomic<bool> stopThread_{false};
    bool simulationMode_{false};
    
    // 网络
#ifdef __linux__
    int socketFd_{-1};
#else
    int socketFd_{-1};
#endif
    
    // 数据队列
    std::queue<RawPacket> packetQueue_;
    std::mutex packetMutex_;
    
    std::queue<PointCloud> cloudQueue_;
    std::mutex queueMutex_;
    
    // 线程
    std::thread receiveThread_;
    std::thread assemblyThread_;
    
    // 统计
    std::atomic<uint64_t> packetCount_{0};
    std::atomic<uint64_t> frameCount_{0};
    std::atomic<uint64_t> lastTimestamp_{0};
};

} // namespace falconmind::sdk::sensors
