#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"

namespace falconmind::sdk::core {

Node::Node(std::string id)
    : id_(std::move(id)) {}

Node::~Node() = default;

bool Node::configure(const std::unordered_map<std::string, std::string>&) {
    // Week1 skeleton: 默认什么都不做
    return true;
}

bool Node::start() {
    // Week1 skeleton: 默认成功
    return true;
}

void Node::stop() {
    // Week1 skeleton: 默认无动作
}

void Node::process() {
    // Week1 skeleton: 由具体节点实现
}

std::shared_ptr<Pad> Node::getPad(const std::string& name) {
    auto it = pads_.find(name);
    if (it != pads_.end()) {
        return it->second;
    }
    return nullptr;
}

void Node::addPad(const std::shared_ptr<Pad>& pad) {
    if (!pad) return;
    pads_.emplace(pad->name(), pad);
}

} // namespace falconmind::sdk::core

