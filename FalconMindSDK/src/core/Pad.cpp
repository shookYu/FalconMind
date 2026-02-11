#include "falconmind/sdk/core/Pad.h"

namespace falconmind::sdk::core {

Pad::Pad(std::string name, PadType type)
    : name_(std::move(name)), type_(type) {}

bool Pad::connectTo(std::shared_ptr<Pad> targetPad, const std::string& targetNodeId, const std::string& targetPadName) {
    if (!targetPad) {
        return false;
    }
    
    // 检查Pad类型兼容性：Source Pad只能连接到Sink Pad
    if (type_ == PadType::Source && targetPad->type_ != PadType::Sink) {
        return false;
    }
    if (type_ == PadType::Sink && targetPad->type_ != PadType::Source) {
        return false;
    }
    
    // 检查是否已经连接
    for (const auto& conn : connections_) {
        if (auto pad = conn.targetPad.lock()) {
            if (pad == targetPad) {
                return true;  // 已经连接
            }
        }
    }
    
    // 创建连接
    PadConnection conn;
    conn.targetPad = targetPad;
    conn.targetNodeId = targetNodeId;
    conn.targetPadName = targetPadName;
    connections_.push_back(conn);
    
    return true;
}

bool Pad::disconnect() {
    connections_.clear();
    return true;
}

void Pad::pushToConnections(const void* data, size_t size) const {
    if (type_ != PadType::Source || !data) return;
    for (const auto& conn : connections_) {
        if (auto target = conn.targetPad.lock()) {
            DataCallback cb = target->getDataCallback();
            if (cb) cb(data, size);
        }
    }
}

} // namespace falconmind::sdk::core

