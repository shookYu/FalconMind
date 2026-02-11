// FalconMindSDK - Pipeline core interface (week1 skeleton)
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace falconmind::sdk::core {

class Node;
class Pad;

enum class PipelineState {
    Null,
    Ready,
    Playing,
    Paused
};

struct PipelineConfig {
    std::string pipelineId;
    std::string name;
    std::string description;
};

class Pipeline {
public:
    explicit Pipeline(const PipelineConfig& cfg);
    ~Pipeline();

    const std::string& id() const noexcept { return config_.pipelineId; }

    bool addNode(const std::shared_ptr<Node>& node);
    bool link(const std::string& srcNodeId,
              const std::string& srcPadName,
              const std::string& dstNodeId,
              const std::string& dstPadName);
    
    bool unlink(const std::string& srcNodeId,
                const std::string& srcPadName,
                const std::string& dstNodeId,
                const std::string& dstPadName);

    bool setState(PipelineState newState);
    PipelineState state() const noexcept { return state_; }
    
    // 获取节点
    std::shared_ptr<Node> getNode(const std::string& nodeId);
    
    // 获取所有连接信息（用于调试和测试）
    struct LinkInfo {
        std::string srcNodeId;
        std::string srcPadName;
        std::string dstNodeId;
        std::string dstPadName;
    };
    std::vector<LinkInfo> getLinks() const;

private:
    PipelineConfig config_;
    PipelineState state_{PipelineState::Null};

    std::unordered_map<std::string, std::shared_ptr<Node>> nodes_;
    
    // 存储连接信息（用于快速查找和验证）
    struct LinkKey {
        std::string srcNodeId;
        std::string srcPadName;
        std::string dstNodeId;
        std::string dstPadName;
        
        bool operator==(const LinkKey& other) const {
            return srcNodeId == other.srcNodeId &&
                   srcPadName == other.srcPadName &&
                   dstNodeId == other.dstNodeId &&
                   dstPadName == other.dstPadName;
        }
    };
    
    struct LinkKeyHash {
        std::size_t operator()(const LinkKey& key) const {
            return std::hash<std::string>()(key.srcNodeId) ^
                   (std::hash<std::string>()(key.srcPadName) << 1) ^
                   (std::hash<std::string>()(key.dstNodeId) << 2) ^
                   (std::hash<std::string>()(key.dstPadName) << 3);
        }
    };
    
    std::unordered_set<LinkKey, LinkKeyHash> links_;  // 存储所有连接
};

} // namespace falconmind::sdk::core

