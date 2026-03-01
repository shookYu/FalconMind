#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>
#include <map>
#include <set>
#include <queue>
#include <chrono>
#include <mutex>
#include <atomic>
#include <thread>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace nodeagent {

enum class SwarmConnectivity {
    FULLY_CONNECTED,
    PARTITIONED,
    FRAGMENTED,
    ISOLATED
};

enum class LeaderElectionReason {
    INITIAL_BOOT,
    GCS_DISCONNECTED,
    LEADER_FAILED,
    PARTITION_FORMED,
    PARTITION_MERGE,
    MANUAL_TRIGGER
};

struct UavCapability {
    std::string uavId;
    int batteryLevel;
    int computePower;
    bool hasNPU;
    double maxFlightTime;
    std::vector<std::string> sensors;
    double signalStrength;
};

struct SwarmMember {
    std::string uavId;
    std::string ipAddress;
    int lastSeenSeconds;
    bool isLeader;
    UavCapability capabilities;
    std::string currentTask;
    int taskProgress;
    std::string partitionId;
    bool isActive;
};

struct Partition {
    std::string partitionId;
    std::string leaderId;
    std::vector<std::string> memberIds;
    std::string formedAt;
    std::string taskAssignment;
    nlohmann::json sharedState;
};

struct SwarmState {
    std::string swarmId;
    SwarmConnectivity connectivity;
    std::string globalLeaderId;
    std::vector<Partition> partitions;
    std::vector<SwarmMember> allMembers;
    std::string lastUpdated;
    int totalMembers;
    int activeMembers;
};

struct LeaderProposal {
    std::string candidateId;
    double score;
    UavCapability capabilities;
    std::string reason;
};

class SwarmPartitionManager {
public:
    using PartitionChangeCallback = std::function<void(const std::vector<Partition>& partitions)>;
    using LeaderChangeCallback = std::function<void(const std::string& partitionId, const std::string& newLeaderId, LeaderElectionReason reason)>;
    using MergeCallback = std::function<void(const std::vector<std::string>& mergedPartitionIds, const std::string& newPartitionId)>;

    explicit SwarmPartitionManager(const std::string& uavId, const std::string& swarmId);
    ~SwarmPartitionManager();

    bool initialize();
    void shutdown();

    void setPartitionChangeCallback(PartitionChangeCallback callback);
    void setLeaderChangeCallback(LeaderChangeCallback callback);
    void setMergeCallback(MergeCallback callback);

    void addMember(const SwarmMember& member);
    void removeMember(const std::string& uavId);
    void updateMember(const std::string& uavId, const SwarmMember& member);
    void onMemberHeartbeat(const std::string& uavId);
    
    void triggerLeaderElection(LeaderElectionReason reason);
    void proposeLeader(const LeaderProposal& proposal);
    void voteForLeader(const std::string& candidateId);
    void announceLeader(const std::string& leaderId);
    bool isLeader() const;
    bool isLeader(const std::string& uavId) const;
    std::string getCurrentLeader() const;
    std::string getPartitionLeader(const std::string& partitionId) const;
    
    void detectPartitions();
    void createPartition(const std::vector<std::string>& memberIds);
    void dissolvePartition(const std::string& partitionId);
    void mergePartitions(const std::vector<std::string>& partitionIds);
    bool isInSamePartition(const std::string& uavId1, const std::string& uavId2) const;
    std::string getMyPartitionId() const;
    std::vector<std::string> getPartitionMembers(const std::string& partitionId) const;
    
    SwarmState getSwarmState() const;
    SwarmConnectivity getConnectivityState() const;
    std::vector<Partition> getAllPartitions() const;
    int getPartitionCount() const;
    bool isPartitioned() const;
    
    void assignSubTask(const std::string& partitionId, const std::string& taskDescription);
    void syncTaskProgress(const std::string& taskId, int progress);
    void coordinateAction(const std::string& action, const nlohmann::json& params);
    
    void resolveConflict(const std::string& conflictType, const nlohmann::json& conflictData);
    void negotiateMerge(const std::vector<std::string>& partitionIds);

private:
    double calculateLeadershipScore(const UavCapability& capabilities) const;
    std::string electBestLeader(const std::vector<std::string>& candidates);
    void startElectionTimer();
    void stopElectionTimer();
    
    std::vector<std::vector<std::string>> findConnectedComponents();
    bool canReach(const std::string& fromUavId, const std::string& toUavId) const;
    
    bool shouldMergePartitions(const Partition& p1, const Partition& p2) const;
    Partition mergeTwoPartitions(const Partition& p1, const Partition& p2);
    std::string generateNewPartitionId() const;
    
    void broadcastPartitionState();
    void synchronizePartitionStates();
    void reconcileTaskAssignments();
    
    void startHeartbeatCheck();
    void stopHeartbeatCheck();
    void checkMemberTimeouts();
    
    std::string getCurrentTimestamp() const;
    SwarmMember* findMember(const std::string& uavId);
    const SwarmMember* findMember(const std::string& uavId) const;
    Partition* findPartition(const std::string& partitionId);
    const Partition* findPartition(const std::string& partitionId) const;
    void notifyPartitionChange();
    void notifyLeaderChange(const std::string& partitionId, const std::string& newLeaderId, LeaderElectionReason reason);
    void notifyMerge(const std::vector<std::string>& mergedIds, const std::string& newId);
    std::vector<std::string> getAllPartitionIds() const;
    bool isSubset(const std::vector<std::string>& subset, const std::vector<std::string>& superset) const;

    std::string uavId_;
    std::string swarmId_;
    std::string myPartitionId_;
    
    std::map<std::string, SwarmMember> members_;
    std::map<std::string, Partition> partitions_;
    SwarmConnectivity connectivityState_;
    
    bool electionInProgress_;
    std::map<std::string, int> votes_;
    std::vector<LeaderProposal> proposals_;
    
    bool isRunning_;
    std::thread heartbeatThread_;
    std::atomic<bool> heartbeatRunning_;
    
    PartitionChangeCallback partitionChangeCallback_;
    LeaderChangeCallback leaderChangeCallback_;
    MergeCallback mergeCallback_;
    
    mutable std::mutex mutex_;
};

} // namespace nodeagent
