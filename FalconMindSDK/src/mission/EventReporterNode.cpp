// FalconMindSDK - Event Reporter Node Implementation
#include "falconmind/sdk/mission/EventReporterNode.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/core/Caps.h"
#include "falconmind/sdk/telemetry/TelemetryPublisher.h"

#include <chrono>
#include <sstream>
#include <unordered_map>

namespace falconmind::sdk::mission {

EventReporterNode::EventReporterNode() : core::Node("event_reporter") {
    // 添加输入端口：接收事件数据（使用 Sink 类型表示输入）
    addPad(std::make_shared<core::Pad>("events", core::PadType::Sink));
    
    // 默认 UAV ID 和 Mission ID
    uavId_ = "uav_001";
    missionId_ = "mission_unknown";
}

bool EventReporterNode::configure(const std::unordered_map<std::string, std::string>& params) {
    core::Node::configure(params);
    // 可以从 params 中读取 uavId 和 missionId
    if (params.find("uav_id") != params.end()) {
        uavId_ = params.at("uav_id");
    }
    if (params.find("mission_id") != params.end()) {
        missionId_ = params.at("mission_id");
    }
    return true;
}

bool EventReporterNode::start() {
    core::Node::start();
    return true;
}

void EventReporterNode::stop() {
    core::Node::stop();
}

void EventReporterNode::process() {
    // process 方法可以处理来自输入端口的事件数据
    // 当前实现主要通过直接调用 reportSearchEvent 等方法
}

void EventReporterNode::reportSearchEvent(const SearchEvent& event) {
    // 通过 TelemetryPublisher 上报事件
    // 注意：当前 TelemetryMessage 结构可能需要扩展以支持事件
    // 这里先输出日志，后续可以扩展 TelemetryMessage 或使用 Bus 发布事件
    
    std::stringstream ss;
    ss << "[EventReporter] Event: " << static_cast<int>(event.type)
       << " at (" << event.position.lat << ", " << event.position.lon << ")"
       << " - " << event.description;
    
    // TODO: 通过 TelemetryPublisher 或 Bus 发布事件
    // 当前先输出日志
    // TelemetryPublisher::instance().publish(...);
}

void EventReporterNode::reportSearchProgress(const SearchProgress& progress) {
    std::stringstream ss;
    ss << "[EventReporter] Progress: " << (progress.coveragePercent * 100.0) << "%"
       << " (" << progress.waypointIndex << "/" << progress.totalWaypoints << " waypoints)";
    
    // TODO: 通过 TelemetryPublisher 发布进度
}

void EventReporterNode::reportDetection(const std::string& targetClass, double confidence,
                                        double lat, double lon, double alt) {
    SearchEvent event;
    event.type = SearchEventType::TARGET_DETECTED;
    event.description = "Target detected: " + targetClass + " (confidence: " + 
                       std::to_string(confidence) + ")";
    event.position = {lat, lon, alt};
    
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    event.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
    
    std::stringstream metadata;
    metadata << "{\"class\":\"" << targetClass << "\",\"confidence\":" << confidence << "}";
    event.metadata = metadata.str();
    
    reportSearchEvent(event);
}

} // namespace falconmind::sdk::mission
