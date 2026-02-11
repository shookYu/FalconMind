// NodeAgent - Mission handler for converting downlink missions to Behavior Tree
#pragma once

#include "nodeagent/DownlinkClient.h"
#include "falconmind/sdk/mission/BehaviorTree.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <string>
#include <memory>

namespace nodeagent {

// 任务处理器：将下行 Mission 消息转换为行为树并执行
class MissionHandler {
public:
    MissionHandler();
    ~MissionHandler();

    // 设置 FlightConnectionService（用于执行飞行动作）
    void setFlightConnectionService(std::shared_ptr<falconmind::sdk::flight::FlightConnectionService> service);

    // 处理下行任务消息
    // 返回：是否成功处理
    bool handleMission(const DownlinkMessage& msg);

    // 更新任务执行（每帧调用）
    void update();

private:
    // 解析 JSON payload 并创建行为树
    // 支持格式：{"id":"mission1","task":"takeoff_and_hover","params":{...}}
    std::shared_ptr<falconmind::sdk::mission::BehaviorNode> parseMissionJson(const std::string& jsonPayload);

    std::shared_ptr<falconmind::sdk::flight::FlightConnectionService> flightService_;
    std::shared_ptr<falconmind::sdk::mission::BehaviorTreeExecutor> executor_;
    bool missionActive_{false};
};

} // namespace nodeagent
