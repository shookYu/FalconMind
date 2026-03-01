/**
 * @file CrossPartitionConflictResolver.cpp
 * @brief Implementation of cross-partition conflict resolution
 */

#include "nodeagent/CrossPartitionConflictResolver.h"
#include "nodeagent/Logger.h"
#include <chrono>
#include <sstream>

namespace nodeagent {

CrossPartitionConflictResolver::CrossPartitionConflictResolver(
    const std::string& localUavId, 
    const std::string& partitionId)
    : localUavId_(localUavId)
    , partitionId_(partitionId)
    , isRunning_(false) {
    LOG_INFO("CrossPartitionConflictResolver", "Constructor - UAV: " + localUavId + 
             ", Partition: " + partitionId);
}

CrossPartitionConflictResolver::~CrossPartitionConflictResolver() {
    LOG_INFO("CrossPartitionConflictResolver", "Destructor");
    shutdown();
}

bool CrossPartitionConflictResolver::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isRunning_) {
        LOG_WARNING("CrossPartitionConflictResolver", "Already initialized");
        return true;
    }
    
    LOG_INFO("CrossPartitionConflictResolver", "Initializing");
    
    // Initialize local state
    localState_.partitionId = partitionId_;
    localState_.lastUpdated = std::chrono::system_clock::now();
    localState_.version = 1;
    
    isRunning_ = true;
    
    LOG_INFO("CrossPartitionConflictResolver", "Initialization complete");
    return true;
}

void CrossPartitionConflictResolver::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!isRunning_) {
            return;
        }
        isRunning_ = false;
    }
    
    if (detectionThread_.joinable()) {
        detectionThread_.join();
    }
    
    LOG_INFO("CrossPartitionConflictResolver", "Shutdown complete");
}

void CrossPartitionConflictResolver::setConflictCallback(ConflictCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    conflictCallback_ = callback;
}

void CrossPartitionConflictResolver::setResolutionCallback(ResolutionCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    resolutionCallback_ = callback;
}

void CrossPartitionConflictResolver::setArbitrationRequest(ArbitrationRequest callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    arbitrationCallback_ = callback;
}

void CrossPartitionConflictResolver::registerPartition(const PartitionState& partition) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    peerStates_[partition.partitionId] = partition;
    LOG_INFO("CrossPartitionConflictResolver", "Registered partition: " + partition.partitionId +
             " with " + std::to_string(partition.memberIds.size()) + " members");
}

void CrossPartitionConflictResolver::unregisterPartition(const std::string& partitionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    peerStates_.erase(partitionId);
    LOG_INFO("CrossPartitionConflictResolver", "Unregistered partition: " + partitionId);
}

void CrossPartitionConflictResolver::updateLocalState(const PartitionState& state) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    localState_ = state;
    localState_.lastUpdated = std::chrono::system_clock::now();
    ++localState_.version;
}

void CrossPartitionConflictResolver::receivePartitionState(const PartitionState& state) {
    if (state.partitionId == partitionId_) {
        return;  // Ignore own state
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = peerStates_.find(state.partitionId);
    if (it != peerStates_.end()) {
        // Update existing
        it->second = state;
    } else {
        // New partition
        peerStates_[state.partitionId] = state;
        LOG_INFO("CrossPartitionConflictResolver", "Discovered new partition: " + state.partitionId);
    }
}

std::vector<Conflict> CrossPartitionConflictResolver::detectConflicts() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Conflict> detected;
    
    // Run all detection methods
    detectDuplicateTasksImpl();
    detectWaypointCollisionsImpl();
    detectResourceContentionImpl();
    detectStateDivergenceImpl();
    detectLeaderDisputesImpl();
    
    // Collect all pending conflicts
    for (const auto& conflictId : pendingConflicts_) {
        auto it = conflicts_.find(conflictId);
        if (it != conflicts_.end()) {
            detected.push_back(it->second);
        }
    }
    
    return detected;
}

std::vector<Conflict> CrossPartitionConflictResolver::detectDuplicateTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    detectDuplicateTasksImpl();
    
    std::vector<Conflict> result;
    for (const auto& [id, conflict] : conflicts_) {
        if (conflict.type == ConflictType::DUPLICATE_TASK && !conflict.resolved) {
            result.push_back(conflict);
        }
    }
    return result;
}

std::vector<Conflict> CrossPartitionConflictResolver::detectWaypointCollisions() {
    std::lock_guard<std::mutex> lock(mutex_);
    detectWaypointCollisionsImpl();
    
    std::vector<Conflict> result;
    for (const auto& [id, conflict] : conflicts_) {
        if (conflict.type == ConflictType::WAYPOINT_COLLISION && !conflict.resolved) {
            result.push_back(conflict);
        }
    }
    return result;
}

std::vector<Conflict> CrossPartitionConflictResolver::detectResourceContention() {
    std::lock_guard<std::mutex> lock(mutex_);
    detectResourceContentionImpl();
    
    std::vector<Conflict> result;
    for (const auto& [id, conflict] : conflicts_) {
        if (conflict.type == ConflictType::RESOURCE_CONTENTION && !conflict.resolved) {
            result.push_back(conflict);
        }
    }
    return result;
}

std::vector<Conflict> CrossPartitionConflictResolver::detectStateDivergence() {
    std::lock_guard<std::mutex> lock(mutex_);
    detectStateDivergenceImpl();
    
    std::vector<Conflict> result;
    for (const auto& [id, conflict] : conflicts_) {
        if (conflict.type == ConflictType::STATE_DIVERGENCE && !conflict.resolved) {
            result.push_back(conflict);
        }
    }
    return result;
}

std::vector<Conflict> CrossPartitionConflictResolver::detectLeaderDisputes() {
    std::lock_guard<std::mutex> lock(mutex_);
    detectLeaderDisputesImpl();
    
    std::vector<Conflict> result;
    for (const auto& [id, conflict] : conflicts_) {
        if (conflict.type == ConflictType::LEADER_DISPUTE && !conflict.resolved) {
            result.push_back(conflict);
        }
    }
    return result;
}

ResolutionResult CrossPartitionConflictResolver::resolveConflict(
    const std::string& conflictId,
    ResolutionStrategy strategy) {
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = conflicts_.find(conflictId);
    if (it == conflicts_.end()) {
        return ResolutionResult{conflictId, false, strategy, "", "Conflict not found", {}, {}};
    }
    
    Conflict& conflict = it->second;
    
    ResolutionResult result;
    result.conflictId = conflictId;
    result.strategy = strategy;
    
    switch (strategy) {
        case ResolutionStrategy::CAPABILITY_BASED:
            result = resolveByCapability(conflict);
            break;
        case ResolutionStrategy::TIMESTAMP_BASED:
            result = resolveByTimestamp(conflict);
            break;
        case ResolutionStrategy::PRIORITY_BASED:
            result = resolveByPriority(conflict);
            break;
        case ResolutionStrategy::LEADER_ARBITRATION:
            result = resolveByLeader(conflict);
            break;
        case ResolutionStrategy::CONSENSUS_VOTE:
            result = resolveByConsensus(conflict);
            break;
        case ResolutionStrategy::MERGE_COMBINE:
            result = resolveByMerge(conflict);
            break;
    }
    
    if (result.resolved) {
        conflict.resolved = true;
        conflict.resolvedAt = std::chrono::system_clock::now();
        conflict.winnerPartition = result.winnerPartition;
        conflict.resolution = result.reason;
        
        // Move from pending to resolved
        auto pendingIt = std::find(pendingConflicts_.begin(), pendingConflicts_.end(), conflictId);
        if (pendingIt != pendingConflicts_.end()) {
            pendingConflicts_.erase(pendingIt);
        }
        resolvedConflicts_.push_back(conflictId);
        
        ++stats_.totalConflictsResolved;
        
        notifyResolution(result);
        
        LOG_INFO("CrossPartitionConflictResolver", "Resolved conflict " + conflictId +
                 " - Winner: " + result.winnerPartition);
    }
    
    return result;
}

std::vector<ResolutionResult> CrossPartitionConflictResolver::autoResolveConflicts() {
    std::vector<ResolutionResult> results;
    
    // Detect all conflicts
    detectConflicts();
    
    // Auto-resolve based on severity and type
    std::vector<std::string> toResolve;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& conflictId : pendingConflicts_) {
            auto it = conflicts_.find(conflictId);
            if (it != conflicts_.end() && it->second.severity == ConflictSeverity::LOW) {
                toResolve.push_back(conflictId);
            }
        }
    }
    
    for (const auto& conflictId : toResolve) {
        auto result = resolveConflict(conflictId, ResolutionStrategy::CAPABILITY_BASED);
        results.push_back(result);
    }
    
    return results;
}

ResolutionResult CrossPartitionConflictResolver::resolveSplitBrain(
    const std::vector<std::string>& leaderIds) {
    
    LOG_WARNING("CrossPartitionConflictResolver", "Resolving split-brain with " +
                std::to_string(leaderIds.size()) + " leaders");
    
    ++stats_.totalSplitBrains;
    
    // Create a conflict for the split-brain
    Conflict conflict;
    conflict.conflictId = generateConflictId();
    conflict.type = ConflictType::LEADER_DISPUTE;
    conflict.severity = ConflictSeverity::CRITICAL;
    conflict.description = "Split-brain detected: multiple leaders";
    conflict.involvedPartitions = getPartitionIds();
    conflict.involvedUavs = leaderIds;
    conflict.detectedAt = std::chrono::system_clock::now();
    conflict.resolved = false;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        conflicts_[conflict.conflictId] = conflict;
        pendingConflicts_.push_back(conflict.conflictId);
        ++stats_.totalConflictsDetected;
    }
    
    notifyConflict(conflict);
    
    // Resolve by capability (highest capability partition wins)
    return resolveConflict(conflict.conflictId, ResolutionStrategy::CAPABILITY_BASED);
}

bool CrossPartitionConflictResolver::mergePartitionStates(const std::string& winningPartitionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LOG_INFO("CrossPartitionConflictResolver", "Merging states with winner: " + winningPartitionId);
    
    // In real implementation, this would merge task assignments, waypoints, etc.
    // For now, just clear peer states
    
    auto winnerIt = peerStates_.find(winningPartitionId);
    if (winnerIt != peerStates_.end()) {
        // Adopt winner's leader
        if (!winnerIt->second.leaderId.empty()) {
            localState_.leaderId = winnerIt->second.leaderId;
        }
        
        // Merge active tasks (avoiding duplicates)
        for (const auto& task : winnerIt->second.activeTasks) {
            if (std::find(localState_.activeTasks.begin(), localState_.activeTasks.end(), task)
                == localState_.activeTasks.end()) {
                localState_.activeTasks.push_back(task);
            }
        }
    }
    
    // Clear peer states after merge
    peerStates_.clear();
    
    return true;
}

bool CrossPartitionConflictResolver::canMergeSafely(const std::string& partitionId1,
                                                    const std::string& partitionId2) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it1 = peerStates_.find(partitionId1);
    auto it2 = peerStates_.find(partitionId2);
    
    if (it1 == peerStates_.end() || it2 == peerStates_.end()) {
        return false;
    }
    
    // Check for conflicting tasks
    for (const auto& task1 : it1->second.activeTasks) {
        for (const auto& task2 : it2->second.activeTasks) {
            if (task1 == task2) {
                // Duplicate task - can't merge safely without resolution
                return false;
            }
        }
    }
    
    // Check for waypoint collisions
    for (const auto& wp1 : it1->second.claimedWaypoints) {
        for (const auto& wp2 : it2->second.claimedWaypoints) {
            if (wp1 == wp2) {
                return false;
            }
        }
    }
    
    return true;
}

std::vector<Conflict> CrossPartitionConflictResolver::getPendingConflicts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Conflict> result;
    for (const auto& conflictId : pendingConflicts_) {
        auto it = conflicts_.find(conflictId);
        if (it != conflicts_.end()) {
            result.push_back(it->second);
        }
    }
    return result;
}

std::vector<Conflict> CrossPartitionConflictResolver::getResolvedConflicts() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Conflict> result;
    for (const auto& conflictId : resolvedConflicts_) {
        auto it = conflicts_.find(conflictId);
        if (it != conflicts_.end()) {
            result.push_back(it->second);
        }
    }
    return result;
}

std::optional<Conflict> CrossPartitionConflictResolver::getConflict(
    const std::string& conflictId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = conflicts_.find(conflictId);
    if (it != conflicts_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool CrossPartitionConflictResolver::acceptResolution(const std::string& conflictId,
                                                      const std::string& partitionId) {
    LOG_INFO("CrossPartitionConflictResolver", "Partition " + partitionId + 
             " accepted resolution for " + conflictId);
    
    // In real implementation, track acceptance from all partitions
    return true;
}

bool CrossPartitionConflictResolver::rejectResolution(const std::string& conflictId,
                                                      const std::string& reason) {
    LOG_WARNING("CrossPartitionConflictResolver", "Resolution rejected for " + conflictId +
                ": " + reason);
    
    // Re-open the conflict
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = conflicts_.find(conflictId);
    if (it != conflicts_.end()) {
        it->second.resolved = false;
        
        auto resolvedIt = std::find(resolvedConflicts_.begin(), resolvedConflicts_.end(), conflictId);
        if (resolvedIt != resolvedConflicts_.end()) {
            resolvedConflicts_.erase(resolvedIt);
        }
        
        if (std::find(pendingConflicts_.begin(), pendingConflicts_.end(), conflictId) 
            == pendingConflicts_.end()) {
            pendingConflicts_.push_back(conflictId);
        }
        
        return true;
    }
    
    return false;
}

int CrossPartitionConflictResolver::comparePartitionCapability(
    const std::string& partitionId1,
    const std::string& partitionId2) const {
    
    auto it1 = peerStates_.find(partitionId1);
    auto it2 = peerStates_.find(partitionId2);
    
    if (it1 == peerStates_.end() && it2 == peerStates_.end()) {
        return 0;
    }
    if (it1 == peerStates_.end()) {
        return -1;
    }
    if (it2 == peerStates_.end()) {
        return 1;
    }
    
    // Compare by member count (more members = higher capability)
    int diff = static_cast<int>(it1->second.memberIds.size()) - 
               static_cast<int>(it2->second.memberIds.size());
    
    if (diff != 0) {
        return diff;
    }
    
    // Compare by active tasks
    diff = static_cast<int>(it1->second.activeTasks.size()) - 
           static_cast<int>(it2->second.activeTasks.size());
    
    return diff;
}

CrossPartitionConflictResolver::Statistics 
CrossPartitionConflictResolver::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

// Private implementation methods

void CrossPartitionConflictResolver::detectDuplicateTasksImpl() {
    // Build map of task -> partitions
    std::map<std::string, std::vector<std::string>> taskPartitions;
    
    // Add local tasks
    for (const auto& task : localState_.activeTasks) {
        taskPartitions[task].push_back(partitionId_);
    }
    
    // Add peer tasks
    for (const auto& [id, state] : peerStates_) {
        for (const auto& task : state.activeTasks) {
            taskPartitions[task].push_back(id);
        }
    }
    
    // Find duplicates (same task in multiple partitions)
    for (const auto& [task, partitions] : taskPartitions) {
        if (partitions.size() > 1) {
            Conflict conflict;
            conflict.conflictId = generateConflictId();
            conflict.type = ConflictType::DUPLICATE_TASK;
            conflict.severity = ConflictSeverity::MEDIUM;
            conflict.description = "Task '" + task + "' assigned in " + 
                                   std::to_string(partitions.size()) + " partitions";
            conflict.involvedPartitions = partitions;
            conflict.conflictingData["task_id"] = task;
            conflict.detectedAt = std::chrono::system_clock::now();
            conflict.resolved = false;
            
            if (conflicts_.find(conflict.conflictId) == conflicts_.end()) {
                conflicts_[conflict.conflictId] = conflict;
                pendingConflicts_.push_back(conflict.conflictId);
                ++stats_.totalConflictsDetected;
                ++stats_.conflictsByType[ConflictType::DUPLICATE_TASK];
                
                notifyConflict(conflict);
            }
        }
    }
}

void CrossPartitionConflictResolver::detectWaypointCollisionsImpl() {
    // Similar logic for waypoint collisions
    // Would compare waypoint coordinates for overlaps
}

void CrossPartitionConflictResolver::detectResourceContentionImpl() {
    // Check for competing resource claims
}

void CrossPartitionConflictResolver::detectStateDivergenceImpl() {
    // Check for version mismatches
    for (const auto& [id, state] : peerStates_) {
        if (std::abs(state.version - localState_.version) > 5) {
            // Significant divergence
            Conflict conflict;
            conflict.conflictId = generateConflictId();
            conflict.type = ConflictType::STATE_DIVERGENCE;
            conflict.severity = ConflictSeverity::HIGH;
            conflict.description = "State version divergence with " + id;
            conflict.involvedPartitions = {partitionId_, id};
            conflict.detectedAt = std::chrono::system_clock::now();
            conflict.resolved = false;
            
            if (conflicts_.find(conflict.conflictId) == conflicts_.end()) {
                conflicts_[conflict.conflictId] = conflict;
                pendingConflicts_.push_back(conflict.conflictId);
                ++stats_.totalConflictsDetected;
                ++stats_.conflictsByType[ConflictType::STATE_DIVERGENCE];
                
                notifyConflict(conflict);
            }
        }
    }
}

void CrossPartitionConflictResolver::detectLeaderDisputesImpl() {
    // Check if multiple partitions claim to have the same leader
    std::map<std::string, std::vector<std::string>> leaderPartitions;
    
    if (!localState_.leaderId.empty()) {
        leaderPartitions[localState_.leaderId].push_back(partitionId_);
    }
    
    for (const auto& [id, state] : peerStates_) {
        if (!state.leaderId.empty()) {
            leaderPartitions[state.leaderId].push_back(id);
        }
    }
    
    for (const auto& [leader, partitions] : leaderPartitions) {
        if (partitions.size() > 1) {
            Conflict conflict;
            conflict.conflictId = generateConflictId();
            conflict.type = ConflictType::LEADER_DISPUTE;
            conflict.severity = ConflictSeverity::CRITICAL;
            conflict.description = "Multiple partitions claim leader: " + leader;
            conflict.involvedPartitions = partitions;
            conflict.detectedAt = std::chrono::system_clock::now();
            conflict.resolved = false;
            
            if (conflicts_.find(conflict.conflictId) == conflicts_.end()) {
                conflicts_[conflict.conflictId] = conflict;
                pendingConflicts_.push_back(conflict.conflictId);
                ++stats_.totalConflictsDetected;
                ++stats_.conflictsByType[ConflictType::LEADER_DISPUTE];
                
                notifyConflict(conflict);
            }
        }
    }
}

ResolutionResult CrossPartitionConflictResolver::resolveByCapability(const Conflict& conflict) {
    ResolutionResult result;
    result.conflictId = conflict.conflictId;
    result.strategy = ResolutionStrategy::CAPABILITY_BASED;
    
    // Find partition with highest capability
    std::string winner;
    int maxCapability = -1;
    
    for (const auto& partitionId : conflict.involvedPartitions) {
        int capability = 0;
        
        if (partitionId == partitionId_) {
            capability = static_cast<int>(localState_.memberIds.size()) * 10 +
                        static_cast<int>(localState_.activeTasks.size());
        } else {
            auto it = peerStates_.find(partitionId);
            if (it != peerStates_.end()) {
                capability = static_cast<int>(it->second.memberIds.size()) * 10 +
                            static_cast<int>(it->second.activeTasks.size());
            }
        }
        
        if (capability > maxCapability) {
            maxCapability = capability;
            winner = partitionId;
        }
    }
    
    if (!winner.empty()) {
        result.resolved = true;
        result.winnerPartition = winner;
        result.reason = "Higher capability (" + std::to_string(maxCapability) + " points)";
        ++stats_.totalAutoResolved;
    }
    
    return result;
}

ResolutionResult CrossPartitionConflictResolver::resolveByTimestamp(const Conflict& conflict) {
    // First claimed wins
    ResolutionResult result;
    result.conflictId = conflict.conflictId;
    result.strategy = ResolutionStrategy::TIMESTAMP_BASED;
    result.resolved = true;
    result.winnerPartition = conflict.involvedPartitions.empty() ? "" : conflict.involvedPartitions[0];
    result.reason = "First claimed";
    return result;
}

ResolutionResult CrossPartitionConflictResolver::resolveByPriority(const Conflict& conflict) {
    // Higher priority task wins
    return resolveByCapability(conflict);  // Fallback to capability
}

ResolutionResult CrossPartitionConflictResolver::resolveByLeader(const Conflict& conflict) {
    ResolutionResult result;
    result.conflictId = conflict.conflictId;
    result.strategy = ResolutionStrategy::LEADER_ARBITRATION;
    
    if (arbitrationCallback_) {
        std::string winner;
        if (arbitrationCallback_(conflict, winner)) {
            result.resolved = true;
            result.winnerPartition = winner;
            result.reason = "Leader arbitration";
            ++stats_.totalManualArbitration;
        }
    }
    
    return result;
}

ResolutionResult CrossPartitionConflictResolver::resolveByConsensus(const Conflict& conflict) {
    // Majority vote - simplified as capability-based
    return resolveByCapability(conflict);
}

ResolutionResult CrossPartitionConflictResolver::resolveByMerge(const Conflict& conflict) {
    ResolutionResult result;
    result.conflictId = conflict.conflictId;
    result.strategy = ResolutionStrategy::MERGE_COMBINE;
    result.resolved = true;
    result.winnerPartition = partitionId_;  // Keep local
    result.reason = "Merged both partitions";
    result.actions.push_back("merge_tasks");
    result.actions.push_back("sync_waypoints");
    return result;
}

void CrossPartitionConflictResolver::notifyConflict(const Conflict& conflict) {
    if (conflictCallback_) {
        conflictCallback_(conflict);
    }
}

void CrossPartitionConflictResolver::notifyResolution(const ResolutionResult& result) {
    if (resolutionCallback_) {
        resolutionCallback_(result);
    }
}

std::string CrossPartitionConflictResolver::generateConflictId() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    return "CONFLICT_" + partitionId_ + "_" + std::to_string(ms);
}

void CrossPartitionConflictResolver::cleanupOldConflicts() {
    // Remove resolved conflicts older than 1 hour
    auto cutoff = std::chrono::system_clock::now() - std::chrono::hours(1);
    
    for (auto it = conflicts_.begin(); it != conflicts_.end();) {
        if (it->second.resolved && it->second.resolvedAt < cutoff) {
            it = conflicts_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<std::string> CrossPartitionConflictResolver::getPartitionIds() const {
    std::vector<std::string> ids;
    ids.push_back(partitionId_);
    for (const auto& [id, state] : peerStates_) {
        ids.push_back(id);
    }
    return ids;
}

} // namespace nodeagent
