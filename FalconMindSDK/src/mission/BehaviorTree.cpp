#include "falconmind/sdk/mission/BehaviorTree.h"

namespace falconmind::sdk::mission {

// Sequence Node - executes children in order until one fails
NodeStatus SequenceNode::tick() {
    while (currentIndex_ < children_.size()) {
        auto status = children_[currentIndex_]->tick();
        if (status == NodeStatus::Running) {
            return NodeStatus::Running;
        }
        if (status == NodeStatus::Failure) {
            return NodeStatus::Failure;
        }
        ++currentIndex_;
    }
    return NodeStatus::Success;
}

void SequenceNode::reset() {
    currentIndex_ = 0;
    for (auto& child : children_) {
        child->reset();
    }
}

// Selector Node - executes children until one succeeds
NodeStatus SelectorNode::tick() {
    while (currentIndex_ < children_.size()) {
        auto status = children_[currentIndex_]->tick();
        if (status == NodeStatus::Running) {
            return NodeStatus::Running;
        }
        if (status == NodeStatus::Success) {
            return NodeStatus::Success;
        }
        ++currentIndex_;
    }
    return NodeStatus::Failure;
}

void SelectorNode::reset() {
    currentIndex_ = 0;
    for (auto& child : children_) {
        child->reset();
    }
}

// Parallel Node - executes all children
NodeStatus ParallelNode::tick() {
    if (children_.empty()) {
        return NodeStatus::Success;
    }
    
    if (childStatus_.size() != children_.size()) {
        childStatus_.resize(children_.size(), NodeStatus::Running);
    }
    
    int successCount = 0;
    int failureCount = 0;
    int runningCount = 0;
    
    for (size_t i = 0; i < children_.size(); ++i) {
        if (childStatus_[i] == NodeStatus::Running) {
            childStatus_[i] = children_[i]->tick();
        }
        
        if (childStatus_[i] == NodeStatus::Success) {
            ++successCount;
        } else if (childStatus_[i] == NodeStatus::Failure) {
            ++failureCount;
        } else {
            ++runningCount;
        }
    }
    
    switch (policy_) {
        case Policy::RequireAll:
            if (successCount == static_cast<int>(children_.size())) {
                return NodeStatus::Success;
            }
            if (failureCount > 0) {
                return NodeStatus::Failure;
            }
            break;
        case Policy::RequireOne:
            if (successCount >= 1) {
                return NodeStatus::Success;
            }
            if (failureCount == static_cast<int>(children_.size())) {
                return NodeStatus::Failure;
            }
            break;
        case Policy::RequireN:
            if (successCount >= requiredN_) {
                return NodeStatus::Success;
            }
            if (failureCount > static_cast<int>(children_.size()) - requiredN_) {
                return NodeStatus::Failure;
            }
            break;
    }
    
    return NodeStatus::Running;
}

void ParallelNode::reset() {
    std::fill(childStatus_.begin(), childStatus_.end(), NodeStatus::Running);
    for (auto& child : children_) {
        child->reset();
    }
}

// Inverter Node - inverts child result
NodeStatus InverterNode::tick() {
    if (!child_) {
        return NodeStatus::Failure;
    }
    auto status = child_->tick();
    if (status == NodeStatus::Success) {
        return NodeStatus::Failure;
    }
    if (status == NodeStatus::Failure) {
        return NodeStatus::Success;
    }
    return NodeStatus::Running;
}

void InverterNode::reset() {
    if (child_) {
        child_->reset();
    }
}

// Repeater Node - repeats child execution
NodeStatus RepeaterNode::tick() {
    if (!child_) {
        return NodeStatus::Failure;
    }
    
    if (repeatCount_ > 0 && currentCount_ >= repeatCount_) {
        return NodeStatus::Success;
    }
    
    auto status = child_->tick();
    if (status == NodeStatus::Running) {
        return NodeStatus::Running;
    }
    
    if (status == NodeStatus::Success || status == NodeStatus::Failure) {
        child_->reset();
        ++currentCount_;
        if (repeatCount_ > 0 && currentCount_ >= repeatCount_) {
            return NodeStatus::Success;
        }
        return NodeStatus::Running;
    }
    
    return NodeStatus::Running;
}

void RepeaterNode::reset() {
    currentCount_ = 0;
    if (child_) {
        child_->reset();
    }
}

// Timeout Node - limits execution time
NodeStatus TimeoutNode::tick() {
    if (!child_) {
        return NodeStatus::Failure;
    }
    
    elapsed_ += 1.0 / 30.0; // Assume 30 FPS
    if (elapsed_ >= timeoutSeconds_) {
        return NodeStatus::Failure;
    }
    
    return child_->tick();
}

void TimeoutNode::reset() {
    elapsed_ = 0;
    if (child_) {
        child_->reset();
    }
}

// AlwaysSuccess Node - always returns success
NodeStatus AlwaysSuccessNode::tick() {
    if (!child_) {
        return NodeStatus::Success;
    }
    child_->tick();
    return NodeStatus::Success;
}

void AlwaysSuccessNode::reset() {
    if (child_) {
        child_->reset();
    }
}

// Behavior Tree Executor
NodeStatus BehaviorTreeExecutor::tick() {
    if (!root_) {
        return NodeStatus::Failure;
    }
    lastStatus_ = root_->tick();
    return lastStatus_;
}

} // namespace falconmind::sdk::mission

