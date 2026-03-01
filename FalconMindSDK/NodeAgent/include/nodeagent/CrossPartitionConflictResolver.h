/**
 * @file CrossPartitionConflictResolver.h
 * @brief Conflict resolution for cross-partition scenarios
 * 
 * Features:
 * - Detect conflicts between partitions (duplicate tasks, waypoint conflicts)
 * - Resolve conflicts using capability-based arbitration
 * - Merge partition states safely
 * - Handle split-brain scenarios
 * - Conflict history tracking
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
#include <chrono>
#include <nlohmann/json.hpp>

namespace nodeagent {

/**
 * @brief Types of conflicts that can occur
 */
enum class ConflictType {
    DUPLICATE_TASK,      // Same task assigned in multiple partitions
    WAYPOINT_COLLISION,  // Multiple UAVs targeting same area
    RESOURCE_CONTENTION, // Competing for limited resources
    STATE_DIVERGENCE,    // Inconsistent state across partitions
    LEADER_DISPUTE,      // Multiple leaders claiming authority
    PRIORITY_INVERSION   // Lower priority task blocking higher priority
};

/**
 * @brief Conflict severity levels
 */
enum class ConflictSeverity {
    LOW,      // Can be resolved automatically
    MEDIUM,   // Requires simple arbitration
    HIGH,     // Requires leader decision
    CRITICAL  // Immediate action required
};

/**
 * @brief Represents a detected conflict
 */
struct Conflict {
    std::string conflictId;
    ConflictType type;
    ConflictSeverity severity;
    std::string description;
    std::vector<std::string> involvedPartitions;
    std::vector<std::string> involvedUavs;
    nlohmann::json conflictingData;
    std::chrono::system_clock::time_point detectedAt;
    std::chrono::system_clock::time_point resolvedAt;
    bool resolved{false};
    std::string resolution;
    std::string winnerPartition;  // Which partition won
};

/**
 * @brief Partition state for conflict detection
 */
struct PartitionState {
    std::string partitionId;
    std::string leaderId;
    std::vector<std::string> memberIds;
    std::vector<std::string> activeTasks;
    std::vector<std::string> claimedWaypoints;
    nlohmann::json sharedResources;
    std::chrono::system_clock::time_point lastUpdated;
    int version{0};  // State version for divergence detection
};

/**
 * @brief Resolution strategy
 */
enum class ResolutionStrategy {
    CAPABILITY_BASED,    // Higher capability wins
    TIMESTAMP_BASED,     // First claimed wins
    PRIORITY_BASED,      // Higher priority wins
    LEADER_ARBITRATION,  // Current leader decides
    CONSENSUS_VOTE,      // Majority vote
    MERGE_COMBINE        // Combine both sides
};

/**
 * @brief Conflict resolution result
 */
struct ResolutionResult {
    std::string conflictId;
    bool resolved{false};
    ResolutionStrategy strategy;
    std::string winnerPartition;
    std::string reason;
    nlohmann::json resolutionData;
    std::vector<std::string> actions;  // Actions to take
};

/**
 * @brief Conflict resolver for cross-partition scenarios
 */
class CrossPartitionConflictResolver {
public:
    using ConflictCallback = std::function<void(const Conflict& conflict)>;
    using ResolutionCallback = std::function<void(const ResolutionResult& result)>;
    using ArbitrationRequest = std::function<bool(const Conflict& conflict, 
                                                   std::string& winner)>;

    CrossPartitionConflictResolver(const std::string& localUavId, 
                                   const std::string& partitionId);
    ~CrossPartitionConflictResolver();

    // Initialization
    bool initialize();
    void shutdown();

    // Callbacks
    void setConflictCallback(ConflictCallback callback);
    void setResolutionCallback(ResolutionCallback callback);
    void setArbitrationRequest(ArbitrationRequest callback);

    /**
     * @brief Register a peer partition for monitoring
     */
    void registerPartition(const PartitionState& partition);

    /**
     * @brief Unregister a partition (when merge completes)
     */
    void unregisterPartition(const std::string& partitionId);

    /**
     * @brief Update local partition state
     */
    void updateLocalState(const PartitionState& state);

    /**
     * @brief Receive state update from peer partition
     */
    void receivePartitionState(const PartitionState& state);

    /**
     * @brief Scan for conflicts between partitions
     */
    std::vector<Conflict> detectConflicts();

    /**
     * @brief Detect specific conflict types
     */
    std::vector<Conflict> detectDuplicateTasks();
    std::vector<Conflict> detectWaypointCollisions();
    std::vector<Conflict> detectResourceContention();
    std::vector<Conflict> detectStateDivergence();
    std::vector<Conflict> detectLeaderDisputes();

    /**
     * @brief Resolve a specific conflict
     */
    ResolutionResult resolveConflict(const std::string& conflictId,
                                     ResolutionStrategy strategy = ResolutionStrategy::CAPABILITY_BASED);

    /**
     * @brief Auto-resolve all detectable conflicts
     */
    std::vector<ResolutionResult> autoResolveConflicts();

    /**
     * @brief Handle split-brain scenario (multiple leaders)
     */
    ResolutionResult resolveSplitBrain(const std::vector<std::string>& leaderIds);

    /**
     * @brief Merge partition states after resolution
     */
    bool mergePartitionStates(const std::string& winningPartitionId);

    /**
     * @brief Check if two partitions can merge without conflicts
     */
    bool canMergeSafely(const std::string& partitionId1, 
                        const std::string& partitionId2) const;

    /**
     * @brief Get pending conflicts
     */
    std::vector<Conflict> getPendingConflicts() const;

    /**
     * @brief Get resolved conflicts
     */
    std::vector<Conflict> getResolvedConflicts() const;

    /**
     * @brief Get conflict by ID
     */
    std::optional<Conflict> getConflict(const std::string& conflictId) const;

    /**
     * @brief Acknowledge and accept resolution
     */
    bool acceptResolution(const std::string& conflictId, 
                          const std::string& partitionId);

    /**
     * @brief Reject resolution and request re-arbitration
     */
    bool rejectResolution(const std::string& conflictId,
                          const std::string& reason);

    /**
     * @brief Compare partition capabilities for arbitration
     */
    int comparePartitionCapability(const std::string& partitionId1,
                                   const std::string& partitionId2) const;

    /**
     * @brief Get conflict statistics
     */
    struct Statistics {
        int totalConflictsDetected{0};
        int totalConflictsResolved{0};
        int totalAutoResolved{0};
        int totalManualArbitration{0};
        int totalSplitBrains{0};
        std::map<ConflictType, int> conflictsByType;
        double averageResolutionTime{0.0};  // milliseconds
    };
    Statistics getStatistics() const;

private:
    void detectDuplicateTasksImpl();
    void detectWaypointCollisionsImpl();
    void detectResourceContentionImpl();
    void detectStateDivergenceImpl();
    void detectLeaderDisputesImpl();
    
    ResolutionResult resolveByCapability(const Conflict& conflict);
    ResolutionResult resolveByTimestamp(const Conflict& conflict);
    ResolutionResult resolveByPriority(const Conflict& conflict);
    ResolutionResult resolveByLeader(const Conflict& conflict);
    ResolutionResult resolveByConsensus(const Conflict& conflict);
    ResolutionResult resolveByMerge(const Conflict& conflict);
    
    void notifyConflict(const Conflict& conflict);
    void notifyResolution(const ResolutionResult& result);
    std::string generateConflictId() const;
    void cleanupOldConflicts();

    std::string localUavId_;
    std::string partitionId_;
    bool isRunning_{false};

    // State tracking
    PartitionState localState_;
    std::map<std::string, PartitionState> peerStates_;

    // Conflict tracking
    std::map<std::string, Conflict> conflicts_;
    std::vector<std::string> pendingConflicts_;
    std::vector<std::string> resolvedConflicts_;

    // Callbacks
    ConflictCallback conflictCallback_;
    ResolutionCallback resolutionCallback_;
    ArbitrationRequest arbitrationCallback_;

    // Threading
    mutable std::mutex mutex_;
    std::thread detectionThread_;

    // Statistics
    Statistics stats_;
};

} // namespace nodeagent
