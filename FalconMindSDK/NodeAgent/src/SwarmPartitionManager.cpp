/**
 * SwarmPartitionManager.cpp - Production-grade cluster partition management
 */

#include "nodeagent/SwarmPartitionManager.h"
#include "nodeagent/Logger.h"
#include <chrono>
#include <iomanip>
#include <math>

namespace nodeagent {

SwarmPartitionManager::SwarmPartitionManager(const std::string& uavId, const std::string& swarmId)
    : uavId_(uavId)
    , swarmId_(swarmId)
    , myPartitionId_("")
    , connectivityState_(SwarmConnectivity::ISOLATED)
    , electionInProgress_(false)
    , isRunning_(false)
    , heartbeatRunning_(false) {
    LOG_INFO("SwarmPartitionManager", "Constructor - UAV: " + uavId + ", Swarm: " + swarmId);
}

SwarmPartitionManager::~SwarmPartitionManager() {
    LOG_INFO("SwarmPartitionManager", "Destructor");
    shutdown();
}

bool SwarmPartitionManager::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isRunning_) {
        LOG_WARNING("SwarmPartitionManager", "Already initialized");
        return true;
    }
    
    LOG_INFO("SwarmPartitionManager", "Initializing");
    
    SwarmMember self;
    self.uavId = uavId_;
    self.isLeader = false;
    self.partitionId = "";
    self.isActive = true;
    self.lastSeenSeconds = 0;
    self.capabilities.uavId = uavId_;
    self.capabilities.batteryLevel = 100;
    self.capabilities.computePower = 6;
    self.capabilities.hasNPU = true;
    self.capabilities.maxFlightTime = 30.0;
    self.capabilities.signalStrength = 100.0;
    members_[uavId_] = self;
    
    isRunning_ = true;
    heartbeatRunning_ = true;
    heartbeatThread_ = std::thread(&SwarmPartitionManager::checkMemberTimeouts, this);
    
    LOG_INFO("SwarmPartitionManager", "Initialization complete");
    return true;
}

void SwarmPartitionManager::shutdown() {
    LOG_INFO("SwarmPartitionManager", "Shutting down");
    isRunning_ = false;
    heartbeatRunning_ = false;
    if (heartbeatThread_.joinable()) {
        heartbeatThread_.join();
    }
    LOG_INFO("SwarmPartitionManager", "Shutdown complete");
}

void SwarmPartitionManager::setPartitionChangeCallback(PartitionChangeCallback callback) {
    partitionChangeCallback_ = callback;
}

void SwarmPartitionManager::setLeaderChangeCallback(LeaderChangeCallback callback) {
    leaderChangeCallback_ = callback;
}

void SwarmPartitionManager::setMergeCallback(MergeCallback callback) {
    mergeCallback_ = callback;
}

void SwarmPartitionManager::addMember(const SwarmMember& member) {
    std::lock_guard<std::mutex> lock(mutex_);
    members_[member.uavId] = member;
    LOG_INFO("SwarmPartitionManager", "Added member: " + member.uavId);
    detectPartitions();
}

void SwarmPartitionManager::removeMember(const std::string& uavId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = members_.find(uavId);
    if (it == members_.end()) return;
    
    bool wasLeader = it->second.isLeader;
    std::string partitionId = it->second.partitionId;
    
    members_.erase(it);
    LOG_INFO("SwarmPartitionManager", "Removed member: " + uavId);
    
    if (wasLeader && !partitionId.empty()) {
        auto partition = findPartition(partitionId);
        if (partition) {
            partition->leaderId.clear();
            lock.unlock();
            triggerLeaderElection(LeaderElectionReason::LEADER_FAILED);
            return;
        }
    }
    
    detectPartitions();
}

void SwarmPartitionManager::updateMember(const std::string& uavId, const SwarmMember& member) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = members_.find(uavId);
    if (it != members_.end()) {
        std::string oldPartitionId = it->second.partitionId;
        it->second = member;
        it->second.uavId = uavId;
        if (member.partitionId.empty()) {
            it->second.partitionId = oldPartitionId;
        }
    }
}

void SwarmPartitionManager::onMemberHeartbeat(const std::string& uavId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = members_.find(uavId);
    if (it != members_.end()) {
        it->second.lastSeenSeconds = 0;
        it->second.isActive = true;
    }
}

SwarmMember* SwarmPartitionManager::findMember(const std::string& uavId) {
    auto it = members_.find(uavId);
    return (it != members_.end()) ? &(it->second) : nullptr;
}

const SwarmMember* SwarmPartitionManager::findMember(const std::string& uavId) const {
    auto it = members_.find(uavId);
    return (it != members_.end()) ? &(it->second) : nullptr;
}

Partition* SwarmPartitionManager::findPartition(const std::string& partitionId) {
    auto it = partitions_.find(partitionId);
    return (it != partitions_.end()) ? &(it->second) : nullptr;
}

const Partition* SwarmPartitionManager::findPartition(const std::string& partitionId) const {
    auto it = partitions_.find(partitionId);
    return (it != partitions_.end()) ? &(it->second) : nullptr;
}

double SwarmPartitionManager::calculateLeadershipScore(const UavCapability& capabilities) const {
    double score = 0.0;
    score += capabilities.batteryLevel * 0.40;
    score += std::min(capabilities.computePower / 10.0, 20.0);
    if (capabilities.hasNPU) score += 10.0;
    score += capabilities.signalStrength * 0.20;
    score += std::min(capabilities.maxFlightTime / 10.0, 10.0);
    return score;
}

void SwarmPartitionManager::triggerLeaderElection(LeaderElectionReason reason) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (electionInProgress_) {
        LOG_WARNING("SwarmPartitionManager", "Election already in progress");
        return;
    }
    
    LOG_INFO("SwarmPartitionManager", "Triggering leader election");
    
    electionInProgress_ = true;
    votes_.clear();
    proposals_.clear();
    
    std::vector<std::string> candidates;
    std::string myPartition = myPartitionId_;
    
    for (const auto& pair : members_) {
        if (pair.second.isActive && 
            (myPartition.empty() || pair.second.partitionId == myPartition)) {
            candidates.push_back(pair.first);
        }
    }
    
    if (candidates.empty()) {
        LOG_ERROR("SwarmPartitionManager", "No active members for election");
        electionInProgress_ = false;
        return;
    }
    
    // Propose self
    auto it = members_.find(uavId_);
    if (it != members_.end()) {
        LeaderProposal proposal;
        proposal.candidateId = uavId_;
        proposal.capabilities = it->second.capabilities;
        proposal.score = calculateLeadershipScore(it->second.capabilities);
        proposeLeader(proposal);
    }
    
    // Propose others
    for (const auto& uavId : candidates) {
        if (uavId != uavId_) {
            auto member = findMember(uavId);
            if (member) {
                LeaderProposal proposal;
                proposal.candidateId = uavId;
                proposal.capabilities = member->capabilities;
                proposal.score = calculateLeadershipScore(member->capabilities);
                proposeLeader(proposal);
            }
        }
    }
    
    // Select best
    if (!proposals_.empty()) {
        auto best = std::max_element(proposals_.begin(), proposals_.end(),
            [](const LeaderProposal& a, const LeaderProposal& b) {
                return a.score < b.score;
            });
        
        if (best != proposals_.end()) {
            announceLeader(best->candidateId);
        }
    }
    
    electionInProgress_ = false;
}

void SwarmPartitionManager::proposeLeader(const LeaderProposal& proposal) {
    proposals_.push_back(proposal);
    LOG_INFO("SwarmPartitionManager", 
             "Proposal from: " + proposal.candidateId + 
             " score: " + std::to_string(proposal.score));
}

void SwarmPartitionManager::announceLeader(const std::string& leaderId) {
    LOG_INFO("SwarmPartitionManager", "Announcing leader: " + leaderId);
    
    std::string partitionId = myPartitionId_;
    if (partitionId.empty()) {
        std::vector<std::string> members;
        for (const auto& pair : members_) {
            if (pair.second.isActive) members.push_back(pair.first);
        }
        if (!members.empty()) {
            createPartition(members);
            partitionId = myPartitionId_;
        }
    }
    
    auto partition = findPartition(partitionId);
    if (partition) {
        partition->leaderId = leaderId;
        for (auto& pair : members_) {
            if (pair.second.partitionId == partitionId) {
                pair.second.isLeader = (pair.first == leaderId);
            }
        }
        notifyLeaderChange(partitionId, leaderId, LeaderElectionReason::PARTITION_FORMED);
    }
}

bool SwarmPartitionManager::isLeader() const {
    return isLeader(uavId_);
}

bool SwarmPartitionManager::isLeader(const std::string& uavId) const {
    auto it = members_.find(uavId);
    return (it != members_.end()) ? it->second.isLeader : false;
}

std::string SwarmPartitionManager::getCurrentLeader() const {
    return getPartitionLeader(myPartitionId_);
}

std::string SwarmPartitionManager::getPartitionLeader(const std::string& partitionId) const {
    auto it = partitions_.find(partitionId);
    return (it != partitions_.end()) ? it->second.leaderId : "";
}

void SwarmPartitionManager::detectPartitions() {
    auto components = findConnectedComponents();
    
    if (components.size() <= 1) {
        connectivityState_ = components.empty() ? SwarmConnectivity::ISOLATED : SwarmConnectivity::FULLY_CONNECTED;
        if (partitions_.size() > 1) {
            std::vector<std::string> allIds;
            for (const auto& p : partitions_) allIds.push_back(p.first);
            mergePartitions(allIds);
        } else if (partitions_.empty() && !components.empty()) {
            createPartition(components[0]);
        }
        return;
    }
    
    connectivityState_ = SwarmConnectivity::PARTITIONED;
    LOG_WARNING("SwarmPartitionManager", 
                "Detected " + std::to_string(components.size()) + " partitions");
    
    auto oldPartitions = partitions_;
    partitions_.clear();
    
    for (const auto& component : components) {
        bool foundExisting = false;
        for (const auto& oldPair : oldPartitions) {
            if (isSubset(component, oldPair.second.memberIds)) {
                Partition partition = oldPair.second;
                partition.memberIds = component;
                partitions_[partition.partitionId] = partition;
                for (const auto& uavId : component) {
                    auto member = findMember(uavId);
                    if (member) member->partitionId = partition.partitionId;
                }
                foundExisting = true;
                break;
            }
        }
        if (!foundExisting) createPartition(component);
    }
    
    for (auto& pair : partitions_) {
        if (pair.second.leaderId.empty()) {
            auto bestLeader = electBestLeader(pair.second.memberIds);
            if (!bestLeader.empty()) {
                pair.second.leaderId = bestLeader;
                auto member = findMember(bestLeader);
                if (member) {
                    member->isLeader = true;
                    member->partitionId = pair.first;
                }
                notifyLeaderChange(pair.first, bestLeader, LeaderElectionReason::PARTITION_FORMED);
            }
        }
    }
    
    for (const auto& pair : partitions_) {
        if (std::find(pair.second.memberIds.begin(), pair.second.memberIds.end(), uavId_) 
            != pair.second.memberIds.end()) {
            myPartitionId_ = pair.first;
            break;
        }
    }
    
    notifyPartitionChange();
}

std::vector<std::vector<std::string>> SwarmPartitionManager::findConnectedComponents() {
    std::vector<std::vector<std::string>> components;
    std::set<std::string> visited;
    
    for (const auto& pair : members_) {
        const std::string& uavId = pair.first;
        if (visited.count(uavId) > 0) continue;
        
        std::vector<std::string> component;
        std::queue<std::string> queue;
        queue.push(uavId);
        visited.insert(uavId);
        
        while (!queue.empty()) {
            std::string current = queue.front();
            queue.pop();
            component.push_back(current);
            
            for (const auto& other : members_) {
                if (visited.count(other.first) == 0 && canReach(current, other.first)) {
                    queue.push(other.first);
                    visited.insert(other.first);
                }
            }
        }
        
        if (!component.empty()) components.push_back(component);
    }
    
    return components;
}

bool SwarmPartitionManager::canReach(const std::string& fromUavId, 
                                      const std::string& toUavId) const {
    auto fromMember = findMember(fromUavId);
    auto toMember = findMember(toUavId);
    
    if (!fromMember || !toMember) return false;
    if (!fromMember->isActive || !toMember->isActive) return false;
    if (fromMember->capabilities.signalStrength < 20.0 || 
        toMember->capabilities.signalStrength < 20.0) return false;
    if (fromMember->lastSeenSeconds > 30 || toMember->lastSeenSeconds > 30) return false;
    
    return true;
}

void SwarmPartitionManager::createPartition(const std::vector<std::string>& memberIds) {
    Partition partition;
    partition.partitionId = generateNewPartitionId();
    partition.memberIds = memberIds;
    partition.formedAt = getCurrentTimestamp();
    partition.leaderId = electBestLeader(memberIds);
    
    partitions_[partition.partitionId] = partition;
    
    for (const auto& uavId : memberIds) {
        auto member = findMember(uavId);
        if (member) {
            member->partitionId = partition.partitionId;
            member->isLeader = (uavId == partition.leaderId);
        }
    }
    
    if (std::find(memberIds.begin(), memberIds.end(), uavId_) != memberIds.end()) {
        myPartitionId_ = partition.partitionId;
    }
    
    LOG_INFO("SwarmPartitionManager", 
             "Created partition: " + partition.partitionId);
}

void SwarmPartitionManager::dissolvePartition(const std::string& partitionId) {
    auto it = partitions_.find(partitionId);
    if (it == partitions_.end()) return;
    
    for (const auto& uavId : it->second.memberIds) {
        auto member = findMember(uavId);
        if (member) {
            member->partitionId = "";
            member->isLeader = false;
        }
    }
    
    partitions_.erase(it);
    if (myPartitionId_ == partitionId) myPartitionId_ = "";
    
    LOG_INFO("SwarmPartitionManager", "Dissolved partition: " + partitionId);
}

void SwarmPartitionManager::mergePartitions(const std::vector<std::string>& partitionIds) {
    if (partitionIds.size() < 2) return;
    
    LOG_INFO("SwarmPartitionManager", "Merging partitions");
    
    std::vector<std::string> allMembers;
    std::string earliestFormedAt;
    std::vector<std::string> mergedIds;
    
    for (const auto& partitionId : partitionIds) {
        auto partition = findPartition(partitionId);
        if (!partition) continue;
        
        allMembers.insert(allMembers.end(), 
                         partition->memberIds.begin(), 
                         partition->memberIds.end());
        
        if (earliestFormedAt.empty() || partition->formedAt < earliestFormedAt) {
            earliestFormedAt = partition->formedAt;
        }
        mergedIds.push_back(partitionId);
    }
    
    std::sort(allMembers.begin(), allMembers.end());
    allMembers.erase(std::unique(allMembers.begin(), allMembers.end()), allMembers.end());
    
    Partition mergedPartition;
    mergedPartition.partitionId = generateNewPartitionId();
    mergedPartition.memberIds = allMembers;
    mergedPartition.formedAt = earliestFormedAt;
    mergedPartition.leaderId = electBestLeader(allMembers);
    
    for (const auto& partitionId : mergedIds) {
        dissolvePartition(partitionId);
    }
    
    partitions_[mergedPartition.partitionId] = mergedPartition;
    
    for (const auto& uavId : allMembers) {
        auto member = findMember(uavId);
        if (member) {
            member->partitionId = mergedPartition.partitionId;
            member->isLeader = (uavId == mergedPartition.leaderId);
        }
    }
    
    if (std::find(allMembers.begin(), allMembers.end(), uavId_) != allMembers.end()) {
        myPartitionId_ = mergedPartition.partitionId;
    }
    
    LOG_INFO("SwarmPartitionManager", 
             "Merged into partition: " + mergedPartition.partitionId);
    
    notifyLeaderChange(mergedPartition.partitionId, mergedPartition.leaderId, 
                       LeaderElectionReason::PARTITION_MERGE);
    notifyMerge(mergedIds, mergedPartition.partitionId);
}

std::vector<std::string> SwarmPartitionManager::getAllPartitionIds() const {
    std::vector<std::string> ids;
    for (const auto& pair : partitions_) ids.push_back(pair.first);
    return ids;
}

bool SwarmPartitionManager::isSubset(const std::vector<std::string>& subset, 
                                      const std::vector<std::string>& superset) const {
    for (const auto& elem : subset) {
        if (std::find(superset.begin(), superset.end(), elem) == superset.end()) {
            return false;
        }
    }
    return true;
}

SwarmState SwarmPartitionManager::getSwarmState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    SwarmState state;
    state.swarmId = swarmId_;
    state.connectivity = connectivityState_;
    state.globalLeaderId = isPartitioned() ? "" : getCurrentLeader();
    state.totalMembers = members_.size();
    
    int active = 0;
    for (const auto& pair : members_) {
        if (pair.second.isActive) active++;
    }
    state.activeMembers = active;
    state.lastUpdated = getCurrentTimestamp();
    
    for (const auto& pair : partitions_) state.partitions.push_back(pair.second);
    for (const auto& pair : members_) state.allMembers.push_back(pair.second);
    
    return state;
}

SwarmConnectivity SwarmPartitionManager::getConnectivityState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connectivityState_;
}

std::vector<Partition> SwarmPartitionManager::getAllPartitions() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Partition> result;
    for (const auto& pair : partitions_) result.push_back(pair.second);
    return result;
}

int SwarmPartitionManager::getPartitionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return partitions_.size();
}

bool SwarmPartitionManager::isPartitioned() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connectivityState_ == SwarmConnectivity::PARTITIONED ||
           connectivityState_ == SwarmConnectivity::FRAGMENTED;
}

std::string SwarmPartitionManager::getMyPartitionId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return myPartitionId_;
}

std::vector<std::string> SwarmPartitionManager::getPartitionMembers(
    const std::string& partitionId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = partitions_.find(partitionId);
    return (it != partitions_.end()) ? it->second.memberIds : std::vector<std::string>{};
}

bool SwarmPartitionManager::isInSamePartition(const std::string& uavId1, 
                                               const std::string& uavId2) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto m1 = findMember(uavId1);
    auto m2 = findMember(uavId2);
    if (!m1 || !m2) return false;
    return m1->partitionId == m2->partitionId && !m1->partitionId.empty();
}

void SwarmPartitionManager::assignSubTask(const std::string& partitionId, 
                                           const std::string& taskDescription) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto partition = findPartition(partitionId);
    if (partition) {
        partition->taskAssignment = taskDescription;
        LOG_INFO("SwarmPartitionManager", "Task assigned to partition: " + partitionId);
    }
}

void SwarmPartitionManager::syncTaskProgress(const std::string& taskId, int progress) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto member = findMember(uavId_);
    if (member) {
        member->currentTask = taskId;
        member->taskProgress = progress;
    }
}

void SwarmPartitionManager::coordinateAction(const std::string& action, 
                                              const nlohmann::json& params) {
    LOG_INFO("SwarmPartitionManager", "Coordinating action: " + action);
}

void SwarmPartitionManager::resolveConflict(const std::string& conflictType, 
                                             const nlohmann::json& conflictData) {
    LOG_INFO("SwarmPartitionManager", "Resolving conflict: " + conflictType);
}

void SwarmPartitionManager::negotiateMerge(const std::vector<std::string>& partitionIds) {
    LOG_INFO("SwarmPartitionManager", "Negotiating merge");
    bool allPrepared = true;
    for (const auto& partitionId : partitionIds) {
        if (!findPartition(partitionId)) {
            allPrepared = false;
            break;
        }
    }
    if (allPrepared) {
        mergePartitions(partitionIds);
    } else {
        LOG_ERROR("SwarmPartitionManager", "Merge negotiation failed");
    }
}

void SwarmPartitionManager::checkMemberTimeouts() {
    while (heartbeatRunning_) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            for (auto& pair : members_) {
                if (pair.first != uavId_) {
                    pair.second.lastSeenSeconds += 1;
                    if (pair.second.lastSeenSeconds > 30 && pair.second.isActive) {
                        pair.second.isActive = false;
                        LOG_WARNING("SwarmPartitionManager", "Member timeout: " + pair.first);
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

std::string SwarmPartitionManager::generateNewPartitionId() const {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    return "partition_" + std::to_string(ms) + "_" + uavId_;
}

std::string SwarmPartitionManager::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

void SwarmPartitionManager::notifyPartitionChange() {
    if (partitionChangeCallback_) {
        std::vector<Partition> partitions;
        for (const auto& pair : partitions_) partitions.push_back(pair.second);
        partitionChangeCallback_(partitions);
    }
}

void SwarmPartitionManager::notifyLeaderChange(const std::string& partitionId, 
                                                const std::string& newLeaderId,
                                                LeaderElectionReason reason) {
    if (leaderChangeCallback_) {
        leaderChangeCallback_(partitionId, newLeaderId, reason);
    }
}

void SwarmPartitionManager::notifyMerge(const std::vector<std::string>& mergedIds, 
                                         const std::string& newId) {
    if (mergeCallback_) {
        mergeCallback_(mergedIds, newId);
    }
}

std::string SwarmPartitionManager::electBestLeader(const std::vector<std::string>& candidates) {
    std::string bestCandidate;
    double bestScore = -1.0;
    
    for (const auto& uavId : candidates) {
        auto member = findMember(uavId);
        if (member && member->isActive) {
            double score = calculateLeadershipScore(member->capabilities);
            if (score > bestScore) {
                bestScore = score;
                bestCandidate = uavId;
            }
        }
    }
    
    return bestCandidate;
}

} // namespace nodeagent
