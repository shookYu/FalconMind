// FalconMindSDK - Search Mission Action Node for Behavior Tree
#pragma once

#include "falconmind/sdk/mission/BehaviorTree.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include "falconmind/sdk/mission/SearchPathPlannerNode.h"
#include "falconmind/sdk/mission/EventReporterNode.h"

#include <memory>
#include <vector>

namespace falconmind::sdk::flight {
    class FlightConnectionService;
}

namespace falconmind::sdk::mission {

/**
 * 搜索任务行为树节点
 * 整合起飞、搜索路径执行、检测、上报、返航的完整流程
 */
class SearchMissionAction : public BehaviorNode {
public:
    SearchMissionAction(
        flight::FlightConnectionService& flightSvc,
        std::shared_ptr<SearchPathPlannerNode> pathPlanner,
        std::shared_ptr<EventReporterNode> eventReporter
    );
    
    ~SearchMissionAction() override = default;

    // 配置搜索任务
    void setSearchArea(const SearchArea& area);
    void setSearchParams(const SearchParams& params);

    // BehaviorNode 接口
    NodeStatus tick() override;

private:
    // 任务状态
    enum class MissionState {
        IDLE,
        ARMING,
        TAKING_OFF,
        FLYING_TO_AREA,
        SEARCHING,
        RETURNING,
        COMPLETE
    };

    // 执行航点任务
    NodeStatus executeWaypointMission();
    
    // 检查是否到达航点
    bool isWaypointReached(const GeoPoint& waypoint, const GeoPoint& currentPos, double tolerance = 5.0);

    flight::FlightConnectionService& flightSvc_;
    std::shared_ptr<SearchPathPlannerNode> pathPlanner_;
    std::shared_ptr<EventReporterNode> eventReporter_;
    
    SearchArea searchArea_;
    SearchParams searchParams_;
    MissionState state_{MissionState::IDLE};
    int currentWaypointIndex_{0};
    bool armingDone_{false};
    bool takeoffDone_{false};
};

} // namespace falconmind::sdk::mission
