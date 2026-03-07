/**
 * @file flow_node.cpp
 * @brief Flow节点基类实现
 */

#include "falconmind/sdk/flow/flow_node.hpp"
#include <iostream>

namespace falconmind {
namespace sdk {
namespace flow {

// NodeContext实现
json NodeContext::getInput(const std::string& name) const {
    auto it = inputs.find(name);
    if (it != inputs.end()) {
        return it->second;
    }
    return json{};
}

void NodeContext::setOutput(const std::string& name, const json& value) {
    outputs[name] = value;
}

json NodeContext::getFlowData(const std::string& key) const {
    auto it = flow_data.find(key);
    if (it != flow_data.end()) {
        return it->second;
    }
    return json{};
}

void NodeContext::setFlowData(const std::string& key, const json& value) {
    flow_data[key] = value;
}

// FlowNode实现
bool FlowNode::configure(const json& config) {
    config_ = config;
    return true;
}

void FlowNode::stop() {
    should_stop_ = true;
    setState(NodeState::IDLE);
}

void FlowNode::reset() {
    should_stop_ = false;
    is_paused_ = false;
    error_message_.clear();
    setState(NodeState::IDLE);
}

void FlowNode::setState(NodeState state) {
    state_ = state;
}

void FlowNode::setError(const std::string& message) {
    error_message_ = message;
    setState(NodeState::ERROR);
}

// BackgroundNode实现
BackgroundNode::~BackgroundNode() {
    stop();
    if (background_thread_ && background_thread_->joinable()) {
        background_thread_->join();
    }
}

bool BackgroundNode::startBackground(NodeContext& context) {
    if (is_running_) {
        return false;
    }
    
    is_running_ = true;
    should_stop_ = false;
    setState(NodeState::RUNNING);
    
    background_thread_ = std::make_unique<std::thread>([this, &context]() {
        try {
            runBackground(context);
        } catch (const std::exception& e) {
            setError(std::string("Background task error: ") + e.what());
        }
        is_running_ = false;
    });
    
    return true;
}

void BackgroundNode::stop() {
    should_stop_ = true;
    FlowNode::stop();
    
    if (background_thread_ && background_thread_->joinable()) {
        background_thread_->join();
        background_thread_.reset();
    }
}

void BackgroundNode::pause() {
    is_paused_ = true;
    setState(NodeState::PAUSED);
}

void BackgroundNode::resume() {
    is_paused_ = false;
    setState(NodeState::RUNNING);
}

// NodeFactory实现
NodeFactory& NodeFactory::getInstance() {
    static NodeFactory instance;
    return instance;
}

void NodeFactory::registerNode(const std::string& type_name, NodeCreator creator) {
    creators_[type_name] = creator;
    std::cout << "[NodeFactory] Registered node type: " << type_name << std::endl;
}

std::unique_ptr<FlowNode> NodeFactory::createNode(const std::string& type_name) {
    auto it = creators_.find(type_name);
    if (it != creators_.end()) {
        return it->second();
    }
    return nullptr;
}

std::vector<std::string> NodeFactory::getRegisteredTypes() const {
    std::vector<std::string> types;
    for (const auto& [name, _] : creators_) {
        types.push_back(name);
    }
    return types;
}

} // namespace flow
} // namespace sdk
} // namespace falconmind
