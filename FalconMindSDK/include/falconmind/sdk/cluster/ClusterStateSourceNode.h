// FalconMindSDK - 集群状态源节点（PRD 3.1.2.2 集群与协同模块）
// 输出当前节点的集群状态（本机 ID、角色、成员列表），供下游或 NodeAgent 使用；可配置或对接集群中间件。
#pragma once

#include "falconmind/sdk/core/Node.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace falconmind::sdk::cluster {

/** 经 cluster_state_out 推送的集群状态包（二进制） */
struct ClusterStatePacket {
    static constexpr int kMaxMemberIds = 16;
    static constexpr int kMaxIdLen = 64;
    char     self_id[kMaxIdLen]{};
    char     role[kMaxIdLen]{};  // "leader" / "follower" / "standalone"
    int32_t  num_members{0};
    char     member_ids[kMaxMemberIds][kMaxIdLen]{};
    int64_t  timestamp_ns{0};
};

class ClusterStateSourceNode : public core::Node {
public:
    ClusterStateSourceNode();

    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void process() override;

    void setSelfId(const std::string& id);
    void setRole(const std::string& role);
    void setMemberIds(const std::vector<std::string>& ids);

private:
    void pushState();

    bool started_{false};
    std::string selfId_{"node_0"};
    std::string role_{"standalone"};
    std::vector<std::string> memberIds_;
    int64_t timestampNs_{0};
};

} // namespace falconmind::sdk::cluster
