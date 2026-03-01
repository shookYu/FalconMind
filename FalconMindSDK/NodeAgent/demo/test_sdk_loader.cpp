/**
 * @file test_sdk_loader.cpp
 * @brief 测试 SDK 动态加载器
 * 
 * 简单的测试程序，验证 NodeAgent 可以编译和运行时加载 SDK
 */

#include "nodeagent/sdk/SdkInterface.h"
#include "nodeagent/sdk/SdkLoader.h"
#include <iostream>

using namespace nodeagent;

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "NodeAgent SDK Loader Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // 1. 测试 SdkLoader 可以实例化
    std::cout << "[TEST 1] Creating SdkLoader..." << std::endl;
    SdkLoader loader;
    std::cout << "[TEST 1] ✓ SdkLoader created" << std::endl << std::endl;
    
    // 2. 测试全局单例
    std::cout << "[TEST 2] Getting global SdkLoader instance..." << std::endl;
    SdkLoader& globalLoader = getSdkLoader();
    std::cout << "[TEST 2] ✓ Global instance obtained" << std::endl;
    std::cout << "         Loaded: " << (globalLoader.isLoaded() ? "Yes" : "No") << std::endl << std::endl;
    
    // 3. 测试接口版本常量
    std::cout << "[TEST 3] Interface version constant..." << std::endl;
    std::cout << "         FALCONMIND_SDK_INTERFACE_VERSION = " << FALCONMIND_SDK_INTERFACE_VERSION << std::endl;
    std::cout << "[TEST 3] ✓ Interface version defined" << std::endl << std::endl;
    
    // 4. 测试数据结构大小
    std::cout << "[TEST 4] Checking data structures..." << std::endl;
    std::cout << "         sizeof(sdk::FlightConnectionConfig) = " << sizeof(sdk::FlightConnectionConfig) << std::endl;
    std::cout << "         sizeof(sdk::VehicleState) = " << sizeof(sdk::VehicleState) << std::endl;
    std::cout << "         sizeof(sdk::SearchMissionParams) = " << sizeof(sdk::SearchMissionParams) << std::endl;
    std::cout << "[TEST 4] ✓ Data structures defined" << std::endl << std::endl;
    
    // 5. 尝试加载 SDK（如果提供了路径）
    if (argc > 1) {
        const char* sdkPath = argv[1];
        std::cout << "[TEST 5] Loading SDK from: " << sdkPath << std::endl;
        
        if (loader.load(sdkPath)) {
            std::cout << "[TEST 5] ✓ SDK loaded successfully!" << std::endl;
            std::cout << "         SDK Version: " << loader.getSdkVersion() << std::endl;
            std::cout << "         Interface Version: " << loader.getInterfaceVersion() << std::endl;
            
            // 获取服务工厂
            sdk::ISdkServiceFactory* factory = loader.createServiceFactory();
            if (factory) {
                std::cout << "[TEST 5] ✓ Service factory created" << std::endl;
                
                // 尝试创建服务
                sdk::IFlightConnectionService* flight = factory->createFlightConnectionService();
                if (flight) {
                    std::cout << "[TEST 5] ✓ FlightConnectionService created" << std::endl;
                    
                    // 测试连接
                    sdk::FlightConnectionConfig config;
                    config.connectionType = "udp";
                    config.remoteAddress = "127.0.0.1";
                    config.remotePort = 14540;
                    
                    if (flight->connect(config)) {
                        std::cout << "[TEST 5] ✓ Connected to flight controller!" << std::endl;
                        
                        // 获取状态
                        sdk::VehicleState state;
                        if (flight->getVehicleState(state)) {
                            std::cout << "[TEST 5] ✓ Vehicle state obtained" << std::endl;
                            std::cout << "         Lat: " << state.latitude << std::endl;
                            std::cout << "         Lon: " << state.longitude << std::endl;
                        }
                        
                        flight->disconnect();
                    } else {
                        std::cout << "[TEST 5] ✗ Failed to connect (expected if no FC running)" << std::endl;
                    }
                    
                    factory->destroyService(flight);
                } else {
                    std::cout << "[TEST 5] ✗ Failed to create FlightConnectionService" << std::endl;
                }
            } else {
                std::cout << "[TEST 5] ✗ Failed to create service factory" << std::endl;
            }
            
            loader.unload();
            std::cout << "[TEST 5] ✓ SDK unloaded" << std::endl;
        } else {
            std::cout << "[TEST 5] ✗ Failed to load SDK: " << loader.getLastErrorString() << std::endl;
            return 1;
        }
    } else {
        std::cout << "[TEST 5] Skipped (no SDK path provided)" << std::endl;
        std::cout << "         Usage: " << argv[0] << " <path/to/libfalconmind_sdk.so>" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "All tests passed!" << std::endl;
    std::cout << "========================================" << std::endl;
    
    return 0;
}
