// NodeAgent - Flow handler for zero-code dynamic execution
#pragma once

#include "nodeagent/DownlinkClient.h"
#include "falconmind/sdk/core/FlowExecutor.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <string>
#include <memory>
#include <functional>
#include <atomic>

namespace nodeagent {

// Flow状态上报回调类型
using FlowStatusCallback = std::function<void(const std::string& flow_id, 
                                               const std::string& status, 
                                               const std::string& error_msg)>;

// Flow处理器：接收Flow定义，使用FlowExecutor执行，上报状态
class FlowHandler {
public:
    FlowHandler();
    ~FlowHandler();

    // 设置FlightConnectionService（用于Flow中的飞行节点）
    void setFlightConnectionService(std::shared_ptr<falconmind::sdk::flight::FlightConnectionService> service);

    // 设置状态上报回调（用于上报Flow执行状态到Cluster Center）
    void setStatusCallback(FlowStatusCallback callback);

    // 处理Flow定义消息（从Cluster Center接收）
    // 消息格式：{"type":"flow","flow_id":"flow_001","flow_definition":{...}}
    // 或：{"type":"flow","flow_id":"flow_001","builder_url":"http://...","project_id":"...","flow_id":"..."}
    bool handleFlow(const DownlinkMessage& msg);

    // 更新Flow执行（每帧调用）
    void update();

    // 停止当前Flow
    void stopCurrentFlow();

    // 获取当前Flow ID
    std::string getCurrentFlowId() const { return current_flow_id_; }

    // 检查是否有Flow正在运行
    bool isFlowRunning() const { return executor_ && executor_->isRunning(); }

private:
    // 从JSON消息解析Flow定义
    bool parseFlowMessage(const std::string& json_payload, std::string& flow_id, std::string& flow_json);

    // 上报Flow状态
    void reportStatus(const std::string& flow_id, const std::string& status, const std::string& error_msg = "");

    std::shared_ptr<falconmind::sdk::core::FlowExecutor> executor_;
    std::shared_ptr<falconmind::sdk::flight::FlightConnectionService> flight_service_;
    FlowStatusCallback status_callback_;
    std::string current_flow_id_;
    std::atomic<bool> flow_active_{false};
};

} // namespace nodeagent
