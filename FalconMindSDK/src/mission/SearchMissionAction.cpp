// FalconMindSDK - Search Mission Action Implementation
#include "falconmind/sdk/mission/SearchMissionAction.h"
#include "falconmind/sdk/flight/FlightTypes.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <cmath>
#include <chrono>

namespace falconmind::sdk::mission {

SearchMissionAction::SearchMissionAction(
    flight::FlightConnectionService& flightSvc,
    std::shared_ptr<SearchPathPlannerNode> pathPlanner,
    std::shared_ptr<EventReporterNode> eventReporter
) : flightSvc_(flightSvc),
    pathPlanner_(pathPlanner),
    eventReporter_(eventReporter) {
}

void SearchMissionAction::setSearchArea(const SearchArea& area) {
    searchArea_ = area;
    if (pathPlanner_) {
        pathPlanner_->setSearchArea(area);
    }
}

void SearchMissionAction::setSearchParams(const SearchParams& params) {
    searchParams_ = params;
    if (pathPlanner_) {
        pathPlanner_->setSearchParams(params);
    }
}

bool SearchMissionAction::isWaypointReached(const GeoPoint& waypoint, 
                                            const GeoPoint& currentPos, 
                                            double tolerance) {
    // 计算两点之间的距离（使用 Haversine 公式的简化版本）
    const double lat1 = waypoint.lat * M_PI / 180.0;
    const double lat2 = currentPos.lat * M_PI / 180.0;
    const double dLat = (currentPos.lat - waypoint.lat) * M_PI / 180.0;
    const double dLon = (currentPos.lon - waypoint.lon) * M_PI / 180.0;
    
    const double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                     std::cos(lat1) * std::cos(lat2) *
                     std::sin(dLon / 2) * std::sin(dLon / 2);
    const double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    const double distance = 6371000.0 * c; // 地球半径（米）
    
    const double altDiff = std::abs(currentPos.alt - waypoint.alt);
    
    return distance < tolerance && altDiff < tolerance;
}

NodeStatus SearchMissionAction::executeWaypointMission() {
    // 获取当前飞行状态
    flight::FlightState currentState = flightSvc_.getLastState();
    
    // 获取航点列表
    const auto& waypoints = pathPlanner_->getWaypoints();
    if (waypoints.empty()) {
        return NodeStatus::Failure;
    }
    
    if (currentWaypointIndex_ >= static_cast<int>(waypoints.size())) {
        // 所有航点已完成
        return NodeStatus::Success;
    }
    
    const auto& targetWaypoint = waypoints[currentWaypointIndex_];
    
    // TODO: 发送航点命令到 PX4
    // 这里简化处理，实际应该使用 MAVLink MISSION_ITEM_INT
    
    // 检查是否到达航点
    GeoPoint currentPos{currentState.lat, currentState.lon, currentState.alt};
    if (isWaypointReached(targetWaypoint, currentPos)) {
        // 到达航点，上报事件
        SearchEvent event;
        event.type = SearchEventType::WAYPOINT_REACHED;
        event.description = "Reached waypoint " + std::to_string(currentWaypointIndex_);
        event.position = targetWaypoint;
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        event.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
        
        if (eventReporter_) {
            eventReporter_->reportSearchEvent(event);
        }
        
        currentWaypointIndex_++;
        
        // 更新搜索进度
        SearchProgress progress;
        progress.coveragePercent = static_cast<double>(currentWaypointIndex_) / waypoints.size();
        progress.waypointIndex = currentWaypointIndex_;
        progress.totalWaypoints = waypoints.size();
        progress.currentPosition = currentPos;
        
        if (eventReporter_) {
            eventReporter_->reportSearchProgress(progress);
        }
    }
    
    return NodeStatus::Running;
}

NodeStatus SearchMissionAction::tick() {
    switch (state_) {
        case MissionState::IDLE:
            // 配置路径规划器
            if (pathPlanner_) {
                pathPlanner_->setSearchArea(searchArea_);
                pathPlanner_->setSearchParams(searchParams_);
            }
            state_ = MissionState::ARMING;
            return NodeStatus::Running;
            
        case MissionState::ARMING:
            if (!armingDone_) {
                flight::FlightCommand cmd;
                cmd.type = flight::FlightCommandType::Arm;
                flightSvc_.sendCommand(cmd);
                armingDone_ = true;
            }
            // 简化的状态机：假设 ARM 立即成功
            state_ = MissionState::TAKING_OFF;
            return NodeStatus::Running;
            
        case MissionState::TAKING_OFF:
            if (!takeoffDone_) {
                flight::FlightCommand cmd;
                cmd.type = flight::FlightCommandType::Takeoff;
                cmd.targetAlt = searchParams_.altitude;
                flightSvc_.sendCommand(cmd);
                takeoffDone_ = true;
            }
            // 简化的状态机：假设起飞立即完成
            state_ = MissionState::FLYING_TO_AREA;
            return NodeStatus::Running;
            
        case MissionState::FLYING_TO_AREA:
            // 飞到搜索区域（第一个航点）
            state_ = MissionState::SEARCHING;
            return NodeStatus::Running;
            
        case MissionState::SEARCHING: {
            NodeStatus status = executeWaypointMission();
            if (status == NodeStatus::Success) {
                // 搜索完成
                SearchEvent event;
                event.type = SearchEventType::SEARCH_COMPLETE;
                event.description = "Search mission completed";
                auto now = std::chrono::system_clock::now();
                auto duration = now.time_since_epoch();
                event.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
                
                if (eventReporter_) {
                    eventReporter_->reportSearchEvent(event);
                }
                
                state_ = MissionState::RETURNING;
            }
            return status;
        }
            
        case MissionState::RETURNING: {
            flight::FlightCommand cmd;
            cmd.type = flight::FlightCommandType::ReturnToLaunch;
            flightSvc_.sendCommand(cmd);
            state_ = MissionState::COMPLETE;
            return NodeStatus::Success;
        }
            
        case MissionState::COMPLETE:
            return NodeStatus::Success;
    }
    
    return NodeStatus::Failure;
}

} // namespace falconmind::sdk::mission
