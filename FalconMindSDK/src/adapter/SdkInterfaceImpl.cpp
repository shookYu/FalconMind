/**
 * @file SdkInterfaceImpl.cpp
 * @brief SDK 接口实现（适配器层）
 * 
 * 将 SDK 现有能力包装为 NodeAgent 接口
 * 编译为独立共享库：libfalconmind_sdk_interface.so
 */

#include "falconmind/sdk/plugin/BuiltinPlugins.h"
#include "falconmind/sdk/plugin/CapabilityRegistry.h"
#include "falconmind/sdk/plugin/PluginManager.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/flight/FlightNodes.h"
#include "falconmind/sdk/mission/BehaviorTree.h"
#include "falconmind/sdk/mission/SearchPathPlanner.h"
#include "falconmind/sdk/perception/PerceptionPipeline.h"
#include "falconmind/sdk/telemetry/TelemetryPublisher.h"
#include "falconmind/sdk/core/Logger.h"

// 包含 NodeAgent 接口定义
#include "../../../../NodeAgent/include/nodeagent/sdk/SdkInterface.h"

#include <memory>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>

using namespace falconmind::sdk;
using namespace nodeagent::sdk;

namespace falconmind {
namespace sdk {
namespace adapter {

//==============================================================================
// SDK 上下文实现
//==============================================================================

class SdkContextImpl : public ISdkContext {
public:
    SdkContextImpl() : initialized_(false) {}
    
    bool initialize(const SdkInitConfig& config) override {
        if (initialized_) {
            return true;
        }
        
        // 设置日志级别
        Logger::setLogLevel(static_cast<LogLevel>(config.logLevel));
        
        // 初始化插件系统
        auto& registry = plugin::CapabilityRegistry::instance();
        registry.initialize(config.pluginDir ? config.pluginDir : "./plugins");
        
        // 注册内建插件
        plugin::registerBuiltinPlugins();
        
        initialized_ = true;
        return true;
    }
    
    void shutdown() override {
        if (!initialized_) {
            return;
        }
        
        auto& registry = plugin::CapabilityRegistry::instance();
        registry.shutdown();
        
        initialized_ = false;
    }
    
    int getInterfaceVersion() const override {
        return FALCONMIND_SDK_INTERFACE_VERSION;
    }
    
    const char* getSdkVersion() const override {
        return "1.0.0";
    }
    
    std::vector<std::string> getCapabilities() const override {
        return {
            "detection", "tracking", "navigation", "mission_planning",
            "flight_control", "visual_guidance", "telemetry"
        };
    }
    
    bool hasCapability(const char* name) const override {
        auto caps = getCapabilities();
        for (const auto& cap : caps) {
            if (cap == name) return true;
        }
        return false;
    }
    
private:
    std::atomic<bool> initialized_;
};

//==============================================================================
// 飞控连接服务适配器
//==============================================================================

class FlightConnectionServiceAdapter : public IFlightConnectionService {
public:
    FlightConnectionServiceAdapter() 
        : flightService_(std::make_unique<flight::FlightConnectionService>()) {}
    
    bool connect(const FlightConnectionConfig& config) override {
        flight::FlightConnectionConfig sdkConfig;
        sdkConfig.connectionType = config.connectionType;
        sdkConfig.remoteAddress = config.remoteAddress;
        sdkConfig.remotePort = config.remotePort;
        sdkConfig.baudRate = config.baudRate;
        sdkConfig.timeoutMs = config.timeoutMs;
        
        return flightService_->connect(sdkConfig);
    }
    
    void disconnect() override {
        flightService_->disconnect();
    }
    
    bool isConnected() const override {
        return flightService_->isConnected();
    }
    
    bool arm(bool arm = true) override {
        if (arm) {
            return flightService_->arm();
        } else {
            return flightService_->disarm();
        }
    }
    
    bool takeoff(float altitude) override {
        return flightService_->takeoff(altitude);
    }
    
    bool land() override {
        return flightService_->land();
    }
    
    bool returnToLaunch() override {
        return flightService_->returnToLaunch();
    }
    
    bool uploadMission(const std::vector<Waypoint>& waypoints) override {
        std::vector<flight::Waypoint> sdkWaypoints;
        for (const auto& wp : waypoints) {
            flight::Waypoint sdkWp;
            sdkWp.latitude = wp.latitude;
            sdkWp.longitude = wp.longitude;
            sdkWp.altitude = wp.altitude;
            sdkWp.speed = wp.speed;
            sdkWp.holdTime = wp.holdTime;
            sdkWp.type = wp.type;
            sdkWaypoints.push_back(sdkWp);
        }
        return flightService_->uploadMission(sdkWaypoints);
    }
    
    bool startMission() override {
        return flightService_->startMission();
    }
    
    bool pauseMission() override {
        return flightService_->pauseMission();
    }
    
    bool resumeMission() override {
        return flightService_->resumeMission();
    }
    
    bool getVehicleState(VehicleState& state) override {
        flight::VehicleState sdkState;
        if (!flightService_->getVehicleState(sdkState)) {
            return false;
        }
        
        state.latitude = sdkState.latitude;
        state.longitude = sdkState.longitude;
        state.altitude = sdkState.altitude;
        state.heading = sdkState.heading;
        state.groundSpeed = sdkState.groundSpeed;
        state.airSpeed = sdkState.airSpeed;
        state.batteryPercent = sdkState.batteryPercent;
        state.flightMode = sdkState.flightMode;
        state.isArmed = sdkState.isArmed;
        state.isFlying = sdkState.isFlying;
        state.timestampMs = sdkState.timestampMs;
        
        return true;
    }
    
    void setStateCallback(StateCallback callback) override {
        stateCallback_ = callback;
        
        if (callback) {
            flightService_->setStateCallback([this](const flight::VehicleState& sdkState) {
                VehicleState state;
                // ... 转换代码同上
                stateCallback_(state);
            });
        }
    }
    
    bool gotoPosition(double lat, double lon, double alt) override {
        return flightService_->gotoPosition(lat, lon, alt);
    }
    
    bool setVelocity(float vx, float vy, float vz, float yawRate) override {
        return flightService_->setVelocity(vx, vy, vz, yawRate);
    }
    
private:
    std::unique_ptr<flight::FlightConnectionService> flightService_;
    StateCallback stateCallback_;
};

//==============================================================================
// 任务执行服务适配器
//==============================================================================

class MissionExecutionServiceAdapter : public IMissionExecutionService {
public:
    MissionExecutionServiceAdapter() : nextMissionId_(1) {}
    
    bool createSearchMission(
        const char* missionId,
        const SearchMissionParams& params,
        IFlightConnectionService* flightService
    ) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (missions_.find(missionId) != missions_.end()) {
            return false; // 任务已存在
        }
        
        auto mission = std::make_unique<MissionState>();
        mission->id = missionId;
        mission->status = MissionStatus::Pending;
        mission->progress = 0;
        
        // 转换搜索区域
        mission::SearchArea searchArea;
        for (const auto& vertex : params.area.polygon) {
            searchArea.polygon.push_back({vertex.first, vertex.second});
        }
        searchArea.minAltitude = params.area.minAltitude;
        searchArea.maxAltitude = params.area.maxAltitude;
        
        // 使用 SDK 的 SearchPathPlanner 生成航点
        mission::SearchPathPlanner planner;
        std::vector<flight::Waypoint> waypoints;
        
        bool success = planner.generateWaypoints(
            searchArea,
            params.pattern,
            params.altitude,
            params.speed,
            params.overlapRatio,
            waypoints
        );
        
        if (!success) {
            return false;
        }
        
        // 创建行为树
        auto root = std::make_shared<mission::SequenceNode>(missionId);
        
        // 添加起飞节点
        root->addChild(std::make_shared<flight::TakeoffNode>(params.altitude));
        
        // 添加航点执行节点
        auto waypointNode = std::make_shared<flight::WaypointMissionNode>();
        waypointNode->setWaypoints(waypoints);
        root->addChild(waypointNode);
        
        // 如果启用了检测，添加检测循环节点
        if (params.enableDetection) {
            auto detectionLoop = std::make_shared<DetectionLoopNode>();
            detectionLoop->setTargetClasses(params.targetClasses);
            root->addChild(detectionLoop);
        }
        
        // 添加返航节点
        root->addChild(std::make_shared<flight::ReturnToLaunchNode>());
        
        // 创建执行器
        mission->executor = std::make_shared<mission::BehaviorTreeExecutor>(root);
        mission->flightServiceAdapter = static_cast<FlightConnectionServiceAdapter*>(flightService);
        
        missions_[missionId] = std::move(mission);
        return true;
    }
    
    bool startMission(const char* missionId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = missions_.find(missionId);
        if (it == missions_.end()) {
            return false;
        }
        
        auto& mission = it->second;
        mission->status = MissionStatus::Running;
        mission->executor->start();
        
        // 启动执行线程
        mission->executionThread = std::thread([this, missionId]() {
            this->executeMissionLoop(missionId);
        });
        
        return true;
    }
    
    bool pauseMission(const char* missionId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = missions_.find(missionId);
        if (it == missions_.end()) {
            return false;
        }
        
        it->second->status = MissionStatus::Paused;
        it->second->executor->pause();
        return true;
    }
    
    bool resumeMission(const char* missionId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = missions_.find(missionId);
        if (it == missions_.end()) {
            return false;
        }
        
        it->second->status = MissionStatus::Running;
        it->second->executor->resume();
        return true;
    }
    
    bool cancelMission(const char* missionId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = missions_.find(missionId);
        if (it == missions_.end()) {
            return false;
        }
        
        it->second->status = MissionStatus::Cancelled;
        it->second->executor->stop();
        return true;
    }
    
    MissionStatus getMissionStatus(const char* missionId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = missions_.find(missionId);
        if (it == missions_.end()) {
            return MissionStatus::Failed;
        }
        
        return it->second->status;
    }
    
    int getMissionProgress(const char* missionId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = missions_.find(missionId);
        if (it == missions_.end()) {
            return 0;
        }
        
        return it->second->progress;
    }
    
    void setMissionCallback(MissionCallback callback) override {
        missionCallback_ = callback;
    }
    
    void destroyMission(const char* missionId) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = missions_.find(missionId);
        if (it != missions_.end()) {
            it->second->executor->stop();
            if (it->second->executionThread.joinable()) {
                it->second->executionThread.join();
            }
            missions_.erase(it);
        }
    }
    
private:
    struct MissionState {
        std::string id;
        MissionStatus status;
        int progress;
        std::shared_ptr<mission::BehaviorTreeExecutor> executor;
        FlightConnectionServiceAdapter* flightServiceAdapter;
        std::thread executionThread;
    };
    
    void executeMissionLoop(const std::string& missionId) {
        auto it = missions_.find(missionId);
        if (it == missions_.end()) {
            return;
        }
        
        auto& mission = it->second;
        
        while (mission->status == MissionStatus::Running) {
            auto status = mission->executor->tick();
            
            // 更新进度
            mission->progress = mission->executor->getProgress();
            
            // 回调
            if (missionCallback_) {
                missionCallback_(missionId.c_str(), mission->status, mission->progress);
            }
            
            // 检查是否完成
            if (status == mission::NodeStatus::Success) {
                mission->status = MissionStatus::Completed;
                break;
            } else if (status == mission::NodeStatus::Failure) {
                mission->status = MissionStatus::Failed;
                break;
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        if (missionCallback_) {
            missionCallback_(missionId.c_str(), mission->status, mission->progress);
        }
    }
    
    std::mutex mutex_;
    std::map<std::string, std::unique_ptr<MissionState>> missions_;
    MissionCallback missionCallback_;
    int nextMissionId_;
};

//==============================================================================
// SDK 服务工厂实现
//==============================================================================

class SdkServiceFactory : public ISdkServiceFactory {
public:
    ISdkContext* createContext() override {
        return new SdkContextImpl();
    }
    
    IFlightConnectionService* createFlightConnectionService() override {
        return new FlightConnectionServiceAdapter();
    }
    
    IMissionExecutionService* createMissionExecutionService() override {
        return new MissionExecutionServiceAdapter();
    }
    
    IDetectionService* createDetectionService() override {
        // TODO: 实现检测服务适配器
        return nullptr;
    }
    
    ITelemetryService* createTelemetryService() override {
        // TODO: 实现遥测服务适配器
        return nullptr;
    }
    
    void destroyService(void* service) override {
        delete static_cast<ISdkContext*>(service);
    }
};

} // namespace adapter
} // namespace sdk
} // namespace falconmind

//==============================================================================
// C 接口导出实现
//==============================================================================

using namespace falconmind::sdk::adapter;

static std::unique_ptr<SdkServiceFactory> g_factory;
static std::mutex g_initMutex;
static bool g_initialized = false;

int FalconMindSdk_GetInterfaceVersion() {
    return FALCONMIND_SDK_INTERFACE_VERSION;
}

nodeagent::sdk::ISdkServiceFactory* FalconMindSdk_CreateServiceFactory() {
    if (!g_factory) {
        g_factory = std::make_unique<SdkServiceFactory>();
    }
    return g_factory.get();
}

void FalconMindSdk_DestroyServiceFactory(nodeagent::sdk::ISdkServiceFactory* factory) {
    // 工厂是单例，不需要销毁
}

const char* FalconMindSdk_GetVersion() {
    return "1.0.0";
}

bool FalconMindSdk_Initialize(const char* pluginDir) {
    std::lock_guard<std::mutex> lock(g_initMutex);
    
    if (g_initialized) {
        return true;
    }
    
    // 初始化 SDK 核心
    Logger::initialize();
    
    g_initialized = true;
    return true;
}

void FalconMindSdk_Shutdown() {
    std::lock_guard<std::mutex> lock(g_initMutex);
    
    if (!g_initialized) {
        return;
    }
    
    // 清理 SDK 核心
    Logger::shutdown();
    
    g_factory.reset();
    g_initialized = false;
}
