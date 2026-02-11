// FalconMindSDK - Telemetry Publisher/Subscriber Demo
// 演示 SDK 内部 Telemetry 发布与订阅机制，为后续 NodeAgent 集成做准备

#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/flight/FlightNodes.h"
#include "falconmind/sdk/telemetry/TelemetryPublisher.h"

#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>

using namespace falconmind::sdk::flight;
using namespace falconmind::sdk::telemetry;

int main() {
    std::cout << "[telemetry_demo] Starting Telemetry Publisher/Subscriber demo..." << std::endl;

    // 1. 连接 FlightConnectionService（模拟 PX4-SITL）
    FlightConnectionService svc;
    FlightConnectionConfig cfg;
    cfg.remoteAddress = "127.0.0.1";
    cfg.remotePort = 14540;

    if (!svc.connect(cfg)) {
        std::cerr << "[telemetry_demo] Failed to connect FlightConnectionService" << std::endl;
        return 1;
    }

    // 2. 创建 FlightStateSourceNode（会自动发布 Telemetry）
    FlightStateSourceNode stateNode(svc);
    stateNode.start();

    // 3. 订阅 Telemetry 消息（模拟 NodeAgent 的订阅行为）
    int subId = TelemetryPublisher::instance().subscribe(
        [](const TelemetryMessage& msg) {
            // 打印格式化的 Telemetry 信息（后续 NodeAgent 会序列化为 Proto 并上报）
            auto seconds = msg.timestampNs / 1000000000LL;
            auto nanos = msg.timestampNs % 1000000000LL;
            std::time_t timeT = seconds;
            std::tm* tm = std::gmtime(&timeT);

            std::cout << "\n[Telemetry] UAV=" << msg.uavId
                      << " time=" << std::put_time(tm, "%Y-%m-%d %H:%M:%S")
                      << "." << std::setfill('0') << std::setw(9) << nanos
                      << "\n  Position: lat=" << std::fixed << std::setprecision(7) << msg.lat
                      << " lon=" << msg.lon << " alt=" << std::setprecision(2) << msg.alt << "m"
                      << "\n  Attitude: roll=" << std::setprecision(3) << msg.roll
                      << " pitch=" << msg.pitch << " yaw=" << msg.yaw
                      << "\n  Velocity: vx=" << msg.vx << " vy=" << msg.vy << " vz=" << msg.vz << " m/s"
                      << "\n  Battery: " << std::setprecision(1) << msg.batteryPercent << "%"
                      << " (" << msg.batteryVoltageMv << "mV)"
                      << "\n  GPS: fix=" << msg.gpsFixType << " sats=" << msg.numSat
                      << " link=" << std::setprecision(1) << msg.linkQuality << "%"
                      << " mode=" << msg.flightMode
                      << std::endl;
        });

    std::cout << "[telemetry_demo] Subscribed to Telemetry (id=" << subId << ")" << std::endl;
    std::cout << "[telemetry_demo] Polling FlightState and publishing Telemetry..." << std::endl;
    std::cout << "[telemetry_demo] (Note: Without PX4-SITL, pollState() may return empty)" << std::endl;

    // 4. 循环调用 process()，触发 Telemetry 发布
    for (int i = 0; i < 5; ++i) {
        stateNode.process();  // 会触发 pollState() 并发布 Telemetry
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // 5. 取消订阅并断开连接
    TelemetryPublisher::instance().unsubscribe(subId);
    svc.disconnect();

    std::cout << "[telemetry_demo] Done." << std::endl;
    return 0;
}
