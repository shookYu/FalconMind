#include "nodeagent/FlowHandler.h"
#include <nlohmann/json.hpp>
#include <iostream>

namespace nodeagent {

using json = nlohmann::json;

FlowHandler::FlowHandler() {
    executor_ = std::make_shared<falconmind::sdk::core::FlowExecutor>();
}

FlowHandler::~FlowHandler() {
    stopCurrentFlow();
}

void FlowHandler::setFlightConnectionService(std::shared_ptr<falconmind::sdk::flight::FlightConnectionService> service) {
    flight_service_ = service;
    // 注意：FlowExecutor中的节点如果需要FlightConnectionService，应该通过NodeFactory配置
    // 这里保留接口以便将来扩展
}

void FlowHandler::setStatusCallback(FlowStatusCallback callback) {
    status_callback_ = callback;
}

bool FlowHandler::handleFlow(const DownlinkMessage& msg) {
    std::string flow_id;
    std::string flow_json;

    // 解析消息
    if (!parseFlowMessage(msg.payload, flow_id, flow_json)) {
        std::cerr << "[FlowHandler] Failed to parse flow message: " << msg.payload << std::endl;
        reportStatus(flow_id, "FAILED", "Failed to parse flow message");
        return false;
    }

    // 停止当前Flow（如果有）
    if (flow_active_ && executor_) {
        stopCurrentFlow();
    }

    // 加载Flow定义
    bool load_success = false;
    try {
        // 检查消息中是否包含flow_definition字段（直接包含Flow定义）
        json msg_json = json::parse(msg.payload);
        
        if (msg_json.contains("flow_definition") && msg_json["flow_definition"].is_object()) {
            // 直接使用flow_definition字段
            flow_json = msg_json["flow_definition"].dump();
            load_success = executor_->loadFlow(flow_json);
        } else if (msg_json.contains("builder_url") && msg_json.contains("project_id")) {
            // 从Builder API加载
            std::string builder_url = msg_json["builder_url"].get<std::string>();
            std::string project_id = msg_json["project_id"].get<std::string>();
            load_success = executor_->loadFlowFromBuilder(builder_url, project_id, flow_id);
        } else {
            // 尝试直接解析payload作为Flow定义
            load_success = executor_->loadFlow(msg.payload);
        }
    } catch (const std::exception& e) {
        std::cerr << "[FlowHandler] Error loading flow: " << e.what() << std::endl;
        reportStatus(flow_id, "FAILED", std::string("Error loading flow: ") + e.what());
        return false;
    }

    if (!load_success) {
        std::cerr << "[FlowHandler] Failed to load flow: " << flow_id << std::endl;
        reportStatus(flow_id, "FAILED", "Failed to load flow definition");
        return false;
    }

    // 启动Flow执行
    if (!executor_->start()) {
        std::cerr << "[FlowHandler] Failed to start flow: " << flow_id << std::endl;
        reportStatus(flow_id, "FAILED", "Failed to start flow execution");
        return false;
    }

    current_flow_id_ = flow_id;
    flow_active_ = true;
    
    std::cout << "[FlowHandler] Flow started: " << flow_id << std::endl;
    reportStatus(flow_id, "RUNNING");

    return true;
}

void FlowHandler::update() {
    if (!flow_active_ || !executor_) {
        return;
    }

    // 检查Flow是否还在运行
    if (!executor_->isRunning()) {
        // Flow已停止
        std::string flow_id = current_flow_id_;
        std::cout << "[FlowHandler] Flow stopped: " << flow_id << std::endl;
        reportStatus(flow_id, "COMPLETED");
        flow_active_ = false;
        current_flow_id_.clear();
    }
}

void FlowHandler::stopCurrentFlow() {
    if (executor_ && executor_->isRunning()) {
        std::string flow_id = current_flow_id_;
        executor_->stop();
        flow_active_ = false;
        current_flow_id_.clear();
        reportStatus(flow_id, "STOPPED");
        std::cout << "[FlowHandler] Flow stopped: " << flow_id << std::endl;
    }
}

bool FlowHandler::parseFlowMessage(const std::string& json_payload, std::string& flow_id, std::string& flow_json) {
    try {
        json msg_json = json::parse(json_payload);

        // 提取flow_id
        if (msg_json.contains("flow_id") && msg_json["flow_id"].is_string()) {
            flow_id = msg_json["flow_id"].get<std::string>();
        } else {
            std::cerr << "[FlowHandler] Missing or invalid 'flow_id' field" << std::endl;
            return false;
        }

        // flow_json将在handleFlow中根据消息类型确定
        return true;
    } catch (const json::parse_error& e) {
        std::cerr << "[FlowHandler] JSON parse error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[FlowHandler] Error parsing flow message: " << e.what() << std::endl;
        return false;
    }
}

void FlowHandler::reportStatus(const std::string& flow_id, const std::string& status, const std::string& error_msg) {
    if (status_callback_) {
        status_callback_(flow_id, status, error_msg);
    } else {
        // 如果没有设置回调，只打印日志
        std::cout << "[FlowHandler] Flow status: " << flow_id << " -> " << status;
        if (!error_msg.empty()) {
            std::cout << " (" << error_msg << ")";
        }
        std::cout << std::endl;
    }
}

} // namespace nodeagent
