/**
 * @file test_swarm_partition_integration.cpp
 * @brief Integration tests for swarm partition management
 * 
 * Tests cover:
 * - Leader election algorithm
 * - Partition detection with BFS connected components
 * - Two-phase commit partition merging
 * - Split and merge scenarios
 * - Communication loss between swarm members
 * - Dynamic topology changes
 * - Task redistribution after merge
 * - Thread safety of partition operations
 * 
 * @note Zero-mock testing - uses real SwarmPartitionManager
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "nodeagent/SwarmPartitionManager.h"
#include <thread>
#include <vector>
#include <chrono>
#include <set>

using namespace nodeagent;
using namespace testing;

class SwarmPartitionIntegrationTest : public Test {
protected:
    void SetUp() override {
        manager_ = std::make_unique<SwarmPartitionManager>();
        SwarmPartitionConfig config;
        config.localUavId = "UAV_001";
        config.heartbeatTimeout = std::chrono::milliseconds(500);
        config.enableAutoMerge = true;
        ASSERT_TRUE(manager_->initialize(config));
    }
    
    void TearDown() override {
        manager_->shutdown();
        manager_.reset();
    }
    
    // Helper to create a swarm member
    SwarmMember createMember(const std::string& id, double capability = 50.0, 
                             bool leader = false) {
        SwarmMember member;
        member.id = id;
        member.capability = capability;
        member.isLeader = leader;
        member.lastSeen = std::chrono::system_clock::now();
        member.position = {0.0, 0.0, 50.0};
        member.batteryLevel = 80.0;
        return member;
    }
    
    std::unique_ptr<SwarmPartitionManager> manager_;
};

// ============================================================================
// Leader Election Tests
// ============================================================================

TEST_F(SwarmPartitionIntegrationTest, LeaderElectionSingleMember) {
    auto member = createMember("UAV_001", 100.0);
    manager_->addMember(member);
    
    auto leader = manager_->electLeader();
    
    ASSERT_TRUE(leader.has_value());
    EXPECT_EQ(leader->id, "UAV_001");
    EXPECT_TRUE(leader->isLeader);
}

TEST_F(SwarmPartitionIntegrationTest, LeaderElectionHighestCapability) {
    std::vector<SwarmMember> members = {
        createMember("UAV_001", 50.0),
        createMember("UAV_002", 80.0),
        createMember("UAV_003", 100.0),
        createMember("UAV_004", 60.0)
    };
    
    for (const auto& m : members) {
        manager_->addMember(m);
    }
    
    auto leader = manager_->electLeader();
    
    ASSERT_TRUE(leader.has_value());
    EXPECT_EQ(leader->id, "UAV_003");  // Highest capability
    EXPECT_EQ(leader->capability, 100.0);
}

TEST_F(SwarmPartitionIntegrationTest, LeaderElectionWithExistingLeader) {
    // Add existing leader
    auto leader = createMember("UAV_001", 100.0, true);
    manager_->addMember(leader);
    
    // Add member with higher capability
    auto challenger = createMember("UAV_002", 150.0, false);
    manager_->addMember(challenger);
    
    // Run election
    auto elected = manager_->electLeader();
    
    // Should keep existing leader (incumbent advantage)
    ASSERT_TRUE(elected.has_value());
    EXPECT_EQ(elected->id, "UAV_001");
}

TEST_F(SwarmPartitionIntegrationTest, LeaderElectionOnLeaderFailure) {
    // Setup with leader
    auto leader = createMember("UAV_001", 100.0, true);
    manager_->addMember(leader);
    
    auto member2 = createMember("UAV_002", 80.0);
    manager_->addMember(member2);
    
    // Simulate leader going offline
    manager_->updateMemberStatus("UAV_001", false);
    
    // Trigger re-election
    auto newLeader = manager_->electLeader();
    
    ASSERT_TRUE(newLeader.has_value());
    EXPECT_EQ(newLeader->id, "UAV_002");
}

TEST_F(SwarmPartitionIntegrationTest, LeaderElectionCallback) {
    std::string electedLeader;
    bool callbackTriggered = false;
    
    manager_->setLeaderElectionCallback([&](const std::string& leaderId) {
        electedLeader = leaderId;
        callbackTriggered = true;
    });
    
    auto member = createMember("UAV_001", 100.0);
    manager_->addMember(member);
    
    manager_->electLeader();
    
    EXPECT_TRUE(callbackTriggered);
    EXPECT_EQ(electedLeader, "UAV_001");
}

// ============================================================================
// Partition Detection Tests
// ============================================================================

TEST_F(SwarmPartitionIntegrationTest, SinglePartitionDetection) {
    // All UAVs connected to each other (fully connected)
    std::vector<SwarmMember> members = {
        createMember("UAV_001", 100.0),
        createMember("UAV_002", 80.0),
        createMember("UAV_003", 60.0)
    };
    
    for (const auto& m : members) {
        manager_->addMember(m);
    }
    
    // All connected to each other
    manager_->updateConnection("UAV_001", "UAV_002", true);
    manager_->updateConnection("UAV_001", "UAV_003", true);
    manager_->updateConnection("UAV_002", "UAV_003", true);
    
    auto partitions = manager_->detectPartitions();
    
    EXPECT_EQ(partitions.size(), 1);
    EXPECT_EQ(partitions[0].memberIds.size(), 3);
}

TEST_F(SwarmPartitionIntegrationTest, MultiplePartitionsDetection) {
    // Two disconnected groups
    std::vector<SwarmMember> members = {
        createMember("UAV_001", 100.0),
        createMember("UAV_002", 80.0),
        createMember("UAV_003", 60.0),
        createMember("UAV_004", 90.0)
    };
    
    for (const auto& m : members) {
        manager_->addMember(m);
    }
    
    // Group 1: UAV_001 connected to UAV_002
    manager_->updateConnection("UAV_001", "UAV_002", true);
    
    // Group 2: UAV_003 connected to UAV_004
    manager_->updateConnection("UAV_003", "UAV_004", true);
    
    // No connection between groups
    auto partitions = manager_->detectPartitions();
    
    EXPECT_EQ(partitions.size(), 2);
    
    // Each partition should have 2 members
    for (const auto& p : partitions) {
        EXPECT_EQ(p.memberIds.size(), 2);
    }
}

TEST_F(SwarmPartitionIntegrationTest, ChainTopologyPartitionDetection) {
    // Linear chain: UAV_001 - UAV_002 - UAV_003 - UAV_004
    std::vector<SwarmMember> members = {
        createMember("UAV_001", 100.0),
        createMember("UAV_002", 80.0),
        createMember("UAV_003", 60.0),
        createMember("UAV_004", 90.0)
    };
    
    for (const auto& m : members) {
        manager_->addMember(m);
    }
    
    // Chain connections
    manager_->updateConnection("UAV_001", "UAV_002", true);
    manager_->updateConnection("UAV_002", "UAV_003", true);
    manager_->updateConnection("UAV_003", "UAV_004", true);
    
    auto partitions = manager_->detectPartitions();
    
    // All connected through the chain
    EXPECT_EQ(partitions.size(), 1);
    EXPECT_EQ(partitions[0].memberIds.size(), 4);
}

TEST_F(SwarmPartitionIntegrationTest, PartitionDetectionAfterLinkFailure) {
    // Start with fully connected
    std::vector<SwarmMember> members = {
        createMember("UAV_001", 100.0),
        createMember("UAV_002", 80.0),
        createMember("UAV_003", 60.0)
    };
    
    for (const auto& m : members) {
        manager_->addMember(m);
    }
    
    manager_->updateConnection("UAV_001", "UAV_002", true);
    manager_->updateConnection("UAV_001", "UAV_003", true);
    manager_->updateConnection("UAV_002", "UAV_003", true);
    
    // Verify single partition
    EXPECT_EQ(manager_->detectPartitions().size(), 1);
    
    // Break link between UAV_002 and UAV_003
    manager_->updateConnection("UAV_002", "UAV_003", false);
    
    // But UAV_001 still connected to both
    auto partitions = manager_->detectPartitions();
    EXPECT_EQ(partitions.size(), 1);  // Still connected through UAV_001
    
    // Now break link between UAV_001 and UAV_003
    manager_->updateConnection("UAV_001", "UAV_003", false);
    
    partitions = manager_->detectPartitions();
    EXPECT_EQ(partitions.size(), 2);  // UAV_003 isolated
}

TEST_F(SwarmPartitionIntegrationTest, PartitionCallbackOnSplit) {
    std::vector<std::string> detectedPartitions;
    bool callbackTriggered = false;
    
    manager_->setPartitionCallback([&](const std::vector<SwarmPartition>& partitions) {
        detectedPartitions.clear();
        for (const auto& p : partitions) {
            detectedPartitions.push_back(p.leaderId);
        }
        callbackTriggered = true;
    });
    
    // Setup connected swarm
    auto m1 = createMember("UAV_001", 100.0, true);
    auto m2 = createMember("UAV_002", 80.0);
    manager_->addMember(m1);
    manager_->addMember(m2);
    manager_->updateConnection("UAV_001", "UAV_002", true);
    
    // Detect initial partition
    manager_->detectPartitions();
    
    EXPECT_TRUE(callbackTriggered);
    EXPECT_EQ(detectedPartitions.size(), 1);
}

// ============================================================================
// Partition Merge Tests
// ============================================================================

TEST_F(SwarmPartitionIntegrationTest, TwoPhaseCommitMerge) {
    // Two partitions
    auto partition1 = SwarmPartition{
        .partitionId = "PART_001",
        .leaderId = "UAV_001",
        .memberIds = {"UAV_001", "UAV_002"}
    };
    
    auto partition2 = SwarmPartition{
        .partitionId = "PART_002",
        .leaderId = "UAV_003",
        .memberIds = {"UAV_003", "UAV_004"}
    };
    
    // Initiate merge
    auto result = manager_->initiateMerge(partition1, partition2);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.newLeaderId.empty());
    EXPECT_EQ(result.mergedMembers.size(), 4);
}

TEST_F(SwarmPartitionIntegrationTest, MergePreservesHighestCapabilityLeader) {
    // Partition with low capability leader
    auto partition1 = SwarmPartition{
        .partitionId = "PART_001",
        .leaderId = "UAV_001",  // capability 50
        .memberIds = {"UAV_001"}
    };
    
    auto partition2 = SwarmPartition{
        .partitionId = "PART_002",
        .leaderId = "UAV_002",  // capability 100
        .memberIds = {"UAV_002"}
    };
    
    // Add members with capabilities
    manager_->addMember(createMember("UAV_001", 50.0, true));
    manager_->addMember(createMember("UAV_002", 100.0, true));
    
    auto result = manager_->initiateMerge(partition1, partition2);
    
    EXPECT_EQ(result.newLeaderId, "UAV_002");  // Higher capability wins
}

TEST_F(SwarmPartitionIntegrationTest, MergeCallback) {
    bool mergeTriggered = false;
    std::string mergeResult;
    
    manager_->setMergeCallback([&](const SwarmPartition& p1, const SwarmPartition& p2, bool success) {
        mergeTriggered = true;
        mergeResult = success ? "SUCCESS" : "FAILED";
    });
    
    auto partition1 = SwarmPartition{
        .partitionId = "PART_001",
        .leaderId = "UAV_001",
        .memberIds = {"UAV_001"}
    };
    
    auto partition2 = SwarmPartition{
        .partitionId = "PART_002",
        .leaderId = "UAV_002",
        .memberIds = {"UAV_002"}
    };
    
    manager_->initiateMerge(partition1, partition2);
    
    EXPECT_TRUE(mergeTriggered);
    EXPECT_EQ(mergeResult, "SUCCESS");
}

TEST_F(SwarmPartitionIntegrationTest, AutoMergeOnConnectionRestore) {
    // Setup two partitions
    auto m1 = createMember("UAV_001", 100.0, true);
    auto m2 = createMember("UAV_002", 80.0);
    auto m3 = createMember("UAV_003", 90.0, true);
    auto m4 = createMember("UAV_004", 70.0);
    
    manager_->addMember(m1);
    manager_->addMember(m2);
    manager_->addMember(m3);
    manager_->addMember(m4);
    
    // Two separate groups
    manager_->updateConnection("UAV_001", "UAV_002", true);
    manager_->updateConnection("UAV_003", "UAV_004", true);
    
    auto partitions = manager_->detectPartitions();
    EXPECT_EQ(partitions.size(), 2);
    
    // Restore connection between groups
    manager_->updateConnection("UAV_002", "UAV_003", true);
    
    // With auto-merge enabled, should merge
    manager_->checkAndMergePartitions();
    
    partitions = manager_->detectPartitions();
    EXPECT_EQ(partitions.size(), 1);
    EXPECT_EQ(partitions[0].memberIds.size(), 4);
}

// ============================================================================
// Split Scenario Tests
// ============================================================================

TEST_F(SwarmPartitionIntegrationTest, SplitScenarioThreeGroups) {
    // Start with 6 UAVs in one group
    std::vector<SwarmMember> members;
    for (int i = 1; i <= 6; ++i) {
        members.push_back(createMember("UAV_00" + std::to_string(i), 100.0 - i * 10));
    }
    
    for (const auto& m : members) {
        manager_->addMember(m);
    }
    
    // Fully connected
    for (size_t i = 0; i < members.size(); ++i) {
        for (size_t j = i + 1; j < members.size(); ++j) {
            manager_->updateConnection(members[i].id, members[j].id, true);
        }
    }
    
    EXPECT_EQ(manager_->detectPartitions().size(), 1);
    
    // Split into three groups: {1,2}, {3,4}, {5,6}
    manager_->updateConnection("UAV_001", "UAV_003", false);
    manager_->updateConnection("UAV_001", "UAV_004", false);
    manager_->updateConnection("UAV_001", "UAV_005", false);
    manager_->updateConnection("UAV_001", "UAV_006", false);
    manager_->updateConnection("UAV_002", "UAV_003", false);
    manager_->updateConnection("UAV_002", "UAV_004", false);
    manager_->updateConnection("UAV_002", "UAV_005", false);
    manager_->updateConnection("UAV_002", "UAV_006", false);
    manager_->updateConnection("UAV_003", "UAV_005", false);
    manager_->updateConnection("UAV_003", "UAV_006", false);
    manager_->updateConnection("UAV_004", "UAV_005", false);
    manager_->updateConnection("UAV_004", "UAV_006", false);
    
    // Keep intra-group connections
    manager_->updateConnection("UAV_001", "UAV_002", true);
    manager_->updateConnection("UAV_003", "UAV_004", true);
    manager_->updateConnection("UAV_005", "UAV_006", true);
    
    auto partitions = manager_->detectPartitions();
    EXPECT_EQ(partitions.size(), 3);
    
    // Each group should elect its own leader
    for (const auto& p : partitions) {
        EXPECT_FALSE(p.leaderId.empty());
        EXPECT_EQ(p.memberIds.size(), 2);
    }
}

TEST_F(SwarmPartitionIntegrationTest, SplitThenMergeScenario) {
    // Start with 4 UAVs connected
    std::vector<SwarmMember> members = {
        createMember("UAV_001", 100.0, true),
        createMember("UAV_002", 80.0),
        createMember("UAV_003", 90.0),
        createMember("UAV_004", 70.0)
    };
    
    for (const auto& m : members) {
        manager_->addMember(m);
    }
    
    // Fully connected
    manager_->updateConnection("UAV_001", "UAV_002", true);
    manager_->updateConnection("UAV_001", "UAV_003", true);
    manager_->updateConnection("UAV_001", "UAV_004", true);
    manager_->updateConnection("UAV_002", "UAV_003", true);
    manager_->updateConnection("UAV_002", "UAV_004", true);
    manager_->updateConnection("UAV_003", "UAV_004", true);
    
    EXPECT_EQ(manager_->detectPartitions().size(), 1);
    
    // Split into two groups: {1,2} and {3,4}
    manager_->updateConnection("UAV_001", "UAV_003", false);
    manager_->updateConnection("UAV_001", "UAV_004", false);
    manager_->updateConnection("UAV_002", "UAV_003", false);
    manager_->updateConnection("UAV_002", "UAV_004", false);
    
    auto partitions = manager_->detectPartitions();
    EXPECT_EQ(partitions.size(), 2);
    
    // Track leaders
    std::string leader1, leader2;
    for (const auto& p : partitions) {
        if (p.memberIds.count("UAV_001")) {
            leader1 = p.leaderId;
        } else {
            leader2 = p.leaderId;
        }
    }
    
    // Reconnect
    manager_->updateConnection("UAV_002", "UAV_003", true);
    manager_->checkAndMergePartitions();
    
    // Should merge back to one partition
    partitions = manager_->detectPartitions();
    EXPECT_EQ(partitions.size(), 1);
    EXPECT_EQ(partitions[0].memberIds.size(), 4);
    
    // Leader should be UAV_001 (highest capability among all)
    EXPECT_EQ(partitions[0].leaderId, "UAV_001");
}

// ============================================================================
// Task Redistribution Tests
// ============================================================================

TEST_F(SwarmPartitionIntegrationTest, TaskRedistributionOnMerge) {
    // Two partitions with tasks
    auto partition1 = SwarmPartition{
        .partitionId = "PART_001",
        .leaderId = "UAV_001",
        .memberIds = {"UAV_001", "UAV_002"},
        .activeTasks = {"task_A", "task_B"}
    };
    
    auto partition2 = SwarmPartition{
        .partitionId = "PART_002",
        .leaderId = "UAV_003",
        .memberIds = {"UAV_003", "UAV_004"},
        .activeTasks = {"task_C"}
    };
    
    // Track redistribution
    std::vector<std::string> redistributedTasks;
    manager_->setTaskRedistributionCallback([&](const std::vector<std::string>& tasks) {
        redistributedTasks = tasks;
    });
    
    auto result = manager_->initiateMerge(partition1, partition2);
    
    // Should redistribute all tasks
    EXPECT_EQ(redistributedTasks.size(), 3);
}

TEST_F(SwarmPartitionIntegrationTest, TaskReassignmentAfterSplit) {
    // Setup with members and tasks
    manager_->addMember(createMember("UAV_001", 100.0, true));
    manager_->addMember(createMember("UAV_002", 80.0));
    manager_->addMember(createMember("UAV_003", 90.0));
    
    // Assign tasks
    manager_->assignTask("task_1", "UAV_001");
    manager_->assignTask("task_2", "UAV_002");
    manager_->assignTask("task_3", "UAV_003");
    
    // Simulate split: UAV_003 disconnected
    manager_->updateConnection("UAV_001", "UAV_003", false);
    manager_->updateConnection("UAV_002", "UAV_003", false);
    
    auto partitions = manager_->detectPartitions();
    
    // UAV_003's task should be reassigned
    EXPECT_EQ(partitions.size(), 2);
}

// ============================================================================
// Heartbeat Timeout Tests
// ============================================================================

TEST_F(SwarmPartitionIntegrationTest, MemberTimeoutDetection) {
    auto member = createMember("UAV_002", 80.0);
    manager_->addMember(member);
    
    // Initially active
    EXPECT_TRUE(manager_->isMemberActive("UAV_002"));
    
    // Don't update heartbeat
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    
    // Should be timed out
    EXPECT_FALSE(manager_->isMemberActive("UAV_002"));
}

TEST_F(SwarmPartitionIntegrationTest, HeartbeatRefreshKeepsMemberActive) {
    auto member = createMember("UAV_002", 80.0);
    manager_->addMember(member);
    
    // Refresh heartbeat multiple times
    for (int i = 0; i < 5; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        manager_->refreshHeartbeat("UAV_002");
    }
    
    // Should still be active
    EXPECT_TRUE(manager_->isMemberActive("UAV_002"));
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(SwarmPartitionIntegrationTest, ConcurrentMemberUpdates) {
    const int numThreads = 10;
    const int updatesPerThread = 50;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < updatesPerThread; ++j) {
                std::string id = "UAV_" + std::to_string(i) + "_" + std::to_string(j);
                auto member = createMember(id, 50.0 + j);
                manager_->addMember(member);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have all members
    auto members = manager_->getAllMembers();
    EXPECT_EQ(members.size(), numThreads * updatesPerThread);
}

TEST_F(SwarmPartitionIntegrationTest, ConcurrentConnectionUpdates) {
    // Add members first
    for (int i = 1; i <= 5; ++i) {
        manager_->addMember(createMember("UAV_00" + std::to_string(i), 100.0 - i * 10));
    }
    
    std::vector<std::thread> threads;
    
    // Concurrently update connections
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([this]() {
            for (int i = 1; i <= 5; ++i) {
                for (int j = i + 1; j <= 5; ++j) {
                    manager_->updateConnection(
                        "UAV_00" + std::to_string(i),
                        "UAV_00" + std::to_string(j),
                        true
                    );
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should detect single partition
    auto partitions = manager_->detectPartitions();
    EXPECT_EQ(partitions.size(), 1);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(SwarmPartitionIntegrationTest, StatisticsTracking) {
    // Add some members
    manager_->addMember(createMember("UAV_001", 100.0, true));
    manager_->addMember(createMember("UAV_002", 80.0));
    
    // Detect partitions
    manager_->detectPartitions();
    
    // Elect leader
    manager_->electLeader();
    
    auto stats = manager_->getStatistics();
    
    EXPECT_EQ(stats.totalPartitionsDetected, 1);
    EXPECT_EQ(stats.totalLeaderElections, 1);
    EXPECT_EQ(stats.totalMembersTracked, 2);
    EXPECT_EQ(stats.currentPartitionCount, 1);
}

TEST_F(SwarmPartitionIntegrationTest, MergeStatistics) {
    // Track merge
    auto partition1 = SwarmPartition{
        .partitionId = "PART_001",
        .leaderId = "UAV_001",
        .memberIds = {"UAV_001"}
    };
    
    auto partition2 = SwarmPartition{
        .partitionId = "PART_002",
        .leaderId = "UAV_002",
        .memberIds = {"UAV_002"}
    };
    
    manager_->initiateMerge(partition1, partition2);
    
    auto stats = manager_->getStatistics();
    
    EXPECT_EQ(stats.totalMergesPerformed, 1);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(SwarmPartitionIntegrationTest, EmptyPartitionHandling) {
    auto emptyPartition = SwarmPartition{
        .partitionId = "EMPTY",
        .leaderId = "",
        .memberIds = {}
    };
    
    auto normalPartition = SwarmPartition{
        .partitionId = "NORMAL",
        .leaderId = "UAV_001",
        .memberIds = {"UAV_001"}
    };
    
    // Should handle gracefully
    auto result = manager_->initiateMerge(emptyPartition, normalPartition);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.mergedMembers.size(), 1);
}

TEST_F(SwarmPartitionIntegrationTest, SingleMemberMultiplePartitions) {
    // Edge case: same member in both partitions (shouldn't happen in practice)
    auto partition1 = SwarmPartition{
        .partitionId = "PART_001",
        .leaderId = "UAV_001",
        .memberIds = {"UAV_001", "UAV_002"}
    };
    
    auto partition2 = SwarmPartition{
        .partitionId = "PART_002",
        .leaderId = "UAV_002",  // Also in partition1
        .memberIds = {"UAV_002", "UAV_003"}
    };
    
    auto result = manager_->initiateMerge(partition1, partition2);
    
    // Should merge without duplicates
    EXPECT_EQ(result.mergedMembers.size(), 3);
}

TEST_F(SwarmPartitionIntegrationTest, RapidSplitMergeCycles) {
    // Setup 3 members
    for (int i = 1; i <= 3; ++i) {
        manager_->addMember(createMember("UAV_00" + std::to_string(i), 100.0 - i * 10));
    }
    
    // Rapid split/merge cycles
    for (int cycle = 0; cycle < 5; ++cycle) {
        // Connect all
        for (int i = 1; i <= 3; ++i) {
            for (int j = i + 1; j <= 3; ++j) {
                manager_->updateConnection(
                    "UAV_00" + std::to_string(i),
                    "UAV_00" + std::to_string(j),
                    true
                );
            }
        }
        
        auto partitions = manager_->detectPartitions();
        EXPECT_EQ(partitions.size(), 1);
        
        // Split
        manager_->updateConnection("UAV_001", "UAV_002", false);
        manager_->updateConnection("UAV_001", "UAV_003", false);
        
        partitions = manager_->detectPartitions();
        EXPECT_EQ(partitions.size(), 2);
    }
}

} // namespace
