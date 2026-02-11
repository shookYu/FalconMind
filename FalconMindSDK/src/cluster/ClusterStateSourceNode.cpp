#include "falconmind/sdk/cluster/ClusterStateSourceNode.h"
#include "falconmind/sdk/core/Pad.h"

#include <chrono>
#include <cstring>
#include <iostream>

namespace falconmind::sdk::cluster {

using namespace falconmind::sdk::core;

ClusterStateSourceNode::ClusterStateSourceNode() : Node("cluster_state_source") {
    addPad(std::make_shared<Pad>("cluster_state_out", PadType::Source));
}

bool ClusterStateSourceNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("self_id");
    if (it != params.end()) selfId_ = it->second;
    it = params.find("role");
    if (it != params.end()) role_ = it->second;
    it = params.find("members");
    if (it != params.end()) {
        memberIds_.clear();
        std::string s = it->second;
        size_t pos = 0;
        while (pos < s.size()) {
            size_t next = s.find(',', pos);
            std::string id = (next == std::string::npos) ? s.substr(pos) : s.substr(pos, next - pos);
            if (!id.empty()) memberIds_.push_back(id);
            pos = (next == std::string::npos) ? s.size() : next + 1;
        }
    }
    return true;
}

void ClusterStateSourceNode::setSelfId(const std::string& id) { selfId_ = id; }
void ClusterStateSourceNode::setRole(const std::string& r) { role_ = r; }
void ClusterStateSourceNode::setMemberIds(const std::vector<std::string>& ids) { memberIds_ = ids; }

bool ClusterStateSourceNode::start() {
    started_ = true;
    std::cout << "[ClusterStateSourceNode] start() self_id=" << selfId_
              << " role=" << role_ << " members=" << memberIds_.size() << std::endl;
    return true;
}

void ClusterStateSourceNode::pushState() {
    ClusterStatePacket pkt{};
    std::strncpy(pkt.self_id, selfId_.c_str(), ClusterStatePacket::kMaxIdLen - 1);
    pkt.self_id[ClusterStatePacket::kMaxIdLen - 1] = '\0';
    std::strncpy(pkt.role, role_.c_str(), ClusterStatePacket::kMaxIdLen - 1);
    pkt.role[ClusterStatePacket::kMaxIdLen - 1] = '\0';
    pkt.num_members = static_cast<int32_t>(memberIds_.size());
    for (size_t i = 0; i < memberIds_.size() && i < static_cast<size_t>(ClusterStatePacket::kMaxMemberIds); ++i) {
        std::strncpy(pkt.member_ids[i], memberIds_[i].c_str(), ClusterStatePacket::kMaxIdLen - 1);
        pkt.member_ids[i][ClusterStatePacket::kMaxIdLen - 1] = '\0';
    }
    pkt.timestamp_ns = timestampNs_++;

    auto outPad = getPad("cluster_state_out");
    if (outPad)
        outPad->pushToConnections(&pkt, sizeof(pkt));
}

void ClusterStateSourceNode::process() {
    if (!started_) return;
    pushState();
}

} // namespace falconmind::sdk::cluster
