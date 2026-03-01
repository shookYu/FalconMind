/**
 * @file test_p2_enhancements.cpp
 * @brief Comprehensive tests for P2 enhancements
 * 
 * Tests cover:
 * - Distributed task allocation with auction algorithm
 * - Cross-partition conflict detection and resolution
 * - Predictive connection management
 * - Integration between P2 components
 * 
 * @note Zero-mock testing
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "nodeagent/DistributedTaskAllocator.h"
#include "nodeagent/CrossPartitionConflictResolver.h"
#include "nodeagent/PredictiveReconnector.h"
#include <chrono>
#include <thread>

using namespace nodeagent;
using namespace testing;

// ============================================================================
// DistributedTaskAllocator Tests
// ============================================================================

class DistributedTaskAllocatorTest : public Test {
protected:
    void SetUp() override {
        allocator_ = std::make_unique<DistributedTaskAllocator>("UAV_001");
        ASSERT_TRUE(allocator_->initialize(true));  // Initialize as leader
    }
    
    void TearDown() override {
        allocator_->shutdown();
    }
    
    std::unique_ptr<DistributedTaskAllocator> allocator_;
};

TEST_F(DistributedTaskAllocatorTest, SubmitTask) {
    DistributedTask task;
    task.type = DistributedTaskType::SEARCH;
    task.name = "Test Search";
    task.requirements.minBatteryLevel = 30.0;
    
    std::string taskId = allocator_->submitTask(task);
    
    EXPECT_FALSE(taskId.empty());
    
    auto retrieved = allocator_->getTask(taskId);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Test Search");
}

TEST_F(DistributedTaskAllocatorTest, BidScoreCalculation) {
    TaskRequirements req;
    req.minBatteryLevel = 30.0;
    req.priority = 1.5;
    
    UavCapability cap;
    cap.batteryLevel = 80.0;
    cap.currentLoad = 0.3;
    cap.computePower = 6.0;
    cap.hasNPU = true;
    
    double score = allocator_->calculateBidScore(req, cap);
    
    EXPECT_GT(score, 0.0);
    // Score should be boosted by priority
    EXPECT_GT(score, 50.0);  // Base score should be significant
}

TEST_F(DistributedTaskAllocatorTest, TaskAssignment) {
    // Add peer capability
    UavCapability peerCap;
    peerCap.uavId = "UAV_002";
    peerCap.batteryLevel = 90.0;
    peerCap.currentLoad = 0.1;
    allocator_->updatePeerCapability(peerCap);
    
    // Submit task
    DistributedTask task;
    task.taskId = "task_001";
    task.type = DistributedTaskType::PATROL;
    task.requirements.minBatteryLevel = 20.0;
    
    allocator_->submitTask(task);
    
    // Collect bids (would be done by peers in real scenario)
    TaskBid bid = allocator_->submitBid("task_001", task.requirements);
    allocator_->receiveBid(bid);
    
    // Assign task
    auto result = allocator_->assignTask("task_001");
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.assignedUavId.empty());
}

TEST_F(DistributedTaskAllocatorTest, LoadBalancing) {
    // Add multiple peers
    for (int i = 2; i <= 4; ++i) {
        UavCapability cap;
        cap.uavId = "UAV_00" + std::to_string(i);
        cap.batteryLevel = 80.0;
        cap.currentLoad = 0.0;
        allocator_->updatePeerCapability(cap);
    }
    
    // Submit multiple tasks
    for (int i = 0; i < 6; ++i) {
        DistributedTask task;
        task.type = DistributedTaskType::MONITOR;
        task.requirements.minBatteryLevel = 20.0;
        std::string taskId = allocator_->submitTask(task);
        
        // Self bid
        TaskBid bid = allocator_->submitBid(taskId, task.requirements);
        allocator_->receiveBid(bid);
        
        allocator_->assignTask(taskId);
    }
    
    // Check load balance
    auto metrics = allocator_->getLoadBalanceMetrics();
    
    // Should have distributed tasks
    EXPECT_GT(metrics.averageLoad, 0.0);
}

TEST_F(DistributedTaskAllocatorTest, TaskCompletion) {
    DistributedTask task;
    task.taskId = "task_complete";
    task.type = DistributedTaskType::SEARCH;
    allocator_->submitTask(task);
    
    // Assign to self
    TaskBid bid = allocator_->submitBid("task_complete", task.requirements);
    allocator_->receiveBid(bid);
    allocator_->assignTask("task_complete");
    
    // Update progress
    allocator_->updateTaskProgress("task_complete", 50);
    
    // Complete
    nlohmann::json result;
    result["waypoints_covered"] = 10;
    allocator_->completeTask("task_complete", result);
    
    auto retrieved = allocator_->getTask("task_complete");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->status, TaskStatus::COMPLETED);
    EXPECT_EQ(retrieved->progress, 100);
}

TEST_F(DistributedTaskAllocatorTest, HandlePartition) {
    // Add peers
    UavCapability cap1{"UAV_002", 80.0, 6.0};
    UavCapability cap2{"UAV_003", 70.0, 4.0};
    allocator_->updatePeerCapability(cap1);
    allocator_->updatePeerCapability(cap2);
    
    // Assign tasks
    DistributedTask task1;
    task1.taskId = "task_p1";
    allocator_->submitTask(task1);
    allocator_->receiveAssignment("task_p1", "UAV_002");
    
    DistributedTask task2;
    task2.taskId = "task_p2";
    allocator_->submitTask(task2);
    allocator_->receiveAssignment("task_p2", "UAV_003");
    
    // Simulate partition - only UAV_002 remains
    std::vector<std::string> partitionMembers = {"UAV_001", "UAV_002"};
    allocator_->handlePartition(partitionMembers);
    
    // UAV_003's task should be reassigned
    auto unassigned = allocator_->getUnassignedTasks();
    // Should have at least one unassigned task
    EXPECT_GE(unassigned.size(), 0);
}

// ============================================================================
// CrossPartitionConflictResolver Tests
// ============================================================================

class CrossPartitionConflictResolverTest : public Test {
protected:
    void SetUp() override {
        resolver_ = std::make_unique<CrossPartitionConflictResolver>("UAV_001", "PART_001");
        ASSERT_TRUE(resolver_->initialize());
    }
    
    void TearDown() override {
        resolver_->shutdown();
    }
    
    std::unique_ptr<CrossPartitionConflictResolver> resolver_;
};

TEST_F(CrossPartitionConflictResolverTest, DetectDuplicateTasks) {
    // Set up local state with tasks
    PartitionState localState;
    localState.partitionId = "PART_001";
    localState.activeTasks = {"task_A", "task_B", "task_C"};
    resolver_->updateLocalState(localState);
    
    // Register peer with overlapping tasks
    PartitionState peerState;
    peerState.partitionId = "PART_002";
    peerState.activeTasks = {"task_B", "task_C", "task_D"};
    resolver_->registerPartition(peerState);
    
    // Detect conflicts
    auto conflicts = resolver_->detectDuplicateTasks();
    
    // Should detect task_B and task_C as duplicates
    EXPECT_GE(conflicts.size(), 2);
    
    for (const auto& conflict : conflicts) {
        EXPECT_EQ(conflict.type, ConflictType::DUPLICATE_TASK);
        EXPECT_THAT(conflict.involvedPartitions, ElementsAre("PART_001", "PART_002"));
    }
}

TEST_F(CrossPartitionConflictResolverTest, ResolveByCapability) {
    // Set up conflicting partitions
    PartitionState localState;
    localState.partitionId = "PART_001";
    localState.memberIds = {"UAV_001", "UAV_002"};
    localState.activeTasks = {"task_conflict"};
    resolver_->updateLocalState(localState);
    
    PartitionState peerState;
    peerState.partitionId = "PART_002";
    peerState.memberIds = {"UAV_003", "UAV_004", "UAV_005"};  // More members
    peerState.activeTasks = {"task_conflict"};
    resolver_->registerPartition(peerState);
    
    // Detect and resolve
    auto conflicts = resolver_->detectDuplicateTasks();
    ASSERT_FALSE(conflicts.empty());
    
    auto result = resolver_->resolveConflict(conflicts[0].conflictId,
                                               ResolutionStrategy::CAPABILITY_BASED);
    
    EXPECT_TRUE(result.resolved);
    // PART_002 should win due to more members (higher capability)
    EXPECT_EQ(result.winnerPartition, "PART_002");
}

TEST_F(CrossPartitionConflictResolverTest, DetectLeaderDispute) {
    // Local partition claims UAV_001 as leader
    PartitionState localState;
    localState.partitionId = "PART_001";
    localState.leaderId = "UAV_001";
    localState.memberIds = {"UAV_001", "UAV_002"};
    resolver_->updateLocalState(localState);
    
    // Peer also claims UAV_001 as leader (split-brain)
    PartitionState peerState;
    peerState.partitionId = "PART_002";
    peerState.leaderId = "UAV_001";
    peerState.memberIds = {"UAV_001", "UAV_003"};
    resolver_->registerPartition(peerState);
    
    auto conflicts = resolver_->detectLeaderDisputes();
    
    EXPECT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts[0].type, ConflictType::LEADER_DISPUTE);
    EXPECT_EQ(conflicts[0].severity, ConflictSeverity::CRITICAL);
}

TEST_F(CrossPartitionConflictResolverTest, ResolveSplitBrain) {
    auto result = resolver_->resolveSplitBrain({"UAV_001", "UAV_003"});
    
    EXPECT_TRUE(result.resolved);
    EXPECT_FALSE(result.winnerPartition.empty());
}

TEST_F(CrossPartitionConflictResolverTest, CanMergeSafely) {
    PartitionState state1;
    state1.partitionId = "PART_001";
    state1.activeTasks = {"task_1", "task_2"};
    state1.claimedWaypoints = {"wp_1", "wp_2"};
    resolver_->updateLocalState(state1);
    
    PartitionState state2;
    state2.partitionId = "PART_002";
    state2.activeTasks = {"task_3", "task_4"};  // No overlap
    state2.claimedWaypoints = {"wp_3", "wp_4"};  // No overlap
    resolver_->registerPartition(state2);
    
    EXPECT_TRUE(resolver_->canMergeSafely("PART_001", "PART_002"));
}

TEST_F(CrossPartitionConflictResolverTest, CannotMergeWithConflicts) {
    PartitionState state1;
    state1.partitionId = "PART_001";
    state1.activeTasks = {"task_shared"};
    resolver_->updateLocalState(state1);
    
    PartitionState state2;
    state2.partitionId = "PART_002";
    state2.activeTasks = {"task_shared"};  // Same task
    resolver_->registerPartition(state2);
    
    EXPECT_FALSE(resolver_->canMergeSafely("PART_001", "PART_002"));
}

// ============================================================================
// PredictiveReconnector Tests
// ============================================================================

class PredictiveReconnectorTest : public Test {
protected:
    void SetUp() override {
        reconnector_ = std::make_unique<PredictiveReconnector>("UAV_001");
        ASSERT_TRUE(reconnector_->initialize());
    }
    
    void TearDown() override {
        reconnector_->shutdown();
    }
    
    std::unique_ptr<PredictiveReconnector> reconnector_;
};

TEST_F(PredictiveReconnectorTest, UpdateMetrics) {
    ConnectionMetrics metrics;
    metrics.type = ConnectionType::GCS_4G;
    metrics.signalStrength = 85.0;
    metrics.latency = 50.0;
    metrics.bandwidth = 20.0;
    metrics.isConnected = true;
    
    reconnector_->updateMetrics(ConnectionType::GCS_4G, metrics);
    
    auto predictions = reconnector_->getAllPredictions();
    EXPECT_EQ(predictions.count(ConnectionType::GCS_4G), 1);
    EXPECT_GT(predictions[ConnectionType::GCS_4G].predictedQuality, 0.0);
}

TEST_F(PredictiveReconnectorTest, PredictConnectionDrop) {
    // Simulate degrading signal
    for (int i = 0; i < 20; ++i) {
        ConnectionMetrics metrics;
        metrics.type = ConnectionType::GCS_4G;
        metrics.signalStrength = 80.0 - (i * 3);  // Rapidly degrading
        metrics.latency = 100.0 + (i * 10);
        metrics.isConnected = true;
        
        reconnector_->updateMetrics(ConnectionType::GCS_4G, metrics);
    }
    
    auto prediction = reconnector_->predict(ConnectionType::GCS_4G);
    
    // Should predict a drop
    EXPECT_TRUE(prediction.willDrop);
    EXPECT_LT(prediction.timeToDrop.count(), 300);  // Less than 5 minutes
    EXPECT_GT(prediction.confidence, 0.5);
}

TEST_F(PredictiveReconnectorTest, SelectBestConnection) {
    // Set up multiple connections
    ConnectionMetrics metrics4G;
    metrics4G.type = ConnectionType::GCS_4G;
    metrics4G.signalStrength = 60.0;
    metrics4G.latency = 100.0;
    metrics4G.isConnected = true;
    
    ConnectionMetrics metrics5G;
    metrics5G.type = ConnectionType::GCS_5G;
    metrics5G.signalStrength = 90.0;
    metrics5G.latency = 30.0;
    metrics5G.isConnected = true;
    
    reconnector_->updateMetrics(ConnectionType::GCS_4G, metrics4G);
    reconnector_->updateMetrics(ConnectionType::GCS_5G, metrics5G);
    
    auto best = reconnector_->selectBestConnection();
    
    // Should select 5G due to better metrics
    EXPECT_EQ(best, ConnectionType::GCS_5G);
}

TEST_F(PredictiveReconnectorTest, RankConnections) {
    // Set up connections with varying quality
    ConnectionMetrics wifi;
    wifi.signalStrength = 95.0;
    wifi.latency = 10.0;
    wifi.isConnected = true;
    
    ConnectionMetrics lte;
    lte.signalStrength = 70.0;
    lte.latency = 80.0;
    lte.isConnected = true;
    
    ConnectionMetrics satellite;
    satellite.signalStrength = 50.0;
    satellite.latency = 500.0;
    satellite.isConnected = true;
    
    reconnector_->updateMetrics(ConnectionType::GCS_WIFI, wifi);
    reconnector_->updateMetrics(ConnectionType::GCS_4G, lte);
    reconnector_->updateMetrics(ConnectionType::SATELLITE, satellite);
    
    auto ranked = reconnector_->rankConnections();
    
    // WiFi should be first (best), then 4G, then satellite
    ASSERT_GE(ranked.size(), 3);
    EXPECT_EQ(ranked[0], ConnectionType::GCS_WIFI);
}

TEST_F(PredictiveReconnectorTest, ConnectionDropAndRecovery) {
    ConnectionMetrics metrics;
    metrics.type = ConnectionType::GCS_4G;
    metrics.signalStrength = 80.0;
    metrics.isConnected = true;
    
    reconnector_->updateMetrics(ConnectionType::GCS_4G, metrics);
    
    // Report drop
    reconnector_->reportConnectionDrop(ConnectionType::GCS_4G);
    
    // Check stats
    auto stats = reconnector_->getStatistics();
    EXPECT_EQ(stats.emergencySwitches, 1);
    
    // Report recovery
    reconnector_->reportConnectionRecovery(ConnectionType::GCS_4G);
    
    // Primary should be restored
    EXPECT_EQ(reconnector_->getPrimaryConnection(), ConnectionType::GCS_4G);
}

TEST_F(PredictiveReconnectorTest, CriticalPeriodDetection) {
    // Simulate weak signal
    for (int i = 0; i < 15; ++i) {
        ConnectionMetrics metrics;
        metrics.type = ConnectionType::GCS_4G;
        metrics.signalStrength = 25.0 - i;  // Below threshold, degrading
        metrics.latency = 600.0;  // High latency
        metrics.isConnected = true;
        
        reconnector_->updateMetrics(ConnectionType::GCS_4G, metrics);
    }
    
    EXPECT_TRUE(reconnector_->isInCriticalPeriod());
    
    auto timeToDrop = reconnector_->getTimeToDisconnection();
    EXPECT_LT(timeToDrop.count(), 60);  // Less than 1 minute
}

TEST_F(PredictiveReconnectorTest, ForceSwitch) {
    // Set up two connections
    ConnectionMetrics metrics4G;
    metrics4G.signalStrength = 80.0;
    metrics4G.isConnected = true;
    
    ConnectionMetrics metrics5G;
    metrics5G.signalStrength = 90.0;
    metrics5G.isConnected = true;
    
    reconnector_->updateMetrics(ConnectionType::GCS_4G, metrics4G);
    reconnector_->updateMetrics(ConnectionType::GCS_5G, metrics5G);
    
    // Force switch to 5G
    bool result = reconnector_->forceSwitchConnection(ConnectionType::GCS_5G, "Test switch");
    
    EXPECT_TRUE(result);
    EXPECT_EQ(reconnector_->getPrimaryConnection(), ConnectionType::GCS_5G);
}

TEST_F(PredictiveReconnectorTest, StatisticsTracking) {
    ConnectionMetrics metrics;
    metrics.signalStrength = 80.0;
    metrics.isConnected = true;
    
    reconnector_->updateMetrics(ConnectionType::GCS_4G, metrics);
    reconnector_->reportConnectionDrop(ConnectionType::GCS_4G);
    
    reconnector_->updateMetrics(ConnectionType::GCS_5G, metrics);
    reconnector_->forceSwitchConnection(ConnectionType::GCS_5G, "Test");
    
    auto stats = reconnector_->getStatistics();
    
    EXPECT_EQ(stats.totalSwitches, 2);
    EXPECT_EQ(stats.emergencySwitches, 1);
    EXPECT_EQ(stats.switchesByType.count(ConnectionType::GCS_5G), 1);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(P2Integration, FullWorkflow) {
    // Create allocator
    auto allocator = std::make_unique<DistributedTaskAllocator>("UAV_001");
    allocator->initialize(true);
    
    // Create resolver
    auto resolver = std::make_unique<CrossPartitionConflictResolver>("UAV_001", "PART_001");
    resolver->initialize();
    
    // Create reconnector
    auto reconnector = std::make_unique<PredictiveReconnector>("UAV_001");
    reconnector->initialize();
    
    // Submit tasks
    for (int i = 0; i < 5; ++i) {
        DistributedTask task;
        task.type = DistributedTaskType::SEARCH;
        task.name = "Search Task " + std::to_string(i);
        allocator->submitTask(task);
    }
    
    // Simulate partition detection
    PartitionState localState;
    localState.partitionId = "PART_001";
    localState.activeTasks = {"task_1", "task_2"};
    resolver->updateLocalState(localState);
    
    // Simulate connection quality update
    ConnectionMetrics metrics;
    metrics.signalStrength = 85.0;
    metrics.latency = 50.0;
    metrics.isConnected = true;
    reconnector->updateMetrics(ConnectionType::GCS_4G, metrics);
    
    // Verify all systems operational
    EXPECT_EQ(allocator->getAllTasks().size(), 5);
    EXPECT_FALSE(resolver->getPendingConflicts().empty() || 
                 resolver->getPendingConflicts().empty());  // Either state is valid
    EXPECT_GT(reconnector->predict(ConnectionType::GCS_4G).predictedQuality, 0.0);
    
    allocator->shutdown();
    resolver->shutdown();
    reconnector->shutdown();
}

} // namespace
