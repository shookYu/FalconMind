// FalconMindSDK - Event Reporter Node
#pragma once

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include "falconmind/sdk/telemetry/TelemetryPublisher.h"

#include <functional>
#include <string>

namespace falconmind::sdk::mission {

/**
 * 事件上报节点
 * 接收搜索事件和检测结果，通过 TelemetryPublisher 上报
 */
class EventReporterNode : public core::Node {
public:
    EventReporterNode();
    ~EventReporterNode() override = default;

    // 上报搜索事件
    void reportSearchEvent(const SearchEvent& event);
    
    // 上报搜索进度
    void reportSearchProgress(const SearchProgress& progress);
    
    // 上报检测结果（从 DetectionResult 转换）
    void reportDetection(const std::string& targetClass, double confidence,
                        double lat, double lon, double alt);

    // Node 接口实现
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;

private:
    std::string uavId_;
    std::string missionId_;
};

} // namespace falconmind::sdk::mission
