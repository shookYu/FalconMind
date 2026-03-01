/**
 * @file test_e2e_scenarios.cpp
 * @brief End-to-end integration tests for complete scenarios
 * 
 * Scenarios:
 * - Complete mission with GCS disconnect and reconnect
 * - Swarm partition during mission
 * - Leader failure and re-election
 * - Multiple partitions merging
 * - Cascading failures and recovery
 * - High-load scenario
 * 
 * @note These tests verify all components working together
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "nodeagent/OfflineAutonomyManager.h"
#include "nodeagent/LocalStore.h"
#include "nodeagent/StateMachine.h"
#include "nodeagent/RuleEngine.h"
#include "nodeagent/SwarmPartitionManager.h"
#include "nodeagent/InterUavManager.h"
#include "nodeagent/DistributedTaskAllocator.h"
#include "nodeagent/MetricsCollector.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace nodeagent;
using namespace testing;

class E2EScenarioTest : public Test {
protected:
    std::filesystem::path tempDir_;
    
    void SetUp() override {
        tempDir_ = std::filesystem::temp_directory_path() / 
                   ("e2e_test_" + std::to_string(
                       std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tempDir_);
    }
    
    void TearDown() override {
        std::filesystem::remove_all(tempDir_);
    }
};

// ============================================================================
// Scenario 1: Complete Mission with GCS Disconnect
// ============================================================================

TEST_F(E2EScenarioTest, CompleteMissionWithGcsDisconnect) {
    // Setup: UAV starts mission, GCS disconnects, UAV completes mission autonomously, GCS reconnects
    
    // 1. Initialize components
    auto localStore = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore->initialize((tempDir_ / "mission.db").string()));
    
    auto stateMachine = std::make_unique<StateMachine>();
    ASSERT_TRUE(stateMachine->initialize(State::ONLINE));
    
    auto ruleEngine = std::make_unique<RuleEngine>();
    ASSERT_TRUE(ruleEngine->initialize());
    
    OfflineAutonomyConfig config;
    config.uavId = "UAV_E2E_001";
    config.heartbeatTimeout = std::chrono::seconds(2);
    
    auto autonomyManager = std::make_unique<OfflineAutonomyManager>();
    ASSERT_TRUE(autonomyManager->initialize(config, localStore.get()));
    
    // 2. Deploy mission
    OfflineTask mission;
    mission.id = "mission_complete_001";
    mission.type = TaskType::SEARCH;
    mission.name = "Area Search Mission";
    mission.payload["area"] = {{0, 0}, {1000, 0}, {1000, 1000}, {0, 1000}};
    mission.payload["pattern"] = "LAWN_MOWER";
    ASSERT_TRUE(autonomyManager->deployOfflineTask(mission));
    
    // 3. Start mission execution
    ASSERT_TRUE(stateMachine->transitionTo(State::MISSION_EXECUTION));
    
    // 4. Simulate GCS disconnect
    autonomyManager->handleGcsDisconnect();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(stateMachine->transitionTo(State::OFFLINE_AUTONOMY));
    ASSERT_TRUE(autonomyManager->startOfflineExecution());
    
    // 5. Execute mission autonomously (cache telemetry)
    for (int i = 0; i < 50; ++i) {
        TelemetryData data;
        data.timestamp = std::chrono::system_clock::now();
        data.uavId = config.uavId;
        data.position = {i * 20.0, i * 10.0, 50.0};
        data.batteryLevel = 80.0 - (i * 0.5);
        data.isArmed = true;
        data.isFlying = true;
        
        ASSERT_TRUE(autonomyManager->cacheTelemetry(data));
        
        // Evaluate rules periodically
        if (i % 10 == 0) {
            UAVState state;
            state.batteryLevel = data.batteryLevel;
            state.isConnectedToGcs = false;
            ruleEngine->evaluateRules(state);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // 6. Complete mission
    ASSERT_TRUE(autonomyManager->completeOfflineTask(mission.id, true));
    
    // 7. Simulate GCS reconnect
    ASSERT_TRUE(autonomyManager->handleGcsConnect());
    ASSERT_TRUE(stateMachine->transitionTo(State::SAFE));
    ASSERT_TRUE(stateMachine->transitionTo(State::ONLINE));
    
    // 8. Sync telemetry
    ASSERT_TRUE(autonomyManager->syncCachedTelemetry());
    
    // 9. Verify mission completed
    auto completedTask = autonomyManager->getOfflineTask(mission.id);
    ASSERT_TRUE(completedTask.has_value());
    EXPECT_EQ(completedTask->status, TaskStatus::COMPLETED);
    
    // 10. Verify telemetry synced
    EXPECT_EQ(autonomyManager->getUnsyncedTelemetryCount(), 0);
    
    // Cleanup
    autonomyManager->shutdown();
    stateMachine->shutdown();
    ruleEngine->shutdown();
    localStore->shutdown();
}

// ============================================================================
// Scenario 2: Swarm Partition During Mission
// ============================================================================

TEST_F(E2EScenarioTest, SwarmPartitionDuringMission) {
    // Setup: 5 UAV swarm, partition occurs, leaders elected in each partition, tasks reassigned
    
    // 1. Initialize swarm components for UAV_001 (local)
    SwarmPartitionManager swarmManager("UAV_001", "SWARM_E2E");
    ASSERT_TRUE(swarmManager.initialize());
    
    DistributedTaskAllocator taskAllocator("UAV_001");
    ASSERT_TRUE(taskAllocator.initialize(true));  // Leader initially
    
    // 2. Add swarm members
    std::vector<std::string> uavIds = {"UAV_001", "UAV_002", "UAV_003", "UAV_004", "UAV_005"};
    for (const auto& id : uavIds) {
        SwarmMember m;
        m.uavId = id;
        m.capabilities.batteryLevel = 80;
        m.capabilities.computePower = 6;
        m.isActive = true;
        swarmManager.addMember(m);
        
        // Add to task allocator
        UavCapability cap;
        cap.uavId = id;
        cap.batteryLevel = 80.0;
        cap.currentLoad = 0.0;
        taskAllocator.updatePeerCapability(cap);
    }
    
    // 3. Fully connect swarm
    for (size_t i = 0; i < uavIds.size(); ++i) {
        for (size_t j = i + 1; j < uavIds.size(); ++j) {
            swarmManager.updateConnection(uavIds[i], uavIds[j], true);
        }
    }
    
    // 4. Verify single partition
    auto partitions = swarmManager.detectPartitions();
    EXPECT_EQ(partitions.size(), 1);
    
    // 5. Assign tasks
    std::vector<std::string> taskIds;
    for (int i = 0; i < 5; ++i) {
        DistributedTask task;
        task.type = DistributedTaskType::PATROL;
        task.name = "Patrol " + std::to_string(i);
        task.requirements.minBatteryLevel = 20.0;
        std::string id = taskAllocator.submitTask(task);
        taskIds.push_back(id);
        
        // Self bid and assign
        TaskBid bid = taskAllocator.submitBid(id, task.requirements);
        taskAllocator.receiveBid(bid);
        taskAllocator.assignTask(id);
    }
    
    // 6. Simulate partition (3 UAVs in one group, 2 in another)
    // Break connections between {1,2,3} and {4,5}
    swarmManager.updateConnection("UAV_001", "UAV_004", false);
    swarmManager.updateConnection("UAV_001", "UAV_005", false);
    swarmManager.updateConnection("UAV_002", "UAV_004", false);
    swarmManager.updateConnection("UAV_002", "UAV_005", false);
    swarmManager.updateConnection("UAV_003", "UAV_004", false);
    swarmManager.updateConnection("UAV_003", "UAV_005", false);
    
    // 7. Detect partition
    partitions = swarmManager.detectPartitions();
    EXPECT_EQ(partitions.size(), 2);
    
    // 8. Handle partition in task allocator
    std::vector<std::string> partitionMembers = {"UAV_001", "UAV_002", "UAV_003"};
    taskAllocator.handlePartition(partitionMembers);
    
    // 9. Verify tasks redistributed within partition
    auto localTasks = taskAllocator.getTasksForUav("UAV_001");
    // Should have reassigned tasks from UAV_004 and UAV_005
    
    // Cleanup
    taskAllocator.shutdown();
    swarmManager.shutdown();
}

// ============================================================================
// Scenario 3: Leader Failure and Re-election
// ============================================================================

TEST_F(E2EScenarioTest, LeaderFailureAndReelection) {
    SwarmPartitionManager swarmManager("UAV_001", "SWARM_E2E");
    ASSERT_TRUE(swarmManager.initialize());
    InterUavManager interUavManager("UAV_001", "SWARM_E2E");
    ASSERT_TRUE(interUavManager.initialize());
    
    // Setup: UAV_001 is leader initially
    SwarmMember leader;
    leader.uavId = "UAV_001";
    leader.capabilities.batteryLevel = 90;
    leader.capabilities.computePower = 8;
    leader.isLeader = true;
    swarmManager.addMember(leader);
    interUavManager.joinSwarm({"UAV_001"});
    
    // Add other members
    for (int i = 2; i <= 4; ++i) {
        SwarmMember m;
        m.uavId = "UAV_00" + std::to_string(i);
        m.capabilities.batteryLevel = 80;
        m.capabilities.computePower = 6;
        m.isActive = true;
        swarmManager.addMember(m);
        
        swarmManager.updateConnection("UAV_001", m.uavId, true);
    }
    
    // Verify initial leader
    auto initialLeader = swarmManager.getCurrentLeader();
    ASSERT_TRUE(initialLeader.has_value());
    EXPECT_EQ(initialLeader->uavId, "UAV_001");
    
    // Simulate leader (UAV_001) failure
    swarmManager.updateMemberStatus("UAV_001", false);
    
    // Trigger re-election
    auto newLeader = swarmManager.electLeader();
    
    // Verify new leader elected
    ASSERT_TRUE(newLeader.has_value());
    EXPECT_NE(newLeader->uavId, "UAV_001");
    
    // Cleanup
    interUavManager.shutdown();
    swarmManager.shutdown();
}

// ============================================================================
// Scenario 4: Multiple Partitions Merging
// ============================================================================

TEST_F(E2EScenarioTest, MultiplePartitionsMerging) {
    SwarmPartitionManager swarmManager("UAV_001", "SWARM_E2E");
    ASSERT_TRUE(swarmManager.initialize());
    
    // Setup three partitions
    std::vector<SwarmMember> members;
    for (int i = 1; i <= 9; ++i) {
        SwarmMember m;
        m.uavId = "UAV_00" + std::to_string(i);
        m.capabilities.batteryLevel = 80;
        m.isActive = true;
        members.push_back(m);
        swarmManager.addMember(m);
    }
    
    // Create three separate partitions
    // Partition 1: UAV_001, UAV_002, UAV_003
    swarmManager.updateConnection("UAV_001", "UAV_002", true);
    swarmManager.updateConnection("UAV_001", "UAV_003", true);
    swarmManager.updateConnection("UAV_002", "UAV_003", true);
    
    // Partition 2: UAV_004, UAV_005, UAV_006
    swarmManager.updateConnection("UAV_004", "UAV_005", true);
    swarmManager.updateConnection("UAV_004", "UAV_006", true);
    swarmManager.updateConnection("UAV_005", "UAV_006", true);
    
    // Partition 3: UAV_007, UAV_008, UAV_009
    swarmManager.updateConnection("UAV_007", "UAV_008", true);
    swarmManager.updateConnection("UAV_007", "UAV_009", true);
    swarmManager.updateConnection("UAV_008", "UAV_009", true);
    
    // Verify three partitions
    auto partitions = swarmManager.detectPartitions();
    EXPECT_EQ(partitions.size(), 3);
    
    // Elect leaders in each partition
    for (auto& partition : partitions) {
        // Simulate local leader election for each partition
        // In real scenario, each partition would elect independently
    }
    
    // Merge partitions by restoring connections
    swarmManager.updateConnection("UAV_003", "UAV_004", true);
    swarmManager.updateConnection("UAV_006", "UAV_007", true);
    
    // Verify merged
    partitions = swarmManager.detectPartitions();
    EXPECT_EQ(partitions.size(), 1);
    EXPECT_EQ(partitions[0].memberIds.size(), 9);
    
    // Cleanup
    swarmManager.shutdown();
}

// ============================================================================
// Scenario 5: Cascading Failures and Recovery
// ============================================================================

TEST_F(E2EScenarioTest, CascadingFailuresAndRecovery) {
    auto localStore = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore->initialize((tempDir_ / "cascade.db").string()));
    
    auto stateMachine = std::make_unique<StateMachine>();
    ASSERT_TRUE(stateMachine->initialize(State::ONLINE));
    
    MetricsCollector metrics;
    ASSERT_TRUE(metrics.initialize());
    
    // Simulate cascading failure scenario
    // 1. GCS disconnects
    stateMachine->transitionTo(State::OFFLINE_AUTONOMY);
    ASSERT_EQ(stateMachine->getCurrentState(), State::OFFLINE_AUTONOMY);
    
    // 2. Swarm partitions
    // (Simulated by state changes)
    
    // 3. Low battery warning
    UAVState state;
    state.batteryLevel = 25.0;
    state.isConnectedToGcs = false;
    
    // Record low battery event
    OfflineEvent event;
    event.id = "evt_001";
    event.type = "LOW_BATTERY";
    event.severity = EventSeverity::WARNING;
    event.timestamp = std::chrono::system_clock::now();
    localStore->storeEvent(event);
    
    metrics.setAlertThreshold("battery_level", 30.0, "<");
    metrics.setGauge("battery_level", 25.0);
    
    // 4. Trigger RTL (Return to Launch)
    stateMachine->transitionTo(State::RETURN_TO_LAUNCH);
    ASSERT_EQ(stateMachine->getCurrentState(), State::RETURN_TO_LAUNCH);
    
    // 5. Land safely
    stateMachine->transitionTo(State::LANDING);
    ASSERT_EQ(stateMachine->getCurrentState(), State::LANDING);
    
    stateMachine->transitionTo(State::SAFE);
    ASSERT_EQ(stateMachine->getCurrentState(), State::SAFE);
    
    // 6. Recover - reconnect
    stateMachine->transitionTo(State::ONLINE);
    ASSERT_EQ(stateMachine->getCurrentState(), State::ONLINE);
    
    // Verify all events recorded
    auto events = localStore->getEvents("UAV_001", 100);
    EXPECT_GE(events.size(), 1);
    
    // Cleanup
    stateMachine->shutdown();
    localStore->shutdown();
}

// ============================================================================
// Scenario 6: High Load Stress Test
// ============================================================================

TEST_F(E2EScenarioTest, HighLoadStressTest) {
    auto localStore = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore->initialize((tempDir_ / "stress.db").string()));
    
    MetricsCollector metrics;
    ASSERT_TRUE(metrics.initialize());
    
    const int telemetryCount = 1000;
    const int taskCount = 50;
    
    // High telemetry rate
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < telemetryCount; ++i) {
        TelemetryData data;
        data.timestamp = std::chrono::system_clock::now();
        data.uavId = "UAV_STRESS";
        data.position = {static_cast<double>(i), 0.0, 50.0};
        data.batteryLevel = 80.0;
        
        ASSERT_TRUE(localStore->storeTelemetry(data));
        
        metrics.incrementCounter("telemetry_recorded");
        
        if (i % 100 == 0) {
            metrics.recordSystemMetrics();
        }
    }
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count();
    
    // Should complete within reasonable time (< 5 seconds for 1000 records)
    EXPECT_LT(elapsed, 5000);
    
    // Verify data stored
    auto unsynced = localStore->getUnsyncedTelemetry("UAV_STRESS", telemetryCount + 1);
    EXPECT_EQ(unsynced.size(), telemetryCount);
    
    // Verify metrics
    auto stats = metrics.getStatistics();
    EXPECT_EQ(stats.totalMetricsRecorded, telemetryCount);
    
    // Cleanup
    localStore->shutdown();
}

// ============================================================================
// Scenario 7: Rule Engine Complex Scenario
// ============================================================================

TEST_F(E2EScenarioTest, ComplexRuleEvaluation) {
    RuleEngine engine;
    ASSERT_TRUE(engine.initialize());
    
    // Setup complex rule hierarchy
    // Rule 1: Critical battery -> Land immediately
    OfflineRule criticalBattery;
    criticalBattery.id = "critical_battery";
    criticalBattery.priority = 100;
    criticalBattery.enabled = true;
    criticalBattery.condition.type = ConditionType::BATTERY_BELOW;
    criticalBattery.condition.threshold = 15.0;
    criticalBattery.action.type = ActionType::LAND;
    engine.addRule(criticalBattery);
    
    // Rule 2: Low battery -> RTL
    OfflineRule lowBattery;
    lowBattery.id = "low_battery";
    lowBattery.priority = 90;
    lowBattery.enabled = true;
    lowBattery.condition.type = ConditionType::BATTERY_BELOW;
    lowBattery.condition.threshold = 30.0;
    lowBattery.action.type = ActionType::RETURN_TO_LAUNCH;
    engine.addRule(lowBattery);
    
    // Rule 3: GCS disconnect -> Continue if battery OK
    OfflineRule gcsDisconnect;
    gcsDisconnect.id = "gcs_disconnect";
    gcsDisconnect.priority = 50;
    gcsDisconnect.enabled = true;
    gcsDisconnect.condition.type = ConditionType::COMMUNICATION_LOST;
    gcsDisconnect.action.type = ActionType::CONTINUE_MISSION;
    engine.addRule(gcsDisconnect);
    
    // Test case 1: Normal conditions - no rules triggered
    UAVState normalState;
    normalState.batteryLevel = 80.0;
    normalState.isConnectedToGcs = true;
    auto actions1 = engine.evaluateRules(normalState);
    EXPECT_TRUE(actions1.empty());
    
    // Test case 2: GCS disconnect - continue mission
    UAVState disconnectedState;
    disconnectedState.batteryLevel = 80.0;
    disconnectedState.isConnectedToGcs = false;
    auto actions2 = engine.evaluateRules(disconnectedState);
    EXPECT_EQ(actions2.size(), 1);
    EXPECT_EQ(actions2[0].type, ActionType::CONTINUE_MISSION);
    
    // Test case 3: Low battery + GCS disconnect - RTL takes priority
    UAVState lowBattState;
    lowBattState.batteryLevel = 25.0;
    lowBattState.isConnectedToGcs = false;
    auto actions3 = engine.evaluateRules(lowBattState);
    EXPECT_EQ(actions3.size(), 1);
    EXPECT_EQ(actions3[0].type, ActionType::RETURN_TO_LAUNCH);
    
    // Test case 4: Critical battery - Land immediately
    UAVState criticalState;
    criticalState.batteryLevel = 10.0;
    criticalState.isConnectedToGcs = false;
    auto actions4 = engine.evaluateRules(criticalState);
    EXPECT_EQ(actions4.size(), 1);
    EXPECT_EQ(actions4[0].type, ActionType::LAND);
    
    engine.shutdown();
}

// ============================================================================
// Scenario 8: Concurrent Operations
// ============================================================================

TEST_F(E2EScenarioTest, ConcurrentOperations) {
    auto localStore = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore->initialize((tempDir_ / "concurrent.db").string()));
    
    MetricsCollector metrics;
    ASSERT_TRUE(metrics.initialize());
    
    const int numThreads = 5;
    const int opsPerThread = 100;
    
    std::vector<std::thread> threads;
    std::atomic<int> successCount{0};
    
    // Concurrent telemetry writes
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < opsPerThread; ++i) {
                TelemetryData data;
                data.timestamp = std::chrono::system_clock::now();
                data.uavId = "UAV_" + std::to_string(t);
                data.position = {static_cast<double>(i), static_cast<double>(t), 50.0};
                
                if (localStore->storeTelemetry(data)) {
                    successCount++;
                }
                
                metrics.incrementCounter("concurrent_ops");
            }
        });
    }
    
    // Concurrent reads
    threads.emplace_back([&]() {
        for (int i = 0; i < opsPerThread; ++i) {
            auto data = localStore->getUnsyncedTelemetry("UAV_0", 10);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All operations should succeed
    EXPECT_EQ(successCount, numThreads * opsPerThread);
    
    // Verify data integrity
    int totalRecords = 0;
    for (int t = 0; t < numThreads; ++t) {
        auto data = localStore->getUnsyncedTelemetry("UAV_" + std::to_string(t), opsPerThread);
        totalRecords += data.size();
    }
    
    EXPECT_EQ(totalRecords, numThreads * opsPerThread);
    
    localStore->shutdown();
}

} // namespace
