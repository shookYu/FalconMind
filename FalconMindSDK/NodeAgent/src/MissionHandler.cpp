#include "nodeagent/MissionHandler.h"
#include "falconmind/sdk/mission/FlightActions.h"
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <chrono>

namespace nodeagent {

using namespace falconmind::sdk::mission;
using namespace falconmind::sdk::flight;

MissionHandler::MissionHandler() {
}

MissionHandler::~MissionHandler() {
}

void MissionHandler::setFlightConnectionService(std::shared_ptr<FlightConnectionService> service) {
    flightService_ = service;
}

bool MissionHandler::handleMission(const DownlinkMessage& msg) {
    if (!flightService_) {
        std::cerr << "[MissionHandler] FlightConnectionService not set" << std::endl;
        return false;
    }

    if (!flightService_->isConnected()) {
        std::cerr << "[MissionHandler] FlightConnectionService not connected" << std::endl;
        return false;
    }

    auto root = parseMissionJson(msg.payload);
    if (!root) {
        std::cerr << "[MissionHandler] Failed to parse mission JSON: " << msg.payload << std::endl;
        return false;
    }

    executor_ = std::make_shared<BehaviorTreeExecutor>(root);
    missionActive_ = true;

    std::cout << "[MissionHandler] Mission started: " << msg.payload << std::endl;
    return true;
}

void MissionHandler::update() {
    if (!missionActive_ || !executor_) {
        return;
    }

    auto status = executor_->tick();
    if (status == NodeStatus::Success || status == NodeStatus::Failure) {
        std::cout << "[MissionHandler] Mission completed with status: "
                  << (status == NodeStatus::Success ? "Success" : "Failure") << std::endl;
        missionActive_ = false;
        executor_.reset();
    }
}

std::shared_ptr<BehaviorNode> MissionHandler::parseMissionJson(const std::string& jsonPayload) {
    try {
        // 使用 nlohmann/json 解析 JSON
        auto json = nlohmann::json::parse(jsonPayload);

        // 提取 task 字段
        std::string taskStr;
        if (json.contains("task") && json["task"].is_string()) {
            taskStr = json["task"].get<std::string>();
        } else {
            std::cerr << "[MissionHandler] Missing or invalid 'task' field" << std::endl;
            return nullptr;
        }

        // 提取可选参数
        double takeoffAlt = 10.0;  // 默认高度
        if (json.contains("params") && json["params"].is_object()) {
            auto params = json["params"];
            if (params.contains("takeoffAlt") && params["takeoffAlt"].is_number()) {
                takeoffAlt = params["takeoffAlt"].get<double>();
            }
        }

        int hoverDuration = 5;  // 默认悬停时间（秒）
        if (json.contains("params") && json["params"].is_object()) {
            auto params = json["params"];
            if (params.contains("hoverDuration") && params["hoverDuration"].is_number()) {
                hoverDuration = params["hoverDuration"].get<int>();
            }
        }

        // 根据 task 类型创建行为树（支持大小写不敏感）
        std::string taskUpper = taskStr;
        std::transform(taskUpper.begin(), taskUpper.end(), taskUpper.begin(), ::toupper);

        auto sequence = std::make_shared<SequenceNode>();

        if (taskUpper == "TAKEOFF_AND_HOVER") {
            // 创建：Arm -> Takeoff -> Hover -> RTL
            auto arm = std::make_shared<ArmAction>(*flightService_);
            auto takeoff = std::make_shared<TakeoffAction>(*flightService_, takeoffAlt);
            auto hover = std::make_shared<HoverAction>(std::chrono::seconds(hoverDuration));
            auto rtl = std::make_shared<RtlAction>(*flightService_);

            sequence->addChild(arm);
            sequence->addChild(takeoff);
            sequence->addChild(hover);
            sequence->addChild(rtl);
        } else if (taskUpper == "SIMPLE_TAKEOFF") {
            // 创建：Arm -> Takeoff
            auto arm = std::make_shared<ArmAction>(*flightService_);
            auto takeoff = std::make_shared<TakeoffAction>(*flightService_, takeoffAlt);

            sequence->addChild(arm);
            sequence->addChild(takeoff);
        } else {
            std::cerr << "[MissionHandler] Unknown task type: " << taskStr << std::endl;
            return nullptr;
        }

        return sequence;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[MissionHandler] JSON parse error: " << e.what() << std::endl;
        return nullptr;
    } catch (const nlohmann::json::type_error& e) {
        std::cerr << "[MissionHandler] JSON type error: " << e.what() << std::endl;
        return nullptr;
    } catch (const std::exception& e) {
        std::cerr << "[MissionHandler] Error parsing mission JSON: " << e.what() << std::endl;
        return nullptr;
    }
}

} // namespace nodeagent
