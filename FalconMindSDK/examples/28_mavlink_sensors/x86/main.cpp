/**
 * Example 28: MAVLink Sensors - IMU & GNSS from PX4
 * 
 * This example demonstrates:
 * - Connecting to PX4 via MAVLink (UDP)
 * - Receiving real IMU data (gyro + accelerometer)
 * - Receiving real GNSS data (position + satellites)
 * - Displaying sensor data in real-time
 * 
 * Usage:
 *   ./28_mavlink_sensors_x86 [ip:port]
 *   
 *   Default: 127.0.0.1:14540 (PX4 SITL)
 *   Example: ./28_mavlink_sensors_x86 192.168.1.10:14540
 * 
 * Prerequisites:
 *   - PX4 running with MAVLink enabled (SITL or real hardware)
 *   - SDK built with flight module enabled
 */

#include <falconmind/sdk/core/Pipeline.h>
#include <falconmind/sdk/core/Node.h>
#include <falconmind/sdk/core/Pad.h>
#include <falconmind/sdk/sensors/ImuSourceNode.h>
#include <falconmind/sdk/sensors/GnssSourceNode.h>
#include <falconmind/sdk/sensors/SensorTypes.h>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <atomic>

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::sensors;

// Global counters for received data
std::atomic<int> imuCount{0};
std::atomic<int> gnssCount{0};

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "FalconMindSDK - MAVLink Sensors Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Parse command line arguments
    std::string mavlinkUri = "127.0.0.1:14540";
    if (argc > 1) {
        mavlinkUri = argv[1];
    }
    
    std::cout << "Connecting to MAVLink at: " << mavlinkUri << std::endl;
    std::cout << "Make sure PX4 is running (e.g., 'make px4_sitl jmavsim')" << std::endl;
    std::cout << std::endl;
    
    // Create IMU source node with MAVLink
    auto imuSource = std::make_shared<ImuSourceNode>();
    std::unordered_map<std::string, std::string> imuParams;
    imuParams["device"] = mavlinkUri;
    imuSource->configure(imuParams);
    
    // Create GNSS source node with MAVLink
    auto gnssSource = std::make_shared<GnssSourceNode>();
    std::unordered_map<std::string, std::string> gnssParams;
    gnssParams["device"] = mavlinkUri;
    gnssSource->configure(gnssParams);
    
    // Setup IMU data callback
    auto imuOutPad = imuSource->getPad("imu_out");
    if (imuOutPad) {
        imuOutPad->setDataCallback([](const void* data, size_t size) {
            if (size == sizeof(ImuSample)) {
                const ImuSample* s = static_cast<const ImuSample*>(data);
                imuCount++;
                
                // Only display every 10th sample to avoid spam
                if (imuCount % 10 == 0) {
                    std::cout << "\r[IMU] "
                              << "Gyro: [" << std::fixed << std::setprecision(3) 
                              << std::setw(7) << s->gx << ", "
                              << std::setw(7) << s->gy << ", "
                              << std::setw(7) << s->gz << "] rad/s  |  "
                              << "Accel: ["
                              << std::setw(7) << s->ax << ", "
                              << std::setw(7) << s->ay << ", "
                              << std::setw(7) << s->az << "] m/s²"
                              << "  (samples: " << imuCount << ")";
                    std::cout.flush();
                }
            }
        });
    }
    
    // Setup GNSS data callback
    auto gnssOutPad = gnssSource->getPad("gnss_out");
    if (gnssOutPad) {
        gnssOutPad->setDataCallback([](const void* data, size_t size) {
            if (size == sizeof(GnssSample)) {
                const GnssSample* s = static_cast<const GnssSample*>(data);
                gnssCount++;
                
                std::cout << "\n[GNSS] "
                          << std::fixed << std::setprecision(6)
                          << "Lat: " << std::setw(10) << s->latitude << "°  "
                          << "Lon: " << std::setw(10) << s->longitude << "°  "
                          << "Alt: " << std::setw(7) << std::setprecision(2) << s->altitude << "m  "
                          << "Sats: " << s->numSatellites << "  "
                          << "HDOP: " << s->hdop
                          << "  (samples: " << gnssCount << ")"
                          << std::endl;
            }
        });
    }
    
    // Start nodes
    if (!imuSource->start()) {
        std::cerr << "Failed to start IMU source node" << std::endl;
        return 1;
    }
    if (!gnssSource->start()) {
        std::cerr << "Failed to start GNSS source node" << std::endl;
        imuSource->stop();
        return 1;
    }
    
    std::cout << "Nodes started. Receiving MAVLink data..." << std::endl;
    std::cout << "Press Ctrl+C to stop." << std::endl;
    std::cout << std::endl;
    
    // Run for 30 seconds
    for (int i = 0; i < 300; ++i) {
        imuSource->process();
        gnssSource->process();
        std::this_thread::sleep_for(std::chrono::milliseconds(100LL));
    }
    
    std::cout << "\n\nStopping pipeline..." << std::endl;
    std::cout << "Total IMU samples: " << imuCount << std::endl;
    std::cout << "Total GNSS samples: " << gnssCount << std::endl;
    
    // Stop nodes
    imuSource->stop();
    gnssSource->stop();
    
    std::cout << "Done!" << std::endl;
    
    return 0;
}
