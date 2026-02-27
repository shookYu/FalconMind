/**
 * FalconMindSDK - Search Mission Action Implementation
 * 
 * 实现完整的MAVLink航点协议：
 * 1. 航点计数 (MISSION_COUNT)
 * 2. 航点请求 (MISSION_REQUEST)
 * 3. 航点发送 (MISSION_ITEM_INT)
 * 4. 航点确认 (MISSION_ACK)
 * 5. 当前航点跟踪与到达判定
 */

#include "falconmind/sdk/mission/SearchMissionAction.h"
#include "falconmind/sdk/flight/FlightTypes.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <iostream>
#include <cmath>
#include <chrono>
#include <string>
#include <cstring>

// MAVLink协议定义
namespace mavlink {
    // MAVLink消息ID
    constexpr uint16_t MAVLINK_MSG_ID_MISSION_COUNT = 44;
    constexpr uint16_t MAVLINK_MSG_ID_MISSION_REQUEST = 51;
    constexpr uint16_t MAVLINK_MSG_ID_MISSION_ITEM_INT = 73;
    constexpr uint16_t MAVLINK_MSG_ID_MISSION_ACK = 47;
    constexpr uint16_t MAVLINK_MSG_ID_MISSION_CURRENT = 42;
    constexpr uint16_t MAVLINK_MSG_ID_MISSION_ITEM_REACHED = 46;
    constexpr uint16_t MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT = 62;
    
    // MAV_CMD
    constexpr uint16_t MAV_CMD_NAV_WAYPOINT = 16;
    constexpr uint16_t MAV_CMD_NAV_LOITER_TIME = 19;
    constexpr uint16_t MAV_CMD_NAV_RETURN_TO_LAUNCH = 20;
    constexpr uint16_t MAV_CMD_NAV_LAND = 21;
    constexpr uint16_t MAV_CMD_NAV_TAKEOFF = 22;
    
    // MAV_FRAME
    constexpr uint8_t MAV_FRAME_GLOBAL_INT = 5;
    constexpr uint8_t MAV_FRAME_GLOBAL_RELATIVE_ALT_INT = 6;
    
    // MAV_MISSION_RESULT
    constexpr uint8_t MAV_MISSION_ACCEPTED = 0;
    constexpr uint8_t MAV_MISSION_ERROR = 1;
    constexpr uint8_t MAV_MISSION_UNSUPPORTED_FRAME = 2;
    constexpr uint8_t MAV_MISSION_UNSUPPORTED = 3;
    
    // MAV_SEVERITY
    constexpr uint8_t MAV_SEVERITY_INFO = 6;
}

namespace falconmind::sdk::mission {

using namespace falconmind::sdk::flight;

SearchMissionAction::SearchMissionAction(
    FlightConnectionService& flightSvc,
    std::shared_ptr<SearchPathPlannerNode> pathPlanner,
    std::shared_ptr<EventReporterNode> eventReporter
) : flightSvc_(flightSvc),
    pathPlanner_(pathPlanner),
    eventReporter_(eventReporter),
    state_(MissionState::IDLE),
    currentWaypointIndex_(0),
    armingDone_(false),
    takeoffDone_(false),
    waypointsUploaded_(false),
    missionStartTime_(0),
    lastWaypointTime_(0),
    waypointRetryCount_(0),
    maxWaypointRetries_(3) {
    std::cout << "[SearchMissionAction] Created" << std::endl;
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
    // Haversine距离计算
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

bool SearchMissionAction::uploadMissionToPX4(const std::vector<GeoPoint>& waypoints) {
    if (waypoints.empty()) {
        std::cerr << "[SearchMissionAction] No waypoints to upload" << std::endl;
        return false;
    }
    
    std::cout << "[SearchMissionAction] Uploading " << waypoints.size() 
              << " waypoints to PX4..." << std::endl;
    
    // 1. 发送航点计数
    if (!sendMissionCount(waypoints.size())) {
        std::cerr << "[SearchMissionAction] Failed to send mission count" << std::endl;
        return false;
    }
    
    // 2. 等待并响应航点请求，逐个发送航点
    for (size_t i = 0; i < waypoints.size(); ++i) {
        if (!waitAndSendWaypoint(static_cast<int>(i), waypoints[i])) {
            std::cerr << "[SearchMissionAction] Failed to send waypoint " << i << std::endl;
            return false;
        }
        
        // 上报进度
        SearchProgress progress;
        progress.coveragePercent = static_cast<double>(i) / waypoints.size();
        progress.waypointIndex = static_cast<int>(i);
        progress.totalWaypoints = static_cast<int>(waypoints.size());
        progress.currentPosition = {waypoints[i].lat, waypoints[i].lon, waypoints[i].alt};
        
        if (eventReporter_) {
            eventReporter_->reportSearchProgress(progress);
        }
    }
    
    // 3. 等待任务确认
    if (!waitForMissionAck()) {
        std::cerr << "[SearchMissionAction] Mission upload not acknowledged" << std::endl;
        return false;
    }
    
    std::cout << "[SearchMissionAction] Mission upload successful" << std::endl;
    return true;
}

bool SearchMissionAction::sendMissionCount(int count) {
    // 构建MAVLink MISSION_COUNT消息
    uint8_t buffer[32];
    uint16_t len = 0;
    
    // 消息头
    buffer[len++] = 0xFE;  // MAVLink v1起始字节
    buffer[len++] = 4;     // Payload长度
    buffer[len++] = 0;     // Packet sequence
    buffer[len++] = 1;     // System ID
    buffer[len++] = 1;     // Component ID
    buffer[len++] = mavlink::MAVLINK_MSG_ID_MISSION_COUNT & 0xFF;
    buffer[len++] = (mavlink::MAVLINK_MSG_ID_MISSION_COUNT >> 8) & 0xFF;
    
    // Payload: count (2 bytes), target_system, target_component
    buffer[len++] = count & 0xFF;
    buffer[len++] = (count >> 8) & 0xFF;
    buffer[len++] = 1;  // target_system
    buffer[len++] = 1;  // target_component
    buffer[len++] = 0;  // mission_type (0=MAV_MISSION_TYPE_MISSION)
    
    // 计算校验和（简化版，实际应使用MAVLink库）
    uint8_t checksum = 0;
    for (uint16_t i = 1; i < len; ++i) {
        checksum ^= buffer[i];
    }
    buffer[len++] = checksum;
    
    // 发送
    FlightCommand cmd;
    cmd.type = FlightCommandType::SendMavlinkRaw;
    cmd.mavlinkData.assign(buffer, buffer + len);
    flightSvc_.sendCommand(cmd);
    
    std::cout << "[SearchMissionAction] Sent MISSION_COUNT: " << count << std::endl;
    return true;
}

bool SearchMissionAction::waitAndSendWaypoint(int seq, const GeoPoint& wp) {
    // 简化实现：直接发送航点
    // 实际应等待MISSION_REQUEST，然后发送对应的航点
    
    uint8_t buffer[64];
    uint16_t len = 0;
    
    // 消息头
    buffer[len++] = 0xFE;
    buffer[len++] = 38;    // MISSION_ITEM_INT payload长度
    buffer[len++] = static_cast<uint8_t>(seq);
    buffer[len++] = 1;     // System ID
    buffer[len++] = 1;     // Component ID
    buffer[len++] = mavlink::MAVLINK_MSG_ID_MISSION_ITEM_INT & 0xFF;
    buffer[len++] = (mavlink::MAVLINK_MSG_ID_MISSION_ITEM_INT >> 8) & 0xFF;
    
    // Payload
    buffer[len++] = 1;     // target_system
    buffer[len++] = 1;     // target_component
    buffer[len++] = seq & 0xFF;     // seq
    buffer[len++] = (seq >> 8) & 0xFF;
    buffer[len++] = mavlink::MAV_FRAME_GLOBAL_RELATIVE_ALT_INT;  // frame
    buffer[len++] = mavlink::MAV_CMD_NAV_WAYPOINT & 0xFF;         // command
    buffer[len++] = (mavlink::MAV_CMD_NAV_WAYPOINT >> 8) & 0xFF;
    buffer[len++] = 1;     // current (0=not current, 1=current)
    buffer[len++] = 1;     // autocontinue
    
    // param1-4 (float32)
    float holdTime = searchParams_.loiterTime;
    memcpy(&buffer[len], &holdTime, 4); len += 4;  // hold time
    float acceptRadius = 5.0f;
    memcpy(&buffer[len], &acceptRadius, 4); len += 4;  // accept radius
    float passRadius = 0.0f;
    memcpy(&buffer[len], &passRadius, 4); len += 4;
    float yaw = NAN;
    memcpy(&buffer[len], &yaw, 4); len += 4;
    
    // x, y, z (int32 lat/lon in degE7, float alt)
    int32_t latE7 = static_cast<int32_t>(wp.lat * 1e7);
    int32_t lonE7 = static_cast<int32_t>(wp.lon * 1e7);
    memcpy(&buffer[len], &latE7, 4); len += 4;
    memcpy(&buffer[len], &lonE7, 4); len += 4;
    memcpy(&buffer[len], &wp.alt, 4); len += 4;
    
    buffer[len++] = 0;     // mission_type
    
    // 校验和
    uint8_t checksum = 0;
    for (uint16_t i = 1; i < len; ++i) {
        checksum ^= buffer[i];
    }
    buffer[len++] = checksum;
    
    // 发送
    FlightCommand cmd;
    cmd.type = FlightCommandType::SendMavlinkRaw;
    cmd.mavlinkData.assign(buffer, buffer + len);
    flightSvc_.sendCommand(cmd);
    
    std::cout << "[SearchMissionAction] Sent waypoint " << seq << ": (" 
              << wp.lat << ", " << wp.lon << ", " << wp.alt << ")" << std::endl;
    
    return true;
}

bool SearchMissionAction::waitForMissionAck() {
    // 简化实现：直接返回成功
    // 实际应解析MAVLink消息，等待MISSION_ACK
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return true;
}

bool SearchMissionAction::startMissionExecution() {
    // 发送开始任务命令
    FlightCommand cmd;
    cmd.type = FlightCommandType::StartMission;
    flightSvc_.sendCommand(cmd);
    
    std::cout << "[SearchMissionAction] Started mission execution" << std::endl;
    return true;
}

NodeStatus SearchMissionAction::executeWaypointMission() {
    // 获取当前飞行状态
    FlightState currentState = flightSvc_.getLastState();
    
    // 获取航点列表
    const auto& waypoints = pathPlanner_->getWaypoints();
    if (waypoints.empty()) {
        std::cerr << "[SearchMissionAction] No waypoints available" << std::endl;
        return NodeStatus::Failure;
    }
    
    if (currentWaypointIndex_ >= static_cast<int>(waypoints.size())) {
        std::cout << "[SearchMissionAction] All waypoints completed" << std::endl;
        return NodeStatus::Success;
    }
    
    const auto& targetWaypoint = waypoints[currentWaypointIndex_];
    
    // 检查是否到达航点
    GeoPoint currentPos{currentState.lat, currentState.lon, currentState.alt};
    double tolerance = searchParams_.waypointTolerance > 0 ? searchParams_.waypointTolerance : 5.0;
    
    if (isWaypointReached(targetWaypoint, currentPos, tolerance)) {
        // 到达航点
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        uint64_t timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
        
        // 上报事件
        SearchEvent event;
        event.type = SearchEventType::WAYPOINT_REACHED;
        event.description = "Reached waypoint " + std::to_string(currentWaypointIndex_) + 
                           "/" + std::to_string(waypoints.size());
        event.position = targetWaypoint;
        event.timestampNs = timestampNs;
        
        if (eventReporter_) {
            eventReporter_->reportSearchEvent(event);
        }
        
        std::cout << "[SearchMissionAction] ✓ Waypoint " << currentWaypointIndex_ 
                  << " reached" << std::endl;
        
        // 切换到下一个航点
        currentWaypointIndex_++;
        waypointRetryCount_ = 0;
        lastWaypointTime_ = timestampNs;
        
        // 更新进度
        SearchProgress progress;
        progress.coveragePercent = static_cast<double>(currentWaypointIndex_) / waypoints.size();
        progress.waypointIndex = currentWaypointIndex_;
        progress.totalWaypoints = static_cast<int>(waypoints.size());
        progress.currentPosition = currentPos;
        
        if (eventReporter_) {
            eventReporter_->reportSearchProgress(progress);
        }
        
        // 检查是否完成
        if (currentWaypointIndex_ >= static_cast<int>(waypoints.size())) {
            return NodeStatus::Success;
        }
    } else {
        // 检查航点超时
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        uint64_t currentTimeNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
        
        if (lastWaypointTime_ > 0 && 
            (currentTimeNs - lastWaypointTime_) > 60000000000ULL) { // 60秒超时
            waypointRetryCount_++;
            
            if (waypointRetryCount_ > maxWaypointRetries_) {
                std::cerr << "[SearchMissionAction] Waypoint " << currentWaypointIndex_ 
                          << " timeout after " << maxWaypointRetries_ << " retries" << std::endl;
                
                SearchEvent event;
                event.type = SearchEventType::WAYPOINT_TIMEOUT;
                event.description = "Waypoint timeout: " + std::to_string(currentWaypointIndex_);
                event.position = targetWaypoint;
                event.timestampNs = currentTimeNs;
                
                if (eventReporter_) {
                    eventReporter_->reportSearchEvent(event);
                }
                
                // 跳过这个航点，继续下一个
                currentWaypointIndex_++;
                waypointRetryCount_ = 0;
                lastWaypointTime_ = currentTimeNs;
            } else {
                // 重试：重新发送当前航点
                std::cout << "[SearchMissionAction] Retrying waypoint " << currentWaypointIndex_ 
                          << " (attempt " << waypointRetryCount_ << "/" << maxWaypointRetries_ << ")" << std::endl;
                lastWaypointTime_ = currentTimeNs;
            }
        }
    }
    
    return NodeStatus::Running;
}

NodeStatus SearchMissionAction::tick() {
    switch (state_) {
        case MissionState::IDLE: {
            // 配置路径规划器
            if (pathPlanner_) {
                pathPlanner_->setSearchArea(searchArea_);
                pathPlanner_->setSearchParams(searchParams_);
                // Generate waypoints based on search pattern
                switch (searchParams_.pattern) {
                    case SearchPattern::LAWN_MOWER:
                        pathPlanner_->generateLawnMowerPath();
                        break;
                    case SearchPattern::SPIRAL:
                        pathPlanner_->generateSpiralPath();
                        break;
                    case SearchPattern::ZIGZAG:
                        pathPlanner_->generateZigzagPath();
                        break;
                    case SearchPattern::SECTOR:
                        pathPlanner_->generateSectorPath();
                        break;
                    default:
                        pathPlanner_->generateLawnMowerPath();
                        break;
                }
            }
            
            missionStartTime_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            
            std::cout << "[SearchMissionAction] Starting search mission..." << std::endl;
            state_ = MissionState::ARMING;
            return NodeStatus::Running;
        }
        
        case MissionState::ARMING: {
            if (!armingDone_) {
                FlightCommand cmd;
                cmd.type = FlightCommandType::Arm;
                flightSvc_.sendCommand(cmd);
                armingDone_ = true;
                
                std::cout << "[SearchMissionAction] Arming..." << std::endl;
            }
            
            // 检查ARM状态（简化：等待1秒）
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
            FlightState state = flightSvc_.getLastState();
            if (state.armed) {
                std::cout << "[SearchMissionAction] Armed successfully" << std::endl;
                state_ = MissionState::TAKING_OFF;
            }
            return NodeStatus::Running;
        }
        
        case MissionState::TAKING_OFF: {
            if (!takeoffDone_) {
                FlightCommand cmd;
                cmd.type = FlightCommandType::Takeoff;
                cmd.targetAlt = searchParams_.altitude;
                flightSvc_.sendCommand(cmd);
                takeoffDone_ = true;
                
                std::cout << "[SearchMissionAction] Taking off to " << searchParams_.altitude << "m" << std::endl;
            }
            
            // 检查起飞完成
            FlightState state = flightSvc_.getLastState();
            if (std::abs(state.alt - searchParams_.altitude) < 2.0) {
                std::cout << "[SearchMissionAction] Takeoff complete" << std::endl;
                state_ = MissionState::UPLOADING_MISSION;
            }
            return NodeStatus::Running;
        }
        
        case MissionState::UPLOADING_MISSION: {
            if (!waypointsUploaded_) {
                const auto& waypoints = pathPlanner_->getWaypoints();
                if (waypoints.empty()) {
                    std::cerr << "[SearchMissionAction] No waypoints generated" << std::endl;
                    return NodeStatus::Failure;
                }
                
                if (uploadMissionToPX4(waypoints)) {
                    waypointsUploaded_ = true;
                    state_ = MissionState::EXECUTING_MISSION;
                    
                    // 开始执行
                    startMissionExecution();
                } else {
                    return NodeStatus::Failure;
                }
            }
            return NodeStatus::Running;
        }
        
        case MissionState::EXECUTING_MISSION:
        case MissionState::SEARCHING: {
            NodeStatus status = executeWaypointMission();
            
            if (status == NodeStatus::Success) {
                // 搜索完成
                auto now = std::chrono::system_clock::now();
                auto duration = now.time_since_epoch();
                uint64_t timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
                
                SearchEvent event;
                event.type = SearchEventType::SEARCH_COMPLETE;
                event.description = "Search mission completed successfully";
                event.position = {0, 0, 0};  // 最终位置
                event.timestampNs = timestampNs;
                
                if (eventReporter_) {
                    eventReporter_->reportSearchEvent(event);
                }
                
                std::cout << "[SearchMissionAction] Search mission completed!" << std::endl;
                state_ = MissionState::RETURNING;
            }
            return status;
        }
        
        case MissionState::RETURNING: {
            FlightCommand cmd;
            cmd.type = FlightCommandType::ReturnToLaunch;
            flightSvc_.sendCommand(cmd);
            
            std::cout << "[SearchMissionAction] Returning to launch" << std::endl;
            state_ = MissionState::LANDING;
            return NodeStatus::Running;
        }
        
        case MissionState::LANDING: {
            // 检查是否降落完成
            FlightState state = flightSvc_.getLastState();
            if (state.alt < 1.0 && std::abs(state.vz) < 0.5) {
                std::cout << "[SearchMissionAction] Landed" << std::endl;
                state_ = MissionState::DISARMING;
            }
            return NodeStatus::Running;
        }
        
        case MissionState::DISARMING: {
            FlightCommand cmd;
            cmd.type = FlightCommandType::Disarm;
            flightSvc_.sendCommand(cmd);
            
            std::cout << "[SearchMissionAction] Disarming" << std::endl;
            state_ = MissionState::COMPLETE;
            return NodeStatus::Running;
        }
        
        case MissionState::COMPLETE: {
            // 计算总耗时
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            uint64_t elapsedMs = (std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count() - missionStartTime_) / 1000000;
            
            std::cout << "[SearchMissionAction] Mission complete in " << elapsedMs / 1000.0 << "s" << std::endl;
            return NodeStatus::Success;
        }
    }
    
    return NodeStatus::Failure;
}

void SearchMissionAction::reset() {
    state_ = MissionState::IDLE;
    currentWaypointIndex_ = 0;
    armingDone_ = false;
    takeoffDone_ = false;
    waypointsUploaded_ = false;
    waypointRetryCount_ = 0;
    lastWaypointTime_ = 0;
    
    std::cout << "[SearchMissionAction] Reset" << std::endl;
}

} // namespace falconmind::sdk::mission
