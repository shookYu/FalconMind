// FalconMindSDK - Minimal Behavior Tree implementation for Mission & Behavior
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace falconmind::sdk::mission {

enum class NodeStatus {
    Running,
    Success,
    Failure
};

class BehaviorNode {
public:
    virtual ~BehaviorNode() = default;
    virtual NodeStatus tick() = 0;
};

using BehaviorNodePtr = std::shared_ptr<BehaviorNode>;

// 顺序节点：依次执行子节点，遇到 Running 或 Failure 时立即返回
class SequenceNode : public BehaviorNode {
public:
    void addChild(const BehaviorNodePtr& child) { children_.push_back(child); }

    NodeStatus tick() override;

private:
    std::vector<BehaviorNodePtr> children_;
    std::size_t currentIndex_{0};
};

// 最小执行器：每帧调用一次 root->tick()
class BehaviorTreeExecutor {
public:
    explicit BehaviorTreeExecutor(BehaviorNodePtr root) : root_(std::move(root)) {}

    NodeStatus tick() {
        if (!root_) return NodeStatus::Failure;
        return root_->tick();
    }

private:
    BehaviorNodePtr root_;
};

} // namespace falconmind::sdk::mission

