// NodeAgent Demo - 演示 NodeAgent 订阅 SDK Telemetry 并上报到 Cluster Center

#include "nodeagent/NodeAgent.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/flight/FlightNodes.h"

#include <chrono>
#include <thread>
#include <iostream>

using namespace falconmind::sdk::flight;
using namespace nodeagent;

int main(int argc, char* argv[]) {
    std::cout << "[nodeagent_demo] Starting NodeAgent demo..." << std::endl;

    // 解析命令行参数（可选）
    std::string centerAddress = "127.0.0.1";
    int centerPort = 8888;
    if (argc >= 3) {
        centerAddress = argv[1];
        centerPort = std::stoi(argv[2]);
    }

    // 1. 连接 SDK FlightConnectionService（模拟 PX4-SITL）
    FlightConnectionService svc;
    FlightConnectionConfig cfg;
    cfg.remoteAddress = "127.0.0.1";
    cfg.remotePort = 14540;

    if (!svc.connect(cfg)) {
        std::cerr << "[nodeagent_demo] Failed to connect FlightConnectionService" << std::endl;
        return 1;
    }

    // 2. 创建 FlightStateSourceNode（会自动发布 Telemetry）
    FlightStateSourceNode stateNode(svc);
    stateNode.start();

    // 3. 创建并启动 NodeAgent
    NodeAgent::Config agentCfg;
    agentCfg.uavId = "uav0";
    agentCfg.centerAddress = centerAddress;
    agentCfg.centerPort = centerPort;
    agentCfg.telemetryIntervalMs = 1000;

    NodeAgent agent(agentCfg);
    if (!agent.start()) {
        std::cerr << "[nodeagent_demo] Failed to start NodeAgent" << std::endl;
        svc.disconnect();
        return 1;
    }

    std::cout << "[nodeagent_demo] NodeAgent started. Polling FlightState..." << std::endl;
    std::cout << "[nodeagent_demo] (Make sure Cluster Center mock is running on "
              << centerAddress << ":" << centerPort << ")" << std::endl;

    // 4. 循环调用 process()，触发 Telemetry 发布和上报
    for (int i = 0; i < 10; ++i) {
        stateNode.process();  // 会触发 pollState() 并发布 Telemetry，NodeAgent 会自动上报
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // 5. 停止 NodeAgent 并断开连接
    agent.stop();
    svc.disconnect();

    std::cout << "[nodeagent_demo] Done." << std::endl;
    return 0;
}
