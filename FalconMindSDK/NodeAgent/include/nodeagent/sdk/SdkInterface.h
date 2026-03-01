/**
 * @file SdkInterface.h
 * @brief NodeAgent 与 SDK 的解耦接口定义
 * 
 * 此文件定义了 NodeAgent 调用 SDK 能力的 C 接口契约。
 * NodeAgent 不直接包含 SDK 头文件，只使用此接口。
 * SDK 实现此接口并通过动态库导出。
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>
#include <memory>

// 接口版本号（用于兼容性检查）
#define FALCONMIND_SDK_INTERFACE_VERSION 1

namespace nodeagent {
namespace sdk {

// 前向声明（不透明指针类型）
struct SdkContext;
struct FlightServiceHandle;
struct DetectionServiceHandle;
struct MissionServiceHandle;
struct TelemetryServiceHandle;

using SdkContextPtr = void*;
using FlightServicePtr = void*;
using DetectionServicePtr = void*;
using MissionServicePtr = void*;
using TelemetryServicePtr = void*;

//==============================================================================
// 1. SDK 生命周期管理接口
//==============================================================================

/**
 * @brief SDK 初始化配置
 */
struct SdkInitConfig {
    const char* pluginDir;          // 插件目录路径
    const char* configFilePath;     // 配置文件路径
    int logLevel;                   // 日志级别 (0-4)
    void* userData;                 // 用户自定义数据
};

/**
 * @brief SDK 上下文接口
 * 
 * 每个 NodeAgent 实例创建一个 SdkContext，用于管理所有 SDK 资源
 */
class ISdkContext {
public:
    virtual ~ISdkContext() = default;
    
    // 初始化 SDK
    virtual bool initialize(const SdkInitConfig& config) = 0;
    
    // 关闭 SDK
    virtual void shutdown() = 0;
    
    // 获取接口版本
    virtual int getInterfaceVersion() const = 0;
    
    // 获取 SDK 版本信息
    virtual const char* getSdkVersion() const = 0;
    
    // 获取能力列表（检测、跟踪、导航等）
    virtual std::vector<std::string> getCapabilities() const = 0;
    
    // 检查是否支持某能力
    virtual bool hasCapability(const char* name) const = 0;
};

//==============================================================================
// 2. 飞控连接服务接口（替代 FlightConnectionService）
//==============================================================================

/**
 * @brief 飞控连接配置
 */
struct FlightConnectionConfig {
    const char* connectionType;     // "udp", "tcp", "serial"
    const char* remoteAddress;      // IP 地址或串口设备
    int remotePort;                 // 端口号（UDP/TCP）
    int baudRate;                   // 波特率（串口）
    int timeoutMs;                  // 连接超时（毫秒）
};

/**
 * @brief 航点结构
 */
struct Waypoint {
    double latitude;
    double longitude;
    double altitude;
    float speed;
    int holdTime;                   // 悬停时间（秒）
    int type;                       // 航点类型（0=普通, 1=起飞, 2=降落等）
};

/**
 * @brief 飞行器状态
 */
struct VehicleState {
    double latitude;
    double longitude;
    double altitude;
    float heading;
    float groundSpeed;
    float airSpeed;
    float batteryPercent;
    int flightMode;                 // 飞行模式
    bool isArmed;
    bool isFlying;
    uint64_t timestampMs;
};

/**
 * @brief 飞控连接服务接口
 * 
 * 封装 MAVLink 通信，提供统一的操作接口
 */
class IFlightConnectionService {
public:
    virtual ~IFlightConnectionService() = default;
    
    // 连接到飞控
    virtual bool connect(const FlightConnectionConfig& config) = 0;
    
    // 断开连接
    virtual void disconnect() = 0;
    
    // 检查是否已连接
    virtual bool isConnected() const = 0;
    
    // 解锁/锁定电机
    virtual bool arm(bool arm = true) = 0;
    
    // 起飞
    virtual bool takeoff(float altitude) = 0;
    
    // 降落
    virtual bool land() = 0;
    
    // 返航
    virtual bool returnToLaunch() = 0;
    
    // 上传航点任务
    virtual bool uploadMission(const std::vector<Waypoint>& waypoints) = 0;
    
    // 开始执行任务
    virtual bool startMission() = 0;
    
    // 暂停任务
    virtual bool pauseMission() = 0;
    
    // 继续任务
    virtual bool resumeMission() = 0;
    
    // 获取当前飞行器状态
    virtual bool getVehicleState(VehicleState& state) = 0;
    
    // 设置状态回调
    using StateCallback = std::function<void(const VehicleState&)>;
    virtual void setStateCallback(StateCallback callback) = 0;
    
    // 移动到指定位置（GUIDED 模式）
    virtual bool gotoPosition(double lat, double lon, double alt) = 0;
    
    // 设置速度（GUIDED 模式）
    virtual bool setVelocity(float vx, float vy, float vz, float yawRate) = 0;
};

//==============================================================================
// 3. 任务执行服务接口（替代 BehaviorTree 直接调用）
//==============================================================================

/**
 * @brief 搜索区域定义
 */
struct SearchArea {
    std::vector<std::pair<double, double>> polygon;  // 多边形顶点（经纬度）
    double minAltitude;
    double maxAltitude;
};

/**
 * @brief 搜索任务参数
 */
struct SearchMissionParams {
    SearchArea area;
    const char* pattern;            // "LAWN_MOWER", "SPIRAL", "ZIGZAG"
    double altitude;
    float speed;
    float overlapRatio;
    bool enableDetection;
    std::vector<const char*> targetClasses;
    float confidenceThreshold;
};

/**
 * @brief 任务状态
 */
enum class MissionStatus {
    Pending,
    Running,
    Paused,
    Completed,
    Failed,
    Cancelled
};

/**
 * @brief 任务执行服务接口
 */
class IMissionExecutionService {
public:
    virtual ~IMissionExecutionService() = default;
    
    // 创建搜索任务
    virtual bool createSearchMission(
        const char* missionId,
        const SearchMissionParams& params,
        IFlightConnectionService* flightService
    ) = 0;
    
    // 开始执行任务
    virtual bool startMission(const char* missionId) = 0;
    
    // 暂停任务
    virtual bool pauseMission(const char* missionId) = 0;
    
    // 恢复任务
    virtual bool resumeMission(const char* missionId) = 0;
    
    // 取消任务
    virtual bool cancelMission(const char* missionId) = 0;
    
    // 获取任务状态
    virtual MissionStatus getMissionStatus(const char* missionId) = 0;
    
    // 获取任务进度 (0-100)
    virtual int getMissionProgress(const char* missionId) = 0;
    
    // 设置任务状态回调
    using MissionCallback = std::function<void(const char* missionId, MissionStatus status, int progress)>;
    virtual void setMissionCallback(MissionCallback callback) = 0;
    
    // 销毁任务
    virtual void destroyMission(const char* missionId) = 0;
};

//==============================================================================
// 4. 目标检测服务接口
//==============================================================================

/**
 * @brief 检测目标结构
 */
struct DetectionTarget {
    const char* className;          // "person", "vehicle", etc.
    float confidence;               // 置信度 (0-1)
    float bboxX;                    // 边界框中心 X (归一化 0-1)
    float bboxY;                    // 边界框中心 Y (归一化 0-1)
    float bboxWidth;                // 边界框宽度 (归一化)
    float bboxHeight;               // 边界框高度 (归一化)
    double latitude;                // GPS 纬度（转换后）
    double longitude;               // GPS 经度（转换后）
    int trackId;                    // 跟踪 ID (-1 表示未跟踪)
    uint64_t timestampMs;
};

/**
 * @brief 检测结果回调
 */
using DetectionCallback = std::function<void(const DetectionTarget*, int numTargets)>;

/**
 * @brief 目标检测服务接口
 */
class IDetectionService {
public:
    virtual ~IDetectionService() = default;
    
    // 初始化检测器
    virtual bool initialize(
        const char* modelPath,
        const char* labelPath,
        float confidenceThreshold
    ) = 0;
    
    // 设置检测结果回调
    virtual void setDetectionCallback(DetectionCallback callback) = 0;
    
    // 开始检测（异步）
    virtual bool startDetection(IFlightConnectionService* flightService) = 0;
    
    // 停止检测
    virtual void stopDetection() = 0;
    
    // 是否正在检测
    virtual bool isDetecting() const = 0;
    
    // 设置目标类别过滤
    virtual void setTargetClasses(const std::vector<const char*>& classes) = 0;
};

//==============================================================================
// 5. 遥测上报服务接口
//==============================================================================

/**
 * @brief 遥测数据包
 */
struct TelemetryPacket {
    const char* uavId;
    VehicleState vehicleState;
    int numTargets;
    const DetectionTarget* targets;
    uint64_t timestampMs;
};

/**
 * @brief 遥测服务接口
 */
class ITelemetryService {
public:
    virtual ~ITelemetryService() = default;
    
    // 初始化遥测服务
    virtual bool initialize(
        const char* brokerHost,
        int brokerPort,
        const char* uavId
    ) = 0;
    
    // 上报遥测数据
    virtual bool publishTelemetry(const TelemetryPacket& packet) = 0;
    
    // 上报检测目标
    virtual bool publishDetection(const DetectionTarget& target) = 0;
    
    // 设置上报间隔（毫秒）
    virtual void setPublishInterval(int intervalMs) = 0;
    
    // 停止遥测服务
    virtual void stop() = 0;
};

//==============================================================================
// 6. SDK 工厂函数（动态加载入口）
//==============================================================================

/**
 * @brief SDK 服务工厂接口
 * 
 * NodeAgent 通过此工厂获取各种服务实例
 */
class ISdkServiceFactory {
public:
    virtual ~ISdkServiceFactory() = default;
    
    // 创建 SDK 上下文
    virtual ISdkContext* createContext() = 0;
    
    // 创建飞控连接服务
    virtual IFlightConnectionService* createFlightConnectionService() = 0;
    
    // 创建任务执行服务
    virtual IMissionExecutionService* createMissionExecutionService() = 0;
    
    // 创建检测服务
    virtual IDetectionService* createDetectionService() = 0;
    
    // 创建遥测服务
    virtual ITelemetryService* createTelemetryService() = 0;
    
    // 销毁服务实例
    virtual void destroyService(void* service) = 0;
};

} // namespace sdk
} // namespace nodeagent

//==============================================================================
// C 接口导出（用于动态加载）
//==============================================================================

extern "C" {

/**
 * @brief 获取 SDK 接口版本
 */
int FalconMindSdk_GetInterfaceVersion();

/**
 * @brief 创建 SDK 服务工厂
 * 
 * NodeAgent 首先调用此函数获取工厂，然后通过工厂创建各种服务
 */
nodeagent::sdk::ISdkServiceFactory* FalconMindSdk_CreateServiceFactory();

/**
 * @brief 销毁 SDK 服务工厂
 */
void FalconMindSdk_DestroyServiceFactory(nodeagent::sdk::ISdkServiceFactory* factory);

/**
 * @brief 获取 SDK 版本字符串
 */
const char* FalconMindSdk_GetVersion();

/**
 * @brief 初始化 SDK（全局初始化，只需调用一次）
 */
bool FalconMindSdk_Initialize(const char* pluginDir);

/**
 * @brief 关闭 SDK（全局清理）
 */
void FalconMindSdk_Shutdown();

} // extern "C"
