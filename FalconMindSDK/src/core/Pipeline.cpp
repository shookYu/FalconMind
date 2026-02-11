#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"
#include <unordered_set>

namespace falconmind::sdk::core {

Pipeline::Pipeline(const PipelineConfig& cfg)
    : config_(cfg) {}

Pipeline::~Pipeline() = default;

bool Pipeline::addNode(const std::shared_ptr<Node>& node) {
    if (!node) return false;
    const auto& id = node->id();
    if (nodes_.count(id) > 0) {
        return false;
    }
    nodes_.emplace(id, node);
    return true;
}

bool Pipeline::link(const std::string& srcNodeId,
                    const std::string& srcPadName,
                    const std::string& dstNodeId,
                    const std::string& dstPadName) {
    // 检查源节点和目标节点是否存在
    auto srcIt = nodes_.find(srcNodeId);
    auto dstIt = nodes_.find(dstNodeId);
    if (srcIt == nodes_.end() || dstIt == nodes_.end()) {
        return false;
    }
    
    auto srcNode = srcIt->second;
    auto dstNode = dstIt->second;
    
    // 检查Pad是否存在
    auto srcPad = srcNode->getPad(srcPadName);
    auto dstPad = dstNode->getPad(dstPadName);
    if (!srcPad || !dstPad) {
        return false;
    }
    
    // 检查Pad类型兼容性：Source Pad只能连接到Sink Pad
    if (srcPad->type() != PadType::Source || dstPad->type() != PadType::Sink) {
        return false;
    }
    
    // 检查连接是否已存在
    LinkKey key;
    key.srcNodeId = srcNodeId;
    key.srcPadName = srcPadName;
    key.dstNodeId = dstNodeId;
    key.dstPadName = dstPadName;
    
    if (links_.count(key) > 0) {
        return true;  // 连接已存在
    }
    
    // 通过Pad的connectTo方法建立连接
    if (!srcPad->connectTo(dstPad, dstNodeId, dstPadName)) {
        return false;
    }
    
    // 存储连接信息
    links_.insert(key);
    
    return true;
}

bool Pipeline::unlink(const std::string& srcNodeId,
                      const std::string& srcPadName,
                      const std::string& dstNodeId,
                      const std::string& dstPadName) {
    // 检查连接是否存在
    LinkKey key;
    key.srcNodeId = srcNodeId;
    key.srcPadName = srcPadName;
    key.dstNodeId = dstNodeId;
    key.dstPadName = dstPadName;
    
    if (links_.count(key) == 0) {
        return false;  // 连接不存在
    }
    
    // 获取源节点和Pad
    auto srcIt = nodes_.find(srcNodeId);
    if (srcIt == nodes_.end()) {
        return false;
    }
    
    auto srcPad = srcIt->second->getPad(srcPadName);
    if (!srcPad) {
        return false;
    }
    
    // 断开Pad连接
    srcPad->disconnect();
    
    // 从连接列表中移除
    links_.erase(key);
    
    return true;
}

std::shared_ptr<Node> Pipeline::getNode(const std::string& nodeId) {
    auto it = nodes_.find(nodeId);
    if (it != nodes_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<Pipeline::LinkInfo> Pipeline::getLinks() const {
    std::vector<LinkInfo> result;
    result.reserve(links_.size());
    
    for (const auto& link : links_) {
        LinkInfo info;
        info.srcNodeId = link.srcNodeId;
        info.srcPadName = link.srcPadName;
        info.dstNodeId = link.dstNodeId;
        info.dstPadName = link.dstPadName;
        result.push_back(info);
    }
    
    return result;
}

bool Pipeline::setState(PipelineState newState) {
    // Week1 skeleton: 不做复杂状态机，仅记录状态。
    state_ = newState;
    return true;
}

} // namespace falconmind::sdk::core

