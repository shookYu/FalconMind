/**
 * @file DistributedTaskAllocator.h
 * @brief Distributed task allocation for UAV swarm in offline mode
 * 
 * Features:
 * - Consensus-based task bidding (auction algorithm)
 * - Capability-aware task assignment
 * - Dynamic task redistribution on partition changes
 * - Conflict detection and resolution
 * - Load balancing across swarm members
 * 
 * @note P2 Enhancement - Zero mocks, production-ready
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <mutex>
#include <nlohmann/json.hpp>
#include <chrono>

namespace nodeagent {

/**
 * @brief Task types supported for distributed allocation
 */
enum class DistributedTaskType {
    SEARCH,         // Area search
    PATROL,         // Perimeter patrol
    MONITOR,        // Point monitoring
    TRACK,          // Target tracking
    TRANSPORT,      // Payload transport
    INSPECT         // Infrastructure inspection
};

/**
 * @brief Task requirements for capability matching
 */
struct TaskRequirements {
    double minBatteryLevel{30.0};
    double minComputePower{1.0};
    double maxRange{1000.0};
    double minAltitude{10.0};
    double maxAltitude{120.0};
    std::vector<std::string> requiredSensors;
    std::vector<std::string> requiredCapabilities;
    double estimatedDuration{300.0};  // seconds
    double priority{1.0};  // 1.0 = normal, >1.0 = high
};

/**
 * @brief Distributed task definition
 */
struct DistributedTask {
    std::string taskId;
    DistributedTaskType type;
    std::string name;
    std::string description;
    TaskRequirements requirements;
    nlohmann::json payload;  // Task-specific data (waypoints, targets, etc.)
    std::string assignedUavId;  // Empty if unassigned
    std::string assignedPartitionId;  // For partition-aware allocation
    TaskStatus status{TaskStatus::PENDING};
    std::chrono::system_clock::time_point createdAt;
    std::chrono::system_clock::time_point assignedAt;
    int progress{0};  // 0-100
    nlohmann::json result;  // Task execution result
};

/**
 * @brief UAV capability for bidding
 */
struct UavCapability {
    std::string uavId;
    double batteryLevel;
    double computePower;
    bool hasNPU{false};
    std::vector<std::string> sensors;
    std::vector<std::string> currentTasks;
    double currentLoad{0.0};  // 0.0-1.0
    double signalStrength{100.0};
    double maxFlightTime{30.0};  // minutes remaining
    nlohmann::json position;
};

/**
 * @brief Bid from a UAV for a task
 */
struct TaskBid {
    std::string taskId;
    std::string uavId;
    double bidScore{0.0};  // Higher = better
    double estimatedCompletionTime{0.0};  // seconds
    double confidence{0.0};  // 0.0-1.0
    std::chrono::system_clock::time_point bidTime;
    bool canExecute{false};
    std::string reason;  // If cannot execute
};

/**
 * @brief Task allocation result
 */
struct AllocationResult {
    std::string taskId;
    std::string assignedUavId;
    bool success{false};
    std::string reason;
    double bidScore{0.0};
    std::vector<TaskBid> allBids;
};

/**
 * @brief Load balancing metrics
 */
struct LoadBalanceMetrics {
    double averageLoad{0.0};
    double maxLoad{0.0};
    double minLoad{0.0};
    double loadVariance{0.0};
    int overloadedUavs{0};
    int underloadedUavs{0};
};

/**
 * @brief Distributed task allocator using auction algorithm
 */
class DistributedTaskAllocator {
public:
    using AllocationCallback = std::function<void(const AllocationResult&)>;
    using TaskCompleteCallback = std::function<void(const std::string& taskId, 
                                                       const std::string& uavId,
                                                       const nlohmann::json& result)>;
    using RebalanceCallback = std::function<void(const std::vector<std::string>& reassignedTasks)>;

    explicit DistributedTaskAllocator(const std::string& localUavId);
    ~DistributedTaskAllocator();

    // Delete copy/move
    DistributedTaskAllocator(const DistributedTaskAllocator&) = delete;
    DistributedTaskAllocator& operator=(const DistributedTaskAllocator&) = delete;

    /**
     * @brief Initialize the allocator
     * @param isLeader Whether this UAV is the swarm leader
     * @return true if initialization successful
     */
    bool initialize(bool isLeader = false);
    void shutdown();

    /**
     * @brief Set callbacks
     */
    void setAllocationCallback(AllocationCallback callback);
    void setTaskCompleteCallback(TaskCompleteCallback callback);
    void setRebalanceCallback(RebalanceCallback callback);

    /**
     * @brief Submit a new task for distributed allocation
     * @param task The task to allocate
     * @return Task ID if successful
     */
    std::string submitTask(const DistributedTask& task);

    /**
     * @brief Submit multiple tasks
     */
    std::vector<std::string> submitTasks(const std::vector<DistributedTask>& tasks);

    /**
     * @brief Update local UAV capability (call periodically)
     */
    void updateLocalCapability(const UavCapability& capability);

    /**
     * @brief Update peer UAV capability
     */
    void updatePeerCapability(const UavCapability& capability);

    /**
     * @brief Remove peer (when disconnected)
     */
    void removePeer(const std::string& uavId);

    /**
     * @brief Initiate bidding for unassigned tasks
     * Leader calls this to trigger allocation
     */
    void initiateBidding();

    /**
     * @brief Submit a bid for a task (called by non-leader UAVs)
     */
    TaskBid submitBid(const std::string& taskId, const TaskRequirements& requirements);

    /**
     * @brief Receive bids from peers (called by leader)
     */
    void receiveBid(const TaskBid& bid);

    /**
     * @brief Assign task to winner (called by leader)
     */
    AllocationResult assignTask(const std::string& taskId);

    /**
     * @brief Receive task assignment (called by non-leader UAVs)
     */
    void receiveAssignment(const std::string& taskId, const std::string& assignedUavId);

    /**
     * @brief Update task progress
     */
    void updateTaskProgress(const std::string& taskId, int progress);

    /**
     * @brief Complete a task
     */
    void completeTask(const std::string& taskId, const nlohmann::json& result);

    /**
     * @brief Cancel a task
     */
    bool cancelTask(const std::string& taskId);

    /**
     * @brief Handle partition - redistribute tasks in partition
     */
    void handlePartition(const std::vector<std::string>& partitionMembers);

    /**
     * @brief Handle partition merge - reconcile task assignments
     */
    void handlePartitionMerge(const std::vector<std::string>& mergedMembers);

    /**
     * @brief Trigger load rebalancing
     */
    void rebalanceLoad();

    /**
     * @brief Calculate bid score for a task
     */
    double calculateBidScore(const TaskRequirements& requirements,
                             const UavCapability& capability) const;

    /**
     * @brief Check if UAV can execute task
     */
    bool canExecuteTask(const TaskRequirements& requirements,
                        const UavCapability& capability) const;

    /**
     * @brief Get task info
     */
    std::optional<DistributedTask> getTask(const std::string& taskId) const;

    /**
     * @brief Get all tasks
     */
    std::vector<DistributedTask> getAllTasks() const;

    /**
     * @brief Get tasks assigned to specific UAV
     */
    std::vector<DistributedTask> getTasksForUav(const std::string& uavId) const;

    /**
     * @brief Get unassigned tasks
     */
    std::vector<DistributedTask> getUnassignedTasks() const;

    /**
     * @brief Get load balance metrics
     */
    LoadBalanceMetrics getLoadBalanceMetrics() const;

    /**
     * @brief Check if this UAV is the leader
     */
    bool isLeader() const;

    /**
     * @brief Set leader status
     */
    void setLeaderStatus(bool isLeader);

    /**
     * @brief Get allocation statistics
     */
    struct Statistics {
        int totalTasksSubmitted{0};
        int totalTasksAssigned{0};
        int totalTasksCompleted{0};
        int totalTasksCancelled{0};
        int totalBidsReceived{0};
        int totalRebalances{0};
        double averageAllocationTime{0.0};  // milliseconds
    };
    Statistics getStatistics() const;

private:
    void collectBids(const std::string& taskId);
    void selectWinner(const std::string& taskId);
    void redistributePartitionTasks();
    void reconcileMergedTasks();
    void checkAndRebalance();
    double calculateUavLoad(const std::string& uavId) const;
    void notifyAssignment(const AllocationResult& result);
    void startAllocationThread();
    void stopAllocationThread();
    void allocationLoop();

    std::string localUavId_;
    bool isLeader_{false};
    bool isRunning_{false};

    // Task storage
    std::map<std::string, DistributedTask> tasks_;
    std::map<std::string, std::vector<TaskBid>> bids_;  // taskId -> bids

    // Capability storage
    UavCapability localCapability_;
    std::map<std::string, UavCapability&gt; peerCapabilities_;

    // Callbacks
    AllocationCallback allocationCallback_;
    TaskCompleteCallback taskCompleteCallback_;
    RebalanceCallback rebalanceCallback_;

    // Threading
    std::thread allocationThread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::string> pendingAllocations_;

    // Statistics
    Statistics stats_;
    std::chrono::steady_clock::time_point startTime_;
};

} // namespace nodeagent
