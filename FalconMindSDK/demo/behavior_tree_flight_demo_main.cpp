#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/mission/BehaviorTree.h"
#include "falconmind/sdk/mission/FlightActions.h"

#include <chrono>
#include <thread>
#include <iostream>

using namespace falconmind::sdk;

int main() {
    using namespace flight;
    using namespace mission;

    FlightConnectionService svc;
    FlightConnectionConfig cfg;
    cfg.remoteAddress = "127.0.0.1";
    cfg.remotePort = 14540;

    if (!svc.connect(cfg)) {
        std::cerr << "[behavior_tree_flight_demo] Failed to connect FlightConnectionService"
                  << std::endl;
        return 1;
    }

    // 构建行为树：Arm → Takeoff(10m) → Hover(5s) → RTL
    auto root = std::make_shared<SequenceNode>();
    root->addChild(std::make_shared<ArmAction>(svc));
    root->addChild(std::make_shared<TakeoffAction>(svc, 10.0));
    root->addChild(std::make_shared<HoverAction>(std::chrono::seconds(5)));
    root->addChild(std::make_shared<RtlAction>(svc));

    BehaviorTreeExecutor executor(root);

    std::cout << "[behavior_tree_flight_demo] Starting behavior tree (Arm -> Takeoff -> Hover -> RTL)"
              << std::endl;

    while (true) {
        NodeStatus status = executor.tick();
        if (status == NodeStatus::Running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        if (status == NodeStatus::Success) {
            std::cout << "[behavior_tree_flight_demo] Behavior tree succeeded." << std::endl;
        } else {
            std::cout << "[behavior_tree_flight_demo] Behavior tree failed." << std::endl;
        }
        break;
    }

    svc.disconnect();
    std::cout << "[behavior_tree_flight_demo] Done." << std::endl;
    return 0;
}

