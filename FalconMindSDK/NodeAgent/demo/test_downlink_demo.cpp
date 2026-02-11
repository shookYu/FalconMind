// 测试下行消息接收：NodeAgent 接收 Cluster Center 下发的命令/任务

#include "nodeagent/NodeAgent.h"
#include "nodeagent/DownlinkClient.h"
#include "falconmind/sdk/telemetry/TelemetryPublisher.h"
#include "falconmind/sdk/telemetry/TelemetryTypes.h"

#include <chrono>
#include <thread>
#include <iostream>

using namespace falconmind::sdk::telemetry;
using namespace nodeagent;

int main(int argc, char* argv[]) {
    std::cout << "[test_downlink_demo] Testing downlink message reception" << std::endl;

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

    // 2. 设置下行消息处理器
    agent.setDownlinkMessageHandler([](const DownlinkMessage& msg) {
        std::cout << "\n[NodeAgent] Received Downlink Message:\n"
                  << "  Type: " << (msg.type == DownlinkMessageType::Command ? "Command" : "Mission") << "\n"
                  << "  UAV ID: " << msg.uavId << "\n"
                  << "  Request ID: " << msg.requestId << "\n"
                  << "  Payload: " << msg.payload << "\n" << std::endl;

        // 这里可以进一步处理命令/任务，例如：
        // - 如果是 Command，转换为 FlightCommand 并发送到 SDK
        // - 如果是 Mission，解析任务定义并启动行为树执行
    });

    if (!agent.start()) {
        std::cerr << "[test_downlink_demo] Failed to start NodeAgent" << std::endl;
        return 1;
    }

    std::cout << "[test_downlink_demo] NodeAgent started. Waiting for downlink messages..." << std::endl;
    std::cout << "[test_downlink_demo] (In Cluster Center mock, type: 'send CMD:{\"type\":\"ARM\"}' or 'send MISSION:{\"id\":\"mission1\"}')" << std::endl;

    // 3. 发布一条测试 Telemetry（触发连接建立）
    TelemetryMessage msg;
    msg.uavId = "uav0";
    auto now = std::chrono::system_clock::now();
    msg.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    TelemetryPublisher::instance().publish(msg);

    // 4. 保持运行，等待下行消息
    std::this_thread::sleep_for(std::chrono::seconds(30));

    agent.stop();
    std::cout << "[test_downlink_demo] Done." << std::endl;
    return 0;
}
