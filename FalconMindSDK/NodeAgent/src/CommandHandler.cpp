#include "nodeagent/CommandHandler.h"
#include <nlohmann/json.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>

namespace nodeagent {

using namespace falconmind::sdk::flight;

CommandHandler::CommandHandler() {
}

CommandHandler::~CommandHandler() {
}

void CommandHandler::setFlightConnectionService(std::shared_ptr<FlightConnectionService> service) {
    flightService_ = service;
}

bool CommandHandler::handleCommand(const DownlinkMessage& msg) {
    if (!flightService_) {
        std::cerr << "[CommandHandler] FlightConnectionService not set" << std::endl;
        return false;
    }

    if (!flightService_->isConnected()) {
        std::cerr << "[CommandHandler] FlightConnectionService not connected" << std::endl;
        return false;
    }

    FlightCommand cmd;
    if (!parseCommandJson(msg.payload, cmd)) {
        std::cerr << "[CommandHandler] Failed to parse command JSON: " << msg.payload << std::endl;
        return false;
    }

    std::cout << "[CommandHandler] Executing command: "
              << static_cast<int>(cmd.type) << " (targetAlt=" << cmd.targetAlt << ")" << std::endl;

    bool success = flightService_->sendCommand(cmd);
    if (success) {
        std::cout << "[CommandHandler] Command sent successfully" << std::endl;
    } else {
        std::cerr << "[CommandHandler] Failed to send command" << std::endl;
    }

    return success;
}

bool CommandHandler::parseCommandJson(const std::string& jsonPayload, FlightCommand& cmd) {
    try {
        // 使用 nlohmann/json 解析 JSON
        auto json = nlohmann::json::parse(jsonPayload);

        // 提取 type 字段
        std::string typeStr;
        if (json.contains("type") && json["type"].is_string()) {
            typeStr = json["type"].get<std::string>();
        } else {
            std::cerr << "[CommandHandler] Missing or invalid 'type' field" << std::endl;
            return false;
        }

        // 提取 targetAlt 字段（可选）
        double targetAlt = 0.0;
        if (json.contains("targetAlt") && json["targetAlt"].is_number()) {
            targetAlt = json["targetAlt"].get<double>();
        }

        // 转换为 FlightCommandType（支持大小写不敏感）
        std::string typeUpper = typeStr;
        std::transform(typeUpper.begin(), typeUpper.end(), typeUpper.begin(), ::toupper);

        if (typeUpper == "ARM") {
            cmd.type = FlightCommandType::Arm;
        } else if (typeUpper == "DISARM") {
            cmd.type = FlightCommandType::Disarm;
        } else if (typeUpper == "TAKEOFF") {
            cmd.type = FlightCommandType::Takeoff;
            cmd.targetAlt = targetAlt;
        } else if (typeUpper == "LAND") {
            cmd.type = FlightCommandType::Land;
        } else if (typeUpper == "RTL" || typeUpper == "RETURN_TO_LAUNCH") {
            cmd.type = FlightCommandType::ReturnToLaunch;
        } else {
            std::cerr << "[CommandHandler] Unknown command type: " << typeStr << std::endl;
            return false;
        }

        return true;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "[CommandHandler] JSON parse error: " << e.what() << std::endl;
        return false;
    } catch (const nlohmann::json::type_error& e) {
        std::cerr << "[CommandHandler] JSON type error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[CommandHandler] Error parsing command JSON: " << e.what() << std::endl;
        return false;
    }
}

} // namespace nodeagent
