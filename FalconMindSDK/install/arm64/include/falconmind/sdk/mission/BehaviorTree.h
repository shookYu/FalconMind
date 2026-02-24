// FalconMindSDK - Behavior Tree implementation for Mission & Behavior
#pragma once

#include <atomic>
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
    virtual void reset() {}
    
    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }
    
protected:
    std::string name_;
};

using BehaviorNodePtr = std::shared_ptr<BehaviorNode>;

// Sequence Node: executes children in order until one fails
class SequenceNode : public BehaviorNode {
public:
    void addChild(const BehaviorNodePtr& child) { children_.push_back(child); }
    NodeStatus tick() override;
    void reset() override;

private:
    std::vector<BehaviorNodePtr> children_;
    std::size_t currentIndex_{0};
};

// Selector Node: executes children until one succeeds
class SelectorNode : public BehaviorNode {
public:
    void addChild(const BehaviorNodePtr& child) { children_.push_back(child); }
    NodeStatus tick() override;
    void reset() override;

private:
    std::vector<BehaviorNodePtr> children_;
    std::size_t currentIndex_{0};
};

// Parallel Node: executes all children concurrently
class ParallelNode : public BehaviorNode {
public:
    enum class Policy {
        RequireAll,     // All must succeed
        RequireOne,     // At least one must succeed
        RequireN        // At least N must succeed
    };
    
    explicit ParallelNode(Policy policy = Policy::RequireAll, int n = 1) 
        : policy_(policy), requiredN_(n) {}
    
    void addChild(const BehaviorNodePtr& child) { children_.push_back(child); }
    NodeStatus tick() override;
    void reset() override;

private:
    std::vector<BehaviorNodePtr> children_;
    Policy policy_;
    int requiredN_;
    std::vector<NodeStatus> childStatus_;
};

// Inverter Decorator: inverts the child status (Success->Failure, Failure->Success)
class InverterNode : public BehaviorNode {
public:
    void setChild(const BehaviorNodePtr& child) { child_ = child; }
    NodeStatus tick() override;
    void reset() override;

private:
    BehaviorNodePtr child_;
};

// Repeater Decorator: repeats the child N times or until it fails
class RepeaterNode : public BehaviorNode {
public:
    explicit RepeaterNode(int count = -1) : repeatCount_(count), currentCount_(0) {}
    void setChild(const BehaviorNodePtr& child) { child_ = child; }
    NodeStatus tick() override;
    void reset() override;

private:
    BehaviorNodePtr child_;
    int repeatCount_;  // -1 means infinite
    int currentCount_;
};

// Timeout Decorator: fails if child takes too long
class TimeoutNode : public BehaviorNode {
public:
    explicit TimeoutNode(double seconds) : timeoutSeconds_(seconds), elapsed_(0) {}
    void setChild(const BehaviorNodePtr& child) { child_ = child; }
    NodeStatus tick() override;
    void reset() override;

private:
    BehaviorNodePtr child_;
    double timeoutSeconds_;
    double elapsed_;
};

// AlwaysSuccess Decorator: always returns Success regardless of child status
class AlwaysSuccessNode : public BehaviorNode {
public:
    void setChild(const BehaviorNodePtr& child) { child_ = child; }
    NodeStatus tick() override;
    void reset() override;

private:
    BehaviorNodePtr child_;
};

// Behavior Tree Executor
class BehaviorTreeExecutor {
public:
    explicit BehaviorTreeExecutor(BehaviorNodePtr root) : root_(std::move(root)) {}
    NodeStatus tick();
    void reset() { if (root_) root_->reset(); }
    bool isRunning() const { return lastStatus_ == NodeStatus::Running; }

private:
    BehaviorNodePtr root_;
    NodeStatus lastStatus_{NodeStatus::Running};
};

} // namespace falconmind::sdk::mission

