// Simple MAVLink IMU test
#include <falconmind/sdk/sensors/ImuSourceNode.h>
#include <falconmind/sdk/sensors/SensorTypes.h>
#include <falconmind/sdk/core/Pad.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace falconmind::sdk::sensors;

std::atomic<int> imuCount{0};

int main() {
    std::cout << "MAVLink IMU Test - Binding to port 14540" << std::endl;
    
    auto imuSource = std::make_shared<ImuSourceNode>();
    std::unordered_map<std::string, std::string> params;
    params["device"] = "127.0.0.1:14540";
    imuSource->configure(params);
    
    auto imuOutPad = imuSource->getPad("imu_out");
    if (imuOutPad) {
        std::cout << "[Test] Got imu_out pad, setting callback..." << std::endl;
        imuOutPad->setDataCallback([](const void* data, size_t size) {
            std::cout << "[Test] Callback called with size=" << size 
                      << " expected=" << sizeof(ImuSample) << std::endl;
            if (size == sizeof(ImuSample)) {
                const ImuSample* s = static_cast<const ImuSample*>(data);
                imuCount++;
                std::cout << "[IMU " << imuCount << "] "
                          << "gyro: [" << s->gx << ", " << s->gy << ", " << s->gz << "] "
                          << "accel: [" << s->ax << ", " << s->ay << ", " << s->az << "]"
                          << std::endl;
            }
        });
        std::cout << "[Test] Callback set successfully" << std::endl;
    } else {
        std::cout << "[Test] ERROR: imu_out pad not found!" << std::endl;
    }
    
    if (!imuSource->start()) {
        std::cerr << "Failed to start IMU source" << std::endl;
        return 1;
    }
    
    std::cout << "Started. Waiting for data..." << std::endl;
    
    for (int i = 0; i < 100; ++i) {
        imuSource->process();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Total samples: " << imuCount << std::endl;
    imuSource->stop();
    
    return 0;
}
