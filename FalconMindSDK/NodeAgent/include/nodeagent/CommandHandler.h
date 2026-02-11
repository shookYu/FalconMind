// NodeAgent - Command handler for converting downlink commands to SDK FlightCommand
#pragma once

#include "nodeagent/DownlinkClient.h"
#include "falconmind/sdk/flight/FlightTypes.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <string>
#include <memory>

namespace nodeagent {

// 命令处理器：将下行 Command 消息转换为 SDK FlightCommand 并执行
class CommandHandler {
public:
    CommandHandler();
    ~CommandHandler();

    // 设置 FlightConnectionService（用于发送命令）
    void setFlightConnectionService(std::shared_ptr<falconmind::sdk::flight::FlightConnectionService> service);

    // 处理下行命令消息
    // 返回：是否成功处理
    bool handleCommand(const DownlinkMessage& msg);

private:
    // 解析 JSON payload 并转换为 FlightCommand
    // 支持格式：{"type":"ARM","uavId":"uav0","targetAlt":10.0}
    bool parseCommandJson(const std::string& jsonPayload, falconmind::sdk::flight::FlightCommand& cmd);

    std::shared_ptr<falconmind::sdk::flight::FlightConnectionService> flightService_;
};

} // namespace nodeagent
