/**
 * @file DistributedTaskAllocator.cpp
 * @brief Implementation of distributed task allocation using auction algorithm
 */

#include "nodeagent/DistributedTaskAllocator.h"
#include "nodeagent/Logger.h"
#include <math>
#include <algorithm>

namespace nodeagent {

DistributedTaskAllocator::DistributedTaskAllocator(const std::string& localUavId)
    : localUavId_(localUavId)
    , isLeader_(false)
    , isRunning_(false) {
    LOG_INFO("DistributedTaskAllocator", "Constructor - UAV: " + localUavId);
    startTime_ = std::chrono::steady_clock::now();
}

DistributedTaskAllocator::~DistributedTaskAllocator() {
    LOG_INFO("DistributedTaskAllocator", "Destructor");
    shutdown();
}

bool DistributedTaskAllocator::initialize(bool isLeader) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isRunning_) {
        LOG_WARNING("DistributedTaskAllocator", "Already initialized");
        return true;
    }
    
    isLeader_ = isLeader;
    isRunning_ = true;
    
    // Initialize local capability
    localCapability_.uavId = localUavId_;
    localCapability_.batteryLevel = 100.0;
    localCapability_.computePower = 6.0;
    localCapability_.hasNPU = true;
    localCapability_.currentLoad = 0.0;
    localCapability_.signalStrength = 100.0;
    
    // Start allocation thread if leader
    if (isLeader_) {
        startAllocationThread();
    }
    
    LOG_INFO("DistributedTaskAllocator", "Initialized - Leader: " + std::to_string(isLeader));
    return true;
}

void DistributedTaskAllocator::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isRunning_) {
            return;
        }
        isRunning_ = false;
    }
    
    cv_.notify_all();
    stopAllocationThread();
    
    LOG_INFO("DistributedTaskAllocator", "Shutdown complete");
}

void DistributedTaskAllocator::setAllocationCallback(AllocationCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    allocationCallback_ = callback;
}

void DistributedTaskAllocator::setTaskCompleteCallback(TaskCompleteCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    taskCompleteCallback_ = callback;
}

void DistributedTaskAllocator::setRebalanceCallback(RebalanceCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    rebalanceCallback_ = callback;
}

std::string DistributedTaskAllocator::submitTask(const DistributedTask& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string taskId = task.taskId.empty() ? generateTaskId() : task.taskId;
    
    DistributedTask newTask = task;
    newTask.taskId = taskId;
    newTask.createdAt = std::chrono::system_clock::now();
    newTask.status = TaskStatus::PENDING;
    
    tasks_[taskId] = newTask;
    ++stats_.totalTasksSubmitted;
    
    // Add to pending allocations if leader
    if (isLeader_) {
        pendingAllocations_.push(taskId);
        cv_.notify_one();
    }
    
    LOG_INFO("DistributedTaskAllocator", "Task submitted: " + taskId + 
             " (Type: " + std::to_string(static_cast<int>(task.type)) + ")");
    
    return taskId;
}

std::vector<std::string> DistributedTaskAllocator::submitTasks(
    const std::vector<DistributedTask>& tasks) {
    std::vector<std::string> taskIds;
    
    for (const auto& task : tasks) {
        taskIds.push_back(submitTask(task));
    }
    
    return taskIds;
}

void DistributedTaskAllocator::updateLocalCapability(const UavCapability& capability) {
    std::lock_guard<std::mutex> lock(mutex_);
    localCapability_ = capability;
}

void DistributedTaskAllocator::updatePeerCapability(const UavCapability& capability) {
    std::lock_guard<std::mutex> lock(mutex_);
    peerCapabilities_[capability.uavId] = capability;
}

void DistributedTaskAllocator::removePeer(const std::string& uavId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    peerCapabilities_.erase(uavId);
    
    // Reassign tasks from removed peer
    std::vector<std::string> tasksToReassign;
    for (auto& [taskId, task] : tasks_) {
        if (task.assignedUavId == uavId && task.status == TaskStatus::EXECUTING) {
            task.status = TaskStatus::PENDING;
            task.assignedUavId.clear();
            tasksToReassign.push_back(taskId);
        }
    }
    
    for (const auto& taskId : tasksToReassign) {
        pendingAllocations_.push(taskId);
    }
    
    if (!tasksToReassign.empty()) {
        cv_.notify_one();
        LOG_INFO("DistributedTaskAllocator", "Reassigning " + 
                 std::to_string(tasksToReassign.size()) + " tasks from " + uavId);
    }
}

void DistributedTaskAllocator::initiateBidding() {
    if (!isLeader_) {
        LOG_WARNING("DistributedTaskAllocator", "Only leader can initiate bidding");
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Add all unassigned tasks to pending
    for (auto& [taskId, task] : tasks_) {
        if (task.status == TaskStatus::PENDING && task.assignedUavId.empty()) {
            pendingAllocations_.push(taskId);
        }
    }
    
    cv_.notify_one();
}

TaskBid DistributedTaskAllocator::submitBid(const std::string& taskId,
                                            const TaskRequirements& requirements) {
    TaskBid bid;
    bid.taskId = taskId;
    bid.uavId = localUavId_;
    bid.bidTime = std::chrono::system_clock::now();
    
    // Check if we can execute
    bid.canExecute = canExecuteTask(requirements, localCapability_);
    
    if (bid.canExecute) {
        bid.bidScore = calculateBidScore(requirements, localCapability_);
        bid.estimatedCompletionTime = requirements.estimatedDuration;
        bid.confidence = std::min(1.0, localCapability_.batteryLevel / 100.0);
    } else {
        bid.reason = "Insufficient capability";
        bid.bidScore = 0.0;
        bid.confidence = 0.0;
    }
    
    LOG_DEBUG("DistributedTaskAllocator", "Bid submitted for " + taskId + 
              ": score=" + std::to_string(bid.bidScore));
    
    return bid;
}

void DistributedTaskAllocator::receiveBid(const TaskBid& bid) {
    if (!isLeader_) {
        LOG_WARNING("DistributedTaskAllocator", "Only leader can receive bids");
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    bids_[bid.taskId].push_back(bid);
    ++stats_.totalBidsReceived;
    
    LOG_DEBUG("DistributedTaskAllocator", "Bid received from " + bid.uavId + 
              " for " + bid.taskId + ": score=" + std::to_string(bid.bidScore));
}

AllocationResult DistributedTaskAllocator::assignTask(const std::string& taskId) {
    if (!isLeader_) {
        return AllocationResult{taskId, "", false, "Not leader", 0.0, {}};
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto taskIt = tasks_.find(taskId);
    if (taskIt == tasks_.end()) {
        return AllocationResult{taskId, "", false, "Task not found", 0.0, {}};
    }
    
    // Collect bids if not already done
    auto& taskBids = bids_[taskId];
    if (taskBids.empty()) {
        // No bids received, try to assign to self if possible
        if (canExecuteTask(taskIt->second.requirements, localCapability_)) {
            taskBids.push_back(submitBid(taskId, taskIt->second.requirements));
        }
    }
    
    if (taskBids.empty()) {
        return AllocationResult{taskId, "", false, "No bids received", 0.0, {}};
    }
    
    // Select winner (highest bid score)
    auto winnerIt = std::max_element(taskBids.begin(), taskBids.end(),
        [](const TaskBid& a, const TaskBid& b) {
            return a.bidScore < b.bidScore;
        });
    
    AllocationResult result;
    result.taskId = taskId;
    result.assignedUavId = winnerIt->uavId;
    result.success = true;
    result.bidScore = winnerIt->bidScore;
    result.allBids = taskBids;
    
    // Update task
    taskIt->second.assignedUavId = result.assignedUavId;
    taskIt->second.status = TaskStatus::ASSIGNED;
    taskIt->second.assignedAt = std::chrono::system_clock::now();
    
    ++stats_.totalTasksAssigned;
    
    // Calculate allocation time
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - startTime_).count();
    stats_.averageAllocationTime = (stats_.averageAllocationTime * (stats_.totalTasksAssigned - 1) +
                                    elapsed) / stats_.totalTasksAssigned;
    
    // Notify
    notifyAssignment(result);
    
    LOG_INFO("DistributedTaskAllocator", "Task " + taskId + " assigned to " + 
             result.assignedUavId + " with score " + std::to_string(result.bidScore));
    
    return result;
}

void DistributedTaskAllocator::receiveAssignment(const std::string& taskId,
                                                  const std::string& assignedUavId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        it->second.assignedUavId = assignedUavId;
        it->second.status = TaskStatus::ASSIGNED;
        it->second.assignedAt = std::chrono::system_clock::now();
        
        if (assignedUavId == localUavId_) {
            LOG_INFO("DistributedTaskAllocator", "Task " + taskId + " assigned to me");
            // Start execution
            it->second.status = TaskStatus::EXECUTING;
        }
    }
}

void DistributedTaskAllocator::updateTaskProgress(const std::string& taskId, int progress) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        it->second.progress = std::max(0, std::min(100, progress));
        LOG_DEBUG("DistributedTaskAllocator", "Task " + taskId + 
                  " progress: " + std::to_string(it->second.progress) + "%");
    }
}

void DistributedTaskAllocator::completeTask(const std::string& taskId,
                                            const nlohmann::json& result) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        it->second.status = TaskStatus::COMPLETED;
        it->second.progress = 100;
        it->second.result = result;
        
        ++stats_.totalTasksCompleted;
        
        if (taskCompleteCallback_) {
            taskCompleteCallback_(taskId, it->second.assignedUavId, result);
        }
        
        LOG_INFO("DistributedTaskAllocator", "Task " + taskId + " completed");
    }
}

bool DistributedTaskAllocator::cancelTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(taskId);
    if (it == tasks_.end()) {
        return false;
    }
    
    it->second.status = TaskStatus::CANCELLED;
    ++stats_.totalTasksCancelled;
    
    LOG_INFO("DistributedTaskAllocator", "Task " + taskId + " cancelled");
    return true;
}

void DistributedTaskAllocator::handlePartition(const std::vector<std::string>& partitionMembers) {
    LOG_INFO("DistributedTaskAllocator", "Handling partition with " + 
             std::to_string(partitionMembers.size()) + " members");
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Keep only members in partition
    std::set<std::string> partitionSet(partitionMembers.begin(), partitionMembers.end());
    
    // Reassign tasks assigned to UAVs outside partition
    std::vector<std::string> tasksToReassign;
    for (auto& [taskId, task] : tasks_) {
        if (!task.assignedUavId.empty() && 
            partitionSet.find(task.assignedUavId) == partitionSet.end() &&
            task.status != TaskStatus::COMPLETED) {
            task.status = TaskStatus::PENDING;
            task.assignedUavId.clear();
            tasksToReassign.push_back(taskId);
        }
    }
    
    // Remove peers outside partition
    for (auto it = peerCapabilities_.begin(); it != peerCapabilities_.end();) {
        if (partitionSet.find(it->first) == partitionSet.end()) {
            it = peerCapabilities_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Reassign tasks within partition
    for (const auto& taskId : tasksToReassign) {
        pendingAllocations_.push(taskId);
    }
    
    cv_.notify_one();
    
    LOG_INFO("DistributedTaskAllocator", "Redistributed " + 
             std::to_string(tasksToReassign.size()) + " tasks within partition");
}

void DistributedTaskAllocator::handlePartitionMerge(
    const std::vector<std::string>& mergedMembers) {
    LOG_INFO("DistributedTaskAllocator", "Handling partition merge");
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Reconcile duplicate task assignments
    std::map<std::string, std::vector<std::string>> taskAssignments;
    
    for (const auto& [taskId, task] : tasks_) {
        if (!task.assignedUavId.empty()) {
            taskAssignments[taskId].push_back(task.assignedUavId);
        }
    }
    
    // Detect conflicts (same task assigned to multiple UAVs)
    int conflictsResolved = 0;
    for (const auto& [taskId, assignees] : taskAssignments) {
        if (assignees.size() > 1) {
            // Conflict: choose the one with higher capability
            std::string bestUav = assignees[0];
            double bestScore = 0.0;
            
            for (const auto& uavId : assignees) {
                auto capIt = peerCapabilities_.find(uavId);
                if (capIt != peerCapabilities_.end()) {
                    double score = capIt->second.batteryLevel;
                    if (score > bestScore) {
                        bestScore = score;
                        bestUav = uavId;
                    }
                }
            }
            
            // Cancel assignments for others
            for (const auto& uavId : assignees) {
                if (uavId != bestUav) {
                    // In real implementation, send cancel message
                    LOG_INFO("DistributedTaskAllocator", "Cancelling duplicate " +
                             taskId + " from " + uavId);
                }
            }
            
            ++conflictsResolved;
        }
    }
    
    LOG_INFO("DistributedTaskAllocator", "Resolved " + std::to_string(conflictsResolved) + 
             " conflicts after merge");
}

void DistributedTaskAllocator::rebalanceLoad() {
    if (!isLeader_) {
        return;
    }
    
    LOG_INFO("DistributedTaskAllocator", "Rebalancing load");
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Calculate current load per UAV
    std::map<std::string, int> taskCounts;
    for (const auto& [taskId, task] : tasks_) {
        if (task.status == TaskStatus::EXECUTING && !task.assignedUavId.empty()) {
            ++taskCounts[task.assignedUavId];
        }
    }
    
    // Find overloaded and underloaded UAVs
    int totalTasks = 0;
    int activeUavs = 1 + static_cast<int>(peerCapabilities_.size());
    
    for (const auto& [uavId, count] : taskCounts) {
        totalTasks += count;
    }
    
    double averageLoad = static_cast<double>(totalTasks) / activeUavs;
    
    std::vector<std::string> overloaded;
    std::vector<std::string> underloaded;
    
    for (const auto& [uavId, count] : taskCounts) {
        if (count > averageLoad + 1) {
            overloaded.push_back(uavId);
        } else if (count < averageLoad - 1) {
            underloaded.push_back(uavId);
        }
    }
    
    // Reassign tasks from overloaded to underloaded
    std::vector<std::string> reassignedTasks;
    for (const auto& overloadedUav : overloaded) {
        for (auto& [taskId, task] : tasks_) {
            if (task.assignedUavId == overloadedUav && 
                task.status == TaskStatus::EXECUTING) {
                // Find underloaded UAV
                if (!underloaded.empty()) {
                    std::string newUav = underloaded[0];
                    task.assignedUavId = newUav;
                    reassignedTasks.push_back(taskId);
                    
                    // Update counts
                    --taskCounts[overloadedUav];
                    ++taskCounts[newUav];
                    
                    if (taskCounts[newUav] >= averageLoad) {
                        underloaded.erase(underloaded.begin());
                    }
                    
                    if (taskCounts[overloadedUav] <= averageLoad) {
                        break;
                    }
                }
            }
        }
    }
    
    if (!reassignedTasks.empty()) {
        ++stats_.totalRebalances;
        
        if (rebalanceCallback_) {
            rebalanceCallback_(reassignedTasks);
        }
        
        LOG_INFO("DistributedTaskAllocator", "Rebalanced " + 
                 std::to_string(reassignedTasks.size()) + " tasks");
    }
}

double DistributedTaskAllocator::calculateBidScore(
    const TaskRequirements& requirements,
    const UavCapability& capability) const {
    
    double score = 0.0;
    
    // Battery factor (0-40 points)
    score += (capability.batteryLevel / 100.0) * 40.0;
    
    // Load factor - prefer less loaded UAVs (0-30 points)
    score += (1.0 - capability.currentLoad) * 30.0;
    
    // Compute power factor (0-20 points)
    score += std::min(1.0, capability.computePower / 6.0) * 20.0;
    
    // NPU bonus (0-10 points)
    if (capability.hasNPU) {
        score += 10.0;
    }
    
    // Signal strength factor
    score += (capability.signalStrength / 100.0) * 5.0;
    
    // Sensor matching
    int matchedSensors = 0;
    for (const auto& sensor : requirements.requiredSensors) {
        if (std::find(capability.sensors.begin(), capability.sensors.end(), sensor) 
            != capability.sensors.end()) {
            ++matchedSensors;
        }
    }
    if (!requirements.requiredSensors.empty()) {
        score += (static_cast<double>(matchedSensors) / requirements.requiredSensors.size()) * 10.0;
    }
    
    // Priority boost
    score *= requirements.priority;
    
    return score;
}

bool DistributedTaskAllocator::canExecuteTask(
    const TaskRequirements& requirements,
    const UavCapability& capability) const {
    
    // Check battery
    if (capability.batteryLevel < requirements.minBatteryLevel) {
        return false;
    }
    
    // Check compute
    if (capability.computePower < requirements.minComputePower) {
        return false;
    }
    
    // Check sensors
    for (const auto& sensor : requirements.requiredSensors) {
        if (std::find(capability.sensors.begin(), capability.sensors.end(), sensor)
            == capability.sensors.end()) {
            return false;
        }
    }
    
    // Check capabilities
    for (const auto& cap : requirements.requiredCapabilities) {
        // In real implementation, check actual capability list
        if (cap == "NPU" && !capability.hasNPU) {
            return false;
        }
    }
    
    // Check flight time
    double requiredMinutes = requirements.estimatedDuration / 60.0;
    if (requiredMinutes > capability.maxFlightTime * 0.8) {  // 80% safety margin
        return false;
    }
    
    return true;
}

std::optional<DistributedTask> DistributedTaskAllocator::getTask(
    const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(taskId);
    if (it != tasks_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<DistributedTask> DistributedTaskAllocator::getAllTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<DistributedTask> result;
    for (const auto& [taskId, task] : tasks_) {
        result.push_back(task);
    }
    return result;
}

std::vector<DistributedTask> DistributedTaskAllocator::getTasksForUav(
    const std::string& uavId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<DistributedTask> result;
    for (const auto& [taskId, task] : tasks_) {
        if (task.assignedUavId == uavId) {
            result.push_back(task);
        }
    }
    return result;
}

std::vector<DistributedTask> DistributedTaskAllocator::getUnassignedTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<DistributedTask> result;
    for (const auto& [taskId, task] : tasks_) {
        if (task.status == TaskStatus::PENDING && task.assignedUavId.empty()) {
            result.push_back(task);
        }
    }
    return result;
}

LoadBalanceMetrics DistributedTaskAllocator::getLoadBalanceMetrics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LoadBalanceMetrics metrics;
    
    std::map<std::string, int> taskCounts;
    for (const auto& [taskId, task] : tasks_) {
        if (task.status == TaskStatus::EXECUTING && !task.assignedUavId.empty()) {
            ++taskCounts[task.assignedUavId];
        }
    }
    
    if (taskCounts.empty()) {
        return metrics;
    }
    
    int total = 0;
    int maxCount = 0;
    int minCount = INT_MAX;
    
    for (const auto& [uavId, count] : taskCounts) {
        total += count;
        maxCount = std::max(maxCount, count);
        minCount = std::min(minCount, count);
    }
    
    double avg = static_cast<double>(total) / taskCounts.size();
    
    metrics.averageLoad = avg;
    metrics.maxLoad = maxCount;
    metrics.minLoad = minCount;
    
    double variance = 0.0;
    for (const auto& [uavId, count] : taskCounts) {
        variance += std::pow(count - avg, 2);
    }
    metrics.loadVariance = variance / taskCounts.size();
    
    for (const auto& [uavId, count] : taskCounts) {
        if (count > avg * 1.5) {
            ++metrics.overloadedUavs;
        } else if (count < avg * 0.5) {
            ++metrics.underloadedUavs;
        }
    }
    
    return metrics;
}

bool DistributedTaskAllocator::isLeader() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isLeader_;
}

void DistributedTaskAllocator::setLeaderStatus(bool isLeader) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    isLeader_ = isLeader;
    
    if (isLeader_ && !isRunning_) {
        startAllocationThread();
    } else if (!isLeader_) {
        stopAllocationThread();
    }
}

DistributedTaskAllocator::Statistics DistributedTaskAllocator::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

// Private methods

void DistributedTaskAllocator::collectBids(const std::string& taskId) {
    // In real implementation, send bid requests to all peers
    // For now, this is a placeholder
}

void DistributedTaskAllocator::selectWinner(const std::string& taskId) {
    // Called by leader to select winner from collected bids
    assignTask(taskId);
}

void DistributedTaskAllocator::redistributePartitionTasks() {
    // Move tasks from outside partition to inside
}

void DistributedTaskAllocator::reconcileMergedTasks() {
    // Resolve conflicts after merge
}

void DistributedTaskAllocator::checkAndRebalance() {
    auto metrics = getLoadBalanceMetrics();
    
    if (metrics.loadVariance > 4.0) {  // Significant imbalance
        rebalanceLoad();
    }
}

double DistributedTaskAllocator::calculateUavLoad(const std::string& uavId) const {
    int taskCount = 0;
    for (const auto& [taskId, task] : tasks_) {
        if (task.assignedUavId == uavId && task.status == TaskStatus::EXECUTING) {
            ++taskCount;
        }
    }
    
    auto capIt = peerCapabilities_.find(uavId);
    if (capIt != peerCapabilities_.end()) {
        return taskCount * capIt->second.currentLoad;
    }
    
    return taskCount;
}

void DistributedTaskAllocator::notifyAssignment(const AllocationResult& result) {
    if (allocationCallback_) {
        allocationCallback_(result);
    }
}

void DistributedTaskAllocator::startAllocationThread() {
    if (!allocationThread_.joinable()) {
        allocationThread_ = std::thread(&DistributedTaskAllocator::allocationLoop, this);
    }
}

void DistributedTaskAllocator::stopAllocationThread() {
    cv_.notify_all();
    if (allocationThread_.joinable()) {
        allocationThread_.join();
    }
}

void DistributedTaskAllocator::allocationLoop() {
    while (isRunning_) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        cv_.wait_for(lock, std::chrono::seconds(1), [this] {
            return !pendingAllocations_.empty() || !isRunning_;
        });
        
        while (!pendingAllocations_.empty()) {
            std::string taskId = pendingAllocations_.front();
            pendingAllocations_.pop();
            lock.unlock();
            
            // Collect bids from peers
            collectBids(taskId);
            
            // Wait for bids (in real implementation, use timeout)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Assign task
            assignTask(taskId);
            
            lock.lock();
        }
        
        // Periodic rebalancing check
        lock.unlock();
        checkAndRebalance();
    }
}

std::string DistributedTaskAllocator::generateTaskId() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    return "TASK_" + localUavId_ + "_" + std::to_string(ms);
}

} // namespace nodeagent
