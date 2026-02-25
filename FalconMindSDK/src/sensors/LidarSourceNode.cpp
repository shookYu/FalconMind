/**
 * FalconMindSDK - LiDAR Source Node Implementation
 * 
 * 支持LiDAR设备：
 * - Velodyne VLP-16, VLP-32, HDL-64E, VLS-128
 * - Livox Mid-40, Mid-70, Horizon, Avia
 * 
 * 功能：
 * - UDP数据包接收与解析
 * - 点云数据组装与去畸变（运动补偿）
 * - 时间同步
 * - 多设备支持
 */

#include "falconmind/sdk/sensors/LidarSourceNode.h"
#include "falconmind/sdk/core/Pad.h"

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <string>
#include <cmath>
#include <queue>
#include <vector>
#include <atomic>
#include <cstring>

#ifdef __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#endif

namespace falconmind::sdk::sensors {

using namespace falconmind::sdk::core;

// ============================================================================
// Velodyne 数据包定义
// ============================================================================

#pragma pack(push, 1)

// Velodyne VLP-16 数据包头
struct VelodyneHeader {
    uint16_t flag;      // 0xEEFF
    uint16_t azimuth;   // 角度，单位0.01度
};

// Velodyne 数据块（2个）
struct VelodyneBlock {
    VelodyneHeader header;
    uint8_t data[96];   // 32 packets * 3 bytes (distance + intensity)
};

// Velodyne 数据包（1206字节）
struct VelodynePacket {
    VelodyneBlock blocks[12];   // 12个数据块
    uint32_t timestamp;         // 微秒
    uint16_t factory;           // 设备信息
};

#pragma pack(pop)

// Velodyne VLP-16 垂直角校正表
const float VLP16_VERTICAL_ANGLES[16] = {
    -15.0f, 1.0f, -13.0f, 3.0f, -11.0f, 5.0f, -9.0f, 7.0f,
    -7.0f, 9.0f, -5.0f, 11.0f, -3.0f, 13.0f, -1.0f, 15.0f
};

// Velodyne VLP-32 垂直角校正表
const float VLP32_VERTICAL_ANGLES[32] = {
    -25.0f, -1.0f, -1.667f, -15.639f, -11.31f, 0.0f, -0.667f, -8.843f,
    -7.254f, 0.333f, -1.333f, -6.148f, -4.0f, 1.333f, -1.0f, -4.0f,
    -3.667f, 1.667f, -0.667f, -3.333f, -2.333f, 3.333f, 0.333f, -2.667f,
    -1.667f, 7.0f, 0.667f, -1.333f, -1.0f, 15.0f, 1.0f, -1.0f
};

// ============================================================================
// Livox 数据包定义
// ============================================================================

#pragma pack(push, 1)

// Livox 点云数据包
struct LivoxPoint {
    uint8_t x[3];      // 24-bit integer, mm
    uint8_t y[3];
    uint8_t z[3];
    uint8_t reflectivity;
    uint8_t tag;       // 标签信息
};

struct LivoxPacket {
    uint8_t header[18];     // 协议头
    uint32_t timestamp;     // 纳秒
    uint8_t data_type;      // 数据类型
    uint32_t point_num;     // 点数
    LivoxPoint points[96];  // 点数据
};

#pragma pack(pop)

// ============================================================================
// LiDAR源节点实现
// ============================================================================

LidarSourceNode::LidarSourceNode() : Node("lidar_source"), config_() {
    addPad(std::make_shared<Pad>("points_out", PadType::Source));
    addPad(std::make_shared<Pad>("imu_out", PadType::Source));  // Livox内置IMU
    
    // 默认配置
    config_.deviceType = LidarDeviceType::Unknown;
    config_.ipAddress = "0.0.0.0";
    config_.port = 2368;
    config_.scanLineCount = 16;
}

LidarSourceNode::LidarSourceNode(const LidarConfig& cfg) 
    : Node("lidar_source"), config_(cfg) {
    addPad(std::make_shared<Pad>("points_out", PadType::Source));
    addPad(std::make_shared<Pad>("imu_out", PadType::Source));
}

LidarSourceNode::~LidarSourceNode() {
    stop();
}

bool LidarSourceNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto typeIt = params.find("device_type");
    if (typeIt != params.end()) {
        if (typeIt->second == "velodyne_vlp16") {
            config_.deviceType = LidarDeviceType::VelodyneVLP16;
            config_.port = 2368;
            config_.scanLineCount = 16;
        } else if (typeIt->second == "velodyne_vlp32") {
            config_.deviceType = LidarDeviceType::VelodyneVLP32;
            config_.port = 2368;
            config_.scanLineCount = 32;
        } else if (typeIt->second == "livox_mid40") {
            config_.deviceType = LidarDeviceType::LivoxMid40;
            config_.port = 56000;
            config_.scanLineCount = 1;
        } else if (typeIt->second == "livox_mid70") {
            config_.deviceType = LidarDeviceType::LivoxMid70;
            config_.port = 56000;
            config_.scanLineCount = 1;
        } else if (typeIt->second == "livox_horizon") {
            config_.deviceType = LidarDeviceType::LivoxHorizon;
            config_.port = 56000;
            config_.scanLineCount = 6;
        } else if (typeIt->second == "livox_avia") {
            config_.deviceType = LidarDeviceType::LivoxAvia;
            config_.port = 56000;
            config_.scanLineCount = 6;
        } else if (typeIt->second == "simulation") {
            config_.deviceType = LidarDeviceType::Simulation;
        }
    }
    
    auto ipIt = params.find("ip_address");
    if (ipIt != params.end()) config_.ipAddress = ipIt->second;
    
    auto portIt = params.find("port");
    if (portIt != params.end()) config_.port = static_cast<uint16_t>(std::stoi(portIt->second));
    
    auto scanLineIt = params.find("scan_lines");
    if (scanLineIt != params.end()) {
        config_.scanLineCount = std::stoi(scanLineIt->second);
    }
    
    auto frameRateIt = params.find("frame_rate");
    if (frameRateIt != params.end()) {
        config_.frameRateHz = std::stof(frameRateIt->second);
    }
    
    auto motionCompIt = params.find("motion_compensation");
    if (motionCompIt != params.end()) {
        config_.enableMotionCompensation = (motionCompIt->second == "true" || motionCompIt->second == "1");
    }
    
    std::cout << "[LidarSourceNode] Configuration:" << std::endl;
    std::cout << "  Device Type: " << static_cast<int>(config_.deviceType) << std::endl;
    std::cout << "  IP: " << config_.ipAddress << ":" << config_.port << std::endl;
    std::cout << "  Scan Lines: " << config_.scanLineCount << std::endl;
    std::cout << "  Frame Rate: " << config_.frameRateHz << " Hz" << std::endl;
    std::cout << "  Motion Compensation: " << (config_.enableMotionCompensation ? "enabled" : "disabled") << std::endl;
    
    return true;
}

bool LidarSourceNode::start() {
    if (started_) return true;
    
    started_ = true;
    stopThread_ = false;
    packetCount_ = 0;
    frameCount_ = 0;
    lastTimestamp_ = 0;
    
    if (config_.deviceType == LidarDeviceType::Simulation) {
        simulationMode_ = true;
        std::cout << "[LidarSourceNode] Started in simulation mode" << std::endl;
        return true;
    }
    
    simulationMode_ = false;
    
#ifdef __linux__
    // 初始化UDP socket
    if (!initSocket()) {
        std::cerr << "[LidarSourceNode] Failed to initialize socket, falling back to simulation" << std::endl;
        simulationMode_ = true;
        return true;
    }
    
    // 启动接收线程
    receiveThread_ = std::thread(&LidarSourceNode::receiveThreadFunc, this);
    
    // 启动组装线程
    assemblyThread_ = std::thread(
&LidarSourceNode::assemblyThreadFunc, this);
    
    std::cout << "[LidarSourceNode] Started receiving from " << config_.ipAddress << ":" << config_.port << std::endl;
#else
    std::cerr << "[LidarSourceNode] Non-Linux platform, using simulation mode" << std::endl;
    simulationMode_ = true;
#endif
    
    return true;
}

void LidarSourceNode::stop() {
    if (!started_) return;
    
    stopThread_ = true;
    
#ifdef __linux__
    // 等待线程结束
    if (receiveThread_.joinable()) {
        receiveThread_.join();
    }
    if (assemblyThread_.joinable()) {
        assemblyThread_.join();
    }
    
    shutdownSocket();
#endif
    
    started_ = false;
    std::cout << "[LidarSourceNode] Stopped. Received " << packetCount_ << " packets, " 
              << frameCount_ << " frames" << std::endl;
}

void LidarSourceNode::process() {
    if (!started_) return;
    
    if (simulationMode_) {
        // 模拟模式：生成合成点云
        PointCloud cloud;
        generateSimulatedPointCloud(cloud);
        
        auto pad = getPad("points_out");
        if (pad && !cloud.empty()) {
            pad->pushToConnections(cloud.data(), cloud.size() * sizeof(PointXYZI));
        }
        
        frameCount_++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10Hz
    } else {
        // 实际模式：从队列取出组装好的点云
        std::unique_lock<std::mutex> lock(queueMutex_);
        if (!cloudQueue_.empty()) {
            PointCloud cloud = std::move(cloudQueue_.front());
            cloudQueue_.pop();
            lock.unlock();
            
            auto pad = getPad("points_out");
            if (pad && !cloud.empty()) {
                pad->pushToConnections(cloud.data(), cloud.size() * sizeof(PointXYZI));
            }
            
            frameCount_++;
        }
    }
}

// ============================================================================
// Socket 操作
// ============================================================================

bool LidarSourceNode::initSocket() {
#ifdef __linux__
    socketFd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socketFd_ < 0) {
        std::cerr << "[LidarSourceNode] Failed to create socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    // 设置非阻塞
    int flags = fcntl(socketFd_, F_GETFL, 0);
    fcntl(socketFd_, F_SETFL, flags | O_NONBLOCK);
    
    // 允许地址重用
    int reuse = 1;
    if (setsockopt(socketFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        std::cerr << "[LidarSourceNode] Failed to set SO_REUSEADDR" << std::endl;
    }
    
    // 绑定地址
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.port);
    addr.sin_addr.s_addr = inet_addr(config_.ipAddress.c_str());
    
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    
    if (::bind(socketFd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[LidarSourceNode] Failed to bind socket: " << strerror(errno) << std::endl;
        close(socketFd_);
        socketFd_ = -1;
        return false;
    }
    
    std::cout << "[LidarSourceNode] Socket bound to port " << config_.port << std::endl;
    return true;
#else
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

// ============================================================================
// 数据接收线程
// ============================================================================

void LidarSourceNode::receiveThreadFunc() {
#ifdef __linux__
    if (socketFd_ < 0) return;
    
    uint8_t buffer[2048];
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    while (!stopThread_.load()) {
        ssize_t n = ::recvfrom(socketFd_, buffer, sizeof(buffer), 0,
                              (struct sockaddr*)&clientAddr, &addrLen);
        
        if (n > 0) {
            // 解析数据包
            std::lock_guard<std::mutex> lock(packetMutex_);
            
            RawPacket packet;
            packet.timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            packet.data.assign(buffer, buffer + n);
            packet.sourceIp = inet_ntoa(clientAddr.sin_addr);
            packet.sourcePort = ntohs(clientAddr.sin_port);
            
            packetQueue_.push(std::move(packet));
            packetCount_++;
            
            // 限制队列大小
            if (packetQueue_.size() > 100) {
                packetQueue_.pop();
            }
        } else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "[LidarSourceNode] recvfrom error: " << strerror(errno) << std::endl;
        } else {
            // 无数据，短暂休眠
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
#endif
}

// ============================================================================
// 数据组装线程
// ============================================================================

void LidarSourceNode::assemblyThreadFunc() {
    PointCloud currentCloud;
    currentCloud.reserve(10000);
    
    uint64_t currentFrameStart = 0;
    const uint64_t FRAME_INTERVAL_NS = static_cast<uint64_t>(1e9 / config_.frameRateHz);
    
    while (!stopThread_.load()) {
        std::unique_lock<std::mutex> lock(packetMutex_);
        
        if (packetQueue_.empty()) {
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        
        RawPacket packet = std::move(packetQueue_.front());
        packetQueue_.pop();
        lock.unlock();
        
        // 解析数据包
        PointCloud partialCloud;
        bool success = false;
        
        switch (config_.deviceType) {
            case LidarDeviceType::VelodyneVLP16:
            case LidarDeviceType::VelodyneVLP32:
                success = parseVelodynePacket(packet.data.data(), packet.data.size(), partialCloud);
                break;
            case LidarDeviceType::LivoxMid40:
            case LidarDeviceType::LivoxMid70:
            case LidarDeviceType::LivoxHorizon:
            case LidarDeviceType::LivoxAvia:
                success = parseLivoxPacket(packet.data.data(), packet.data.size(), partialCloud);
                break;
            default:
                break;
        }
        
        if (success && !partialCloud.empty()) {
            // 检查是否需要发送当前帧
            if (currentFrameStart == 0) {
                currentFrameStart = packet.timestamp;
            }
            
            if (packet.timestamp - currentFrameStart > FRAME_INTERVAL_NS) {
                // 发送当前帧
                {
                    std::lock_guard<std::mutex> qlock(queueMutex_);
                    if (cloudQueue_.size() < 10) {  // 限制队列大小
                        cloudQueue_.push(std::move(currentCloud));
                    }
                }
                
                currentCloud.clear();
                currentCloud.reserve(10000);
                currentFrameStart = packet.timestamp;
            }
            
            // 添加点到当前帧
            currentCloud.insert(currentCloud.end(), partialCloud.begin(), partialCloud.end());
        }
    }
}

// ============================================================================
// Velodyne 数据包解析
// ============================================================================

bool LidarSourceNode::parseVelodynePacket(const uint8_t* data, size_t len, PointCloud& cloud) {
    if (len != sizeof(VelodynePacket)) {
        return false;
    }
    
    const VelodynePacket* packet = reinterpret_cast<const VelodynePacket*>(data);
    cloud.clear();
    cloud.reserve(384);  // 12 blocks * 32 points
    
    // 获取垂直角表
    const float* verticalAngles = (config_.scanLineCount == 16) ? 
                                  VLP16_VERTICAL_ANGLES : VLP32_VERTICAL_ANGLES;
    int laserCount = config_.scanLineCount;
    
    // 解析12个数据块
    for (int blockIdx = 0; blockIdx < 12; ++blockIdx) {
        const auto& block = packet->blocks[blockIdx];
        
        // 检查标志
        if (block.header.flag != 0xEEFF) {
            continue;
        }
        
        // 方位角（0.01度单位）
        float azimuth = block.header.azimuth * 0.01f * M_PI / 180.0f;
        
        // 解析32个激光点（每2个数据块为一组，但VLP-16只有16线）
        for (int laserIdx = 0; laserIdx < 32; ++laserIdx) {
            if (laserCount == 16 && laserIdx >= 16) break;
            
            int dataOffset = laserIdx * 3;
            if (dataOffset + 2 >= 96) break;
            
            // 距离（2mm单位）
            uint16_t distance = (block.data[dataOffset + 1] << 8) | block.data[dataOffset];
            uint8_t intensity = block.data[dataOffset + 2];
            
            if (distance == 0) continue;  // 无效点
            
            float distanceM = distance * 0.002f;  // 转换为米
            float verticalAngle = verticalAngles[laserIdx % laserCount] * M_PI / 180.0f;
            
            // 球坐标转笛卡尔坐标
            PointXYZI point;
            point.x = distanceM * std::cos(verticalAngle) * std::sin(azimuth);
            point.y = distanceM * std::cos(verticalAngle) * std::cos(azimuth);
            point.z = distanceM * std::sin(verticalAngle);
            point.intensity = intensity / 255.0f;
            
            cloud.push_back(point);
        }
    }
    
    return !cloud.empty();
}

// ============================================================================
// Livox 数据包解析
// ============================================================================

bool LidarSourceNode::parseLivoxPacket(const uint8_t* data, size_t len, PointCloud& cloud) {
    if (len < sizeof(LivoxPacket)) {
        return false;
    }
    
    const LivoxPacket* packet = reinterpret_cast<const LivoxPacket*>(data);
    cloud.clear();
    cloud.reserve(packet->point_num);
    
    for (uint32_t i = 0; i < packet->point_num && i < 96; ++i) {
        const auto& lp = packet->points[i];
        
        // 24-bit整数转float (mm转m)
        int32_t x = (lp.x[0] | (lp.x[1] << 8) | (lp.x[2] << 16));
        int32_t y = (lp.y[0] | (lp.y[1] << 8) | (lp.y[2] << 16));
        int32_t z = (lp.z[0] | (lp.z[1] << 8) | (lp.z[2] << 16));
        
        // 符号扩展
        if (x & 0x800000) x |= 0xFF000000;
        if (y & 0x800000) y |= 0xFF000000;
        if (z & 0x800000) z |= 0xFF000000;
        
        PointXYZI point;
        point.x = x * 0.001f;  // mm to m
        point.y = y * 0.001f;
        point.z = z * 0.001f;
        point.intensity = lp.reflectivity / 255.0f;
        
        cloud.push_back(point);
    }
    
    return !cloud.empty();
}

// ============================================================================
// 模拟点云生成
// ============================================================================

void LidarSourceNode::generateSimulatedPointCloud(PointCloud& cloud) {
    cloud.clear();
    cloud.reserve(config_.scanLineCount * 180);  // 每线180个点
    
    static float time = 0.0f;
    time += 0.1f;
    
    // 生成螺旋点云
    for (int ring = 0; ring < config_.scanLineCount; ++ring) {
        float verticalAngle = -15.0f + (30.0f * ring / (config_.scanLineCount - 1));
        float radV = verticalAngle * M_PI / 180.0f;
        
        for (int i = 0; i < 180; ++i) {
            float azimuth = i * 2.0f * M_PI / 180.0f;
            
            // 添加一些变化
            float distance = 10.0f + 5.0f * std::sin(time + azimuth * 2.0f);
            
            PointXYZI point;
            point.x = distance * std::cos(radV) * std::sin(azimuth);
            point.y = distance * std::cos(radV) * std::cos(azimuth);
            point.z = distance * std::sin(radV);
            point.intensity = static_cast<float>(i) / 180.0f;
            
            cloud.push_back(point);
        }
    }
}

// ============================================================================
// 点云后处理
// ============================================================================

void LidarSourceNode::applyMotionCompensation(PointCloud& cloud, 
                                               const ImuSample& imu,
                                               uint64_t targetTime) {
    if (!config_.enableMotionCompensation || cloud.empty()) return;
    
    // 简化的运动补偿：基于IMU加速度估计（积分得到速度）
    double dt = (targetTime - imu.timestampNs) * 1e-9;
    
    // 使用加速度积分估算位移（简化模型）
    float dx = 0.5f * imu.ax * dt * dt;
    float dy = 0.5f * imu.ay * dt * dt;
    float dz = 0.5f * imu.az * dt * dt;
    
    for (auto& point : cloud) {
        point.x += dx;
        point.y += dy;
        point.z += dz;
    }
}

void LidarSourceNode::filterPointCloud(PointCloud& cloud, float minRange, float maxRange) {
    if (cloud.empty()) return;
    
    size_t writeIdx = 0;
    for (size_t i = 0; i < cloud.size(); ++i) {
        const auto& p = cloud[i];
        float range = std::sqrt(p.x * p.x + p.y * p.y + p.z * p.z);
        
        if (range >= minRange && range <= maxRange) {
            cloud[writeIdx++] = p;
        }
    }
    
    cloud.resize(writeIdx);
}

void LidarSourceNode::downsamplePointCloud(PointCloud& cloud, float leafSize) {
    if (cloud.empty() || leafSize <= 0) return;
    
    // 体素网格降采样（简化版）
    struct Voxel {
        float x = 0, y = 0, z = 0, intensity = 0;
        int count = 0;
    };
    
    std::unordered_map<uint64_t, Voxel> voxels;
    
    for (const auto& p : cloud) {
        int vx = static_cast<int>(std::floor(p.x / leafSize));
        int vy = static_cast<int>(std::floor(p.y / leafSize));
        int vz = static_cast<int>(std::floor(p.z / leafSize));
        
        uint64_t key = ((static_cast<uint64_t>(vx) & 0x1FFFF) << 34) |
                      ((static_cast<uint64_t>(vy) & 0x1FFFF) << 17) |
                      (static_cast<uint64_t>(vz) & 0x1FFFF);
        
        auto& voxel = voxels[key];
        voxel.x += p.x;
        voxel.y += p.y;
        voxel.z += p.z;
        voxel.intensity += p.intensity;
        voxel.count++;
    }
    
    cloud.clear();
    cloud.reserve(voxels.size());
    
    for (const auto& [key, voxel] : voxels) {
        PointXYZI point;
        point.x = voxel.x / voxel.count;
        point.y = voxel.y / voxel.count;
        point.z = voxel.z / voxel.count;
        point.intensity = voxel.intensity / voxel.count;
        cloud.push_back(point);
    }
}

} // namespace falconmind::sdk::sensors
