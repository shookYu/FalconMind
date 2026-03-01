#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

namespace nodeagent {

enum class SwarmConnectivity {
    FULLY_CONNECTED,
    PARTITIONED,
    FRAGMENTED,
    ISOLATED
};

struct SwarmMember {
    std::string uavId;
    std::string ipAddress;
    int signalStrength;
    std::string lastSeen;
    bool isLeader;
    nlohmann::json capabilities;
};

struct PartitionInfo {
    std::string partitionId;
    std::string leaderId;
    std::vector<std::string> memberIds;
    std::string formedAt;
};

struct SwarmState {
    std::string swarmId;
    std::string leaderId;
    std::vector<SwarmMember> members;
    SwarmConnectivity connectivity;
    std::vector<PartitionInfo> partitions;
    std::string lastUpdated;
};

struct UavHeartbeat {
    std::string uavId;
    std::string timestamp;
    double latitude;
    double longitude;
    double altitude;
    int battery;
    std::string currentTask;
    int taskProgress;
};

class InterUavManager {
public:
    using SwarmStateCallback = std::function<void(const SwarmState&)>;
    using LeaderElectionCallback = std::function<void(const std::string& newLeaderId)>;
    using PartitionCallback = std::function<void(bool isPartitioned, const PartitionInfo& info)>;

    explicit InterUavManager(const std::string& uavId, const std::string& swarmId);
    ~InterUavManager();

    bool initialize();
    void shutdown();

    void setSwarmStateCallback(SwarmStateCallback callback);
    void setLeaderElectionCallback(LeaderElectionCallback callback);
    void setPartitionCallback(PartitionCallback callback);

    void joinSwarm(const std::vector<std::string>& knownPeers);
    void leaveSwarm();

    void broadcastHeartbeat(const UavHeartbeat& heartbeat);
    void onHeartbeatReceived(const UavHeartbeat& heartbeat);

    void participateLeaderElection();
    bool isLeader() const;
    std::string getLeaderId() const;

    bool detectPartition();
    void handlePartition();
    void handleReconciliation();

    SwarmConnectivity getConnectivityState() const;
    SwarmState getSwarmState() const;
    std::vector<SwarmMember> getVisiblePeers() const;
    bool isPeerReachable(const std::string& peerId) const;

    void coordinateWithPeers(const std::string& coordinationType, const nlohmann::json& data);
    void onCoordinationReceived(const std::string& fromUavId, const std::string& coordinationType, const nlohmann::json& data);

    void syncTaskProgress(const std::string& taskId, int progress);
    void onTaskProgressSync(const std::string& fromUavId, const std::string& taskId, int progress);

    void mergePartition(const PartitionInfo& otherPartition);

private:
    void startDiscoveryService();
    void stopDiscoveryService();
    void startHeartbeatService();
    void stopHeartbeatService();
    void checkPeerTimeouts();
    void electLeader();
    void broadcastLeaderAnnouncement();
    void updateSwarmState();
    std::string generatePartitionId() const;
    std::string getCurrentTimestamp() const;

    std::string uavId_;
    std::string swarmId_;
    bool isLeader_;
    std::string currentLeaderId_;
    SwarmConnectivity connectivityState_;
    
    std::vector<SwarmMember> peers_;
    std::vector<PartitionInfo> knownPartitions_;
    SwarmState currentSwarmState_;
    
    bool isRunning_;
    
    SwarmStateCallback swarmStateCallback_;
    LeaderElectionCallback leaderElectionCallback_;
    PartitionCallback partitionCallback_;
};

} // namespace nodeagent
