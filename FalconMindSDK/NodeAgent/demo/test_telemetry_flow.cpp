// 测试 Telemetry 数据流：手动发布 Telemetry 并验证 NodeAgent → Cluster Center

#include "nodeagent/NodeAgent.h"
#include "falconmind/sdk/telemetry/TelemetryPublisher.h"
#include "falconmind/sdk/telemetry/TelemetryTypes.h"

#include <chrono>
#include <thread>
#include <iostream>

using namespace falconmind::sdk::telemetry;
using namespace nodeagent;

int main(int argc, char* argv[]) {
    std::cout << "[test_telemetry_flow] Testing Telemetry flow: SDK → NodeAgent → Cluster Center" << std::endl;

    std::string centerAddress = "127.0.0.1";
    int centerPort = 8888;
    if (argc >= 3) {
        centerAddress = argv[1];
        centerPort = std::stoi(argv[2]);
    }

    // 1. 启动 NodeAgent
    NodeAgent::Config agentCfg;
    agentCfg.uavId = "uav0";
    agentCfg.centerAddress = centerAddress;
    agentCfg.centerPort = centerPort;

    NodeAgent agent(agentCfg);
    if (!agent.start()) {
        std::cerr << "[test_telemetry_flow] Failed to start NodeAgent" << std::endl;
        return 1;
    }

    std::cout << "[test_telemetry_flow] NodeAgent started. Waiting 1 second..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 2. 手动发布几条 Telemetry 消息（模拟 FlightStateSourceNode 的行为）
    std::cout << "[test_telemetry_flow] Publishing test Telemetry messages..." << std::endl;

    for (int i = 0; i < 3; ++i) {
        TelemetryMessage msg;
        msg.uavId = "uav0";
        auto now = std::chrono::system_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();
        msg.timestampNs = ns;

        // 模拟一些测试数据
        msg.lat = 40.2265 + i * 0.0001;  // 昌平区附近
        msg.lon = 116.2317 + i * 0.0001;
        msg.alt = 100.0 + i * 10.0;
        msg.roll = 0.1;
        msg.pitch = 0.2;
        msg.yaw = 1.57;
        msg.vx = 5.0;
        msg.vy = 0.0;
        msg.vz = 0.0;
        msg.batteryPercent = 85.0 - i;
        msg.batteryVoltageMv = 12600;
        msg.gpsFixType = 3;  // RTK Float
        msg.numSat = 12;
        msg.linkQuality = 95.0;
        msg.flightMode = "OFFBOARD";

        std::cout << "[test_telemetry_flow] Publishing Telemetry #" << (i + 1)
                  << " (lat=" << msg.lat << ", lon=" << msg.lon << ", alt=" << msg.alt << ")" << std::endl;

        TelemetryPublisher::instance().publish(msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[test_telemetry_flow] Waiting 1 second for messages to be sent..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // 3. 停止 NodeAgent
    agent.stop();

    std::cout << "[test_telemetry_flow] Done. Check Cluster Center mock output for received messages." << std::endl;
    return 0;
}
