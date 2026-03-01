/**
 * @file MockSdkImpl.cpp
 * @brief 模拟 SDK 实现
 * 
 * 用于演示 NodeAgent 运行时加载 SDK 的机制
 * 这是一个简化实现，展示接口契约如何工作
 */

#include "nodeagent/sdk/SdkInterface.h"
#include <iostream>
#include <string>

using namespace nodeagent::sdk;

namespace mock {

// 模拟飞控连接服务
class MockFlightService : public IFlightConnectionService {
public:
    MockFlightService() : connected_(false) {}
    
    bool connect(const FlightConnectionConfig& config) override {
        std::cout << "[MockFlight] Connecting to " << config.remoteAddress 
                  << ":" << config.remotePort << std::endl;
        connected_ = true;
        return true;
    }
    
    void disconnect() override {
        std::cout << "[MockFlight] Disconnected" << std::endl;
        connected_ = false;
    }
    
    bool isConnected() const override {
        return connected_;
    }
    
    bool arm(bool arm = true) override {
        std::cout << "[MockFlight] " << (arm ? "Armed" : "Disarmed") << std::endl;
        return true;
    }
    
    bool takeoff(float altitude) override {
        std::cout << "[MockFlight] Taking off to " << altitude << "m" << std::endl;
        return true;
    }
    
    bool land() override {
        std::cout << "[MockFlight] Landing" << std::endl;
        return true;
    }
    
    bool returnToLaunch() override {
        std::cout << "[MockFlight] Return to launch" << std::endl;
        return true;
    }
    
    bool uploadMission(const std::vector<Waypoint>& waypoints) override {
        std::cout << "[MockFlight] Uploaded " << waypoints.size() << " waypoints" << std::endl;
        return true;
    }
    
    bool startMission() override {
        std::cout << "[MockFlight] Mission started" << std::endl;
        return true;
    }
    
    bool pauseMission() override {
        std::cout << "[MockFlight] Mission paused" << std::endl;
        return true;
    }
    
    bool resumeMission() override {
        std::cout << "[MockFlight] Mission resumed" << std::endl;
        return true;
    }
    
    bool getVehicleState(VehicleState& state) override {
        state.latitude = 39.9042;
        state.longitude = 116.4074;
        state.altitude = 100.0;
        state.heading = 90.0;
        state.groundSpeed = 5.0;
        state.airSpeed = 6.0;
        state.batteryPercent = 85.0;
        state.flightMode = 0;
        state.isArmed = true;
        state.isFlying = true;
        state.timestampMs = 1234567890;
        return true;
    }
    
    void setStateCallback(StateCallback callback) override {
        callback_ = callback;
    }
    
    bool gotoPosition(double lat, double lon, double alt) override {
        std::cout << "[MockFlight] Going to: " << lat << ", " << lon << ", " << alt << std::endl;
        return true;
    }
    
    bool setVelocity(float vx, float vy, float vz, float yawRate) override {
        std::cout << "[MockFlight] Velocity set: " << vx << ", " << vy << ", " << vz << std::endl;
        return true;
    }
    
private:
    bool connected_;
    StateCallback callback_;
};

// 模拟 SDK 上下文
class MockSdkContext : public ISdkContext {
public:
    bool initialize(const SdkInitConfig& config) override {
        std::cout << "[MockSDK] Initialized with pluginDir: " 
                  << (config.pluginDir ? config.pluginDir : "default") << std::endl;
        return true;
    }
    
    void shutdown() override {
        std::cout << "[MockSDK] Shutdown" << std::endl;
    }
    
    int getInterfaceVersion() const override {
        return FALCONMIND_SDK_INTERFACE_VERSION;
    }
    
    const char* getSdkVersion() const override {
        return "1.0.0-mock";
    }
    
    std::vector<std::string> getCapabilities() const override {
        return {"flight", "mock"};
    }
    
    bool hasCapability(const char* name) const override {
        return std::string(name) == "flight" || std::string(name) == "mock";
    }
};

// 模拟服务工厂
class MockServiceFactory : public ISdkServiceFactory {
public:
    ISdkContext* createContext() override {
        return new MockSdkContext();
    }
    
    IFlightConnectionService* createFlightConnectionService() override {
        return new MockFlightService();
    }
    
    IMissionExecutionService* createMissionExecutionService() override {
        return nullptr; // 未实现
    }
    
    IDetectionService* createDetectionService() override {
        return nullptr; // 未实现
    }
    
    ITelemetryService* createTelemetryService() override {
        return nullptr; // 未实现
    }
    
    void destroyService(void* service) override {
        delete static_cast<ISdkContext*>(service);
    }
};

static MockServiceFactory g_factory;

} // namespace mock

//==============================================================================
// C 接口导出
//==============================================================================

extern "C" {

int FalconMindSdk_GetInterfaceVersion() {
    return FALCONMIND_SDK_INTERFACE_VERSION;
}

nodeagent::sdk::ISdkServiceFactory* FalconMindSdk_CreateServiceFactory() {
    return &mock::g_factory;
}

void FalconMindSdk_DestroyServiceFactory(nodeagent::sdk::ISdkServiceFactory* factory) {
    // 单例，不销毁
}

const char* FalconMindSdk_GetVersion() {
    return "1.0.0-mock";
}

bool FalconMindSdk_Initialize(const char* pluginDir) {
    std::cout << "[MockSDK] Global initialized" << std::endl;
    return true;
}

void FalconMindSdk_Shutdown() {
    std::cout << "[MockSDK] Global shutdown" << std::endl;
}

}
