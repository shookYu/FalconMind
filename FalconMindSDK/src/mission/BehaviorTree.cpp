#include "falconmind/sdk/mission/BehaviorTree.h"

namespace falconmind::sdk::mission {

NodeStatus SequenceNode::tick() {
    while (currentIndex_ < children_.size()) {
        auto status = children_[currentIndex_]->tick();
        if (status == NodeStatus::Running) {
            return NodeStatus::Running;
        }
        if (status == NodeStatus::Failure) {
            return NodeStatus::Failure;
        }
        // Success，继续下一个子节点
        ++currentIndex_;
    }
    return NodeStatus::Success;
}

} // namespace falconmind::sdk::mission

