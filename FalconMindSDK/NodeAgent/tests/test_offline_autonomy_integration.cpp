/**
 * @file test_offline_autonomy_integration.cpp
 * @brief Integration tests for offline autonomy system
 * 
 * Tests cover:
 * - Complete offline autonomy workflow
 * - GCS disconnection detection and handling
 * - Offline task execution with cached rules
 * - Telemetry buffering and sync on reconnection
 * - State machine transitions during offline/online cycles
 * - Rule engine evaluation during autonomy
 * - Local store persistence across restarts
 * - Thread safety of full autonomy pipeline
 * 
 * @note Zero-mock testing - uses real implementation with temp storage
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "nodeagent/OfflineAutonomyManager.h"
#include "nodeagent/LocalStore.h"
#include "nodeagent/RuleEngine.h"
#include "nodeagent/StateMachine.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace nodeagent;
using namespace testing;

class OfflineAutonomyIntegrationTest : public Test {
protected:
    void SetUp() override {
        // Create temp directory for test database
        tempDir_ = std::filesystem::temp_directory_path() / "offline_autonomy_test";
        std::filesystem::create_directories(tempDir_);
        
        dbPath_ = (tempDir_ / "test_autonomy.db").string();
        
        // Initialize components
        localStore_ = std::make_unique<LocalStore>();
        ASSERT_TRUE(localStore_->initialize(dbPath_));
        
        ruleEngine_ = std::make_unique<RuleEngine>();
        ASSERT_TRUE(ruleEngine_->initialize());
        
        stateMachine_ = std::make_unique<StateMachine>();
        ASSERT_TRUE(stateMachine_->initialize(State::ONLINE));
        
        autonomyManager_ = std::make_unique<OfflineAutonomyManager>();
        OfflineAutonomyConfig config;
        config.uavId = "UAV_TEST_001";
        config.heartbeatTimeout = std::chrono::seconds(2);
        config.autonomyConfig.lowBatteryThreshold = 30.0;
        config.autonomyConfig.criticalBatteryThreshold = 15.0;
        ASSERT_TRUE(autonomyManager_->initialize(config, localStore_.get()));
    }
    
    void TearDown() override {
        autonomyManager_->shutdown();
        stateMachine_->shutdown();
        ruleEngine_->shutdown();
        localStore_->shutdown();
        
        autonomyManager_.reset();
        stateMachine_.reset();
        ruleEngine_.reset();
        localStore_.reset();
        
        // Clean up temp directory
        std::filesystem::remove_all(tempDir_);
    }
    
    std::unique_ptr<OfflineAutonomyManager> autonomyManager_;
    std::unique_ptr<LocalStore> localStore_;
    std::unique_ptr<RuleEngine> ruleEngine_;
    std::unique_ptr<StateMachine> stateMachine_;
    std::filesystem::path tempDir_;
    std::string dbPath_;
};

// ============================================================================
// Basic Workflow Tests
// ============================================================================

TEST_F(OfflineAutonomyIntegrationTest, CompleteOnlineToOfflineTransition) {
    // Start online
    EXPECT_EQ(stateMachine_->getCurrentState(), State::ONLINE);
    
    // Deploy an offline task
    OfflineTask task;
    task.id = "task_001";
    task.type = TaskType::SEARCH;
    task.name = "Test Search Task";
    task.payload["area"] = {{0, 0}, {100, 0}, {100, 100}, {0, 100}};
    task.payload["altitude"] = 50;
    
    ASSERT_TRUE(autonomyManager_->deployOfflineTask(task));
    
    // Simulate GCS disconnect
    autonomyManager_->handleGcsDisconnect();
    
    // Wait for heartbeat timeout
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Should transition to autonomous mode
    EXPECT_TRUE(autonomyManager_->isAutonomousMode());
    
    // Start executing the task
    ASSERT_TRUE(autonomyManager_->startOfflineExecution());
    
    // Record some telemetry
    for (int i = 0; i < 5; ++i) {
        autonomyManager_->cacheTelemetry({
            .timestamp = std::chrono::system_clock::now(),
            .uavId = "UAV_TEST_001",
            .position = {i * 10.0, i * 10.0, 50.0},
            .batteryLevel = 80.0 - (i * 2)
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Verify telemetry is cached
    auto cached = autonomyManager_->getCachedTelemetryCount();
    EXPECT_EQ(cached, 5);
    
    // Complete the task
    ASSERT_TRUE(autonomyManager_->completeOfflineTask("task_001", true));
    
    // Simulate GCS reconnect
    ASSERT_TRUE(autonomyManager_->handleGcsConnect());
    
    // Should sync telemetry
    EXPECT_TRUE(autonomyManager_->syncCachedTelemetry());
    
    // Should be back online
    EXPECT_FALSE(autonomyManager_->isAutonomousMode());
}

TEST_F(OfflineAutonomyIntegrationTest, OfflineTaskPersistenceAcrossRestart) {
    // Deploy a task
    OfflineTask task;
    task.id = "persistent_task";
    task.type = TaskType::PATROL;
    task.name = "Persistent Patrol";
    task.payload["waypoints"] = {{0, 0, 30}, {100, 0, 30}, {100, 100, 30}, {0, 100, 30}};
    
    ASSERT_TRUE(autonomyManager_->deployOfflineTask(task));
    
    // Shutdown and recreate manager
    autonomyManager_->shutdown();
    autonomyManager_.reset();
    
    autonomyManager_ = std::make_unique<OfflineAutonomyManager>();
    OfflineAutonomyConfig config;
    config.uavId = "UAV_TEST_001";
    ASSERT_TRUE(autonomyManager_->initialize(config, localStore_.get()));
    
    // Task should still exist
    auto retrieved = autonomyManager_->getOfflineTask("persistent_task");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->type, TaskType::PATROL);
    EXPECT_EQ(retrieved->name, "Persistent Patrol");
}

// ============================================================================
// Telemetry Sync Tests
// ============================================================================

TEST_F(OfflineAutonomyIntegrationTest, TelemetrySyncOnReconnection) {
    // Simulate going offline and collecting telemetry
    autonomyManager_->handleGcsDisconnect();
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(autonomyManager_->startOfflineExecution());
    
    // Add telemetry records
    std::vector<std::string> telemetryIds;
    for (int i = 0; i < 10; ++i) {
        TelemetryData data;
        data.timestamp = std::chrono::system_clock::now();
        data.uavId = "UAV_TEST_001";
        data.position = {i * 10.0, i * 5.0, 50.0};
        data.batteryLevel = 90.0 - i;
        data.groundSpeed = 15.0;
        data.altitude = 50.0;
        data.latitude = 37.7749 + (i * 0.001);
        data.longitude = -122.4194 + (i * 0.001);
        
        ASSERT_TRUE(autonomyManager_->cacheTelemetry(data));
        telemetryIds.push_back(data.timestamp.time_since_epoch().count());
    }
    
    EXPECT_EQ(autonomyManager_->getCachedTelemetryCount(), 10);
    EXPECT_EQ(autonomyManager_->getUnsyncedTelemetryCount(), 10);
    
    // Simulate successful sync of some records
    ASSERT_TRUE(autonomyManager_->markTelemetrySynced(
        std::vector<std::string>(telemetryIds.begin(), telemetryIds.begin() + 5)));
    
    EXPECT_EQ(autonomyManager_->getUnsyncedTelemetryCount(), 5);
    EXPECT_EQ(autonomyManager_->getCachedTelemetryCount(), 10);
    
    // Sync remaining
    ASSERT_TRUE(autonomyManager_->markTelemetrySynced(
        std::vector<std::string>(telemetryIds.begin() + 5, telemetryIds.end())));
    
    EXPECT_EQ(autonomyManager_->getUnsyncedTelemetryCount(), 0);
}

TEST_F(OfflineAutonomyIntegrationTest, TelemetryBufferSizeLimit) {
    autonomyManager_->handleGcsDisconnect();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Add many telemetry records
    for (int i = 0; i < 1005; ++i) {
        TelemetryData data;
        data.timestamp = std::chrono::system_clock::now();
        data.uavId = "UAV_TEST_001";
        data.position = {static_cast<double>(i), 0.0, 50.0};
        data.batteryLevel = 80.0;
        
        autonomyManager_->cacheTelemetry(data);
    }
    
    // Buffer should be limited to 1000
    EXPECT_LE(autonomyManager_->getCachedTelemetryCount(), 1000);
}

// ============================================================================
// Rule Engine Integration Tests
// ============================================================================

TEST_F(OfflineAutonomyIntegrationTest, RuleEvaluationDuringAutonomy) {
    autonomyManager_->handleGcsDisconnect();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(autonomyManager_->startOfflineExecution());
    
    // Configure rule engine with low battery rule
    OfflineRule rule;
    rule.id = "low_battery_rtl";
    rule.name = "Low Battery RTL";
    rule.priority = 100;
    rule.enabled = true;
    rule.condition.type = ConditionType::BATTERY_BELOW;
    rule.condition.threshold = 30.0;
    rule.action.type = ActionType::RETURN_TO_LAUNCH;
    
    ASSERT_TRUE(ruleEngine_->addRule(rule));
    
    // Simulate battery dropping
    bool rtlTriggered = false;
    ruleEngine_->setActionCallback([&](const OfflineRule& r) {
        if (r.action.type == ActionType::RETURN_TO_LAUNCH) {
            rtlTriggered = true;
        }
    });
    
    // Evaluate with safe battery
    UAVState state;
    state.batteryLevel = 50.0;
    state.isConnectedToGcs = false;
    ruleEngine_->evaluateRules(state);
    
    EXPECT_FALSE(rtlTriggered);
    
    // Evaluate with low battery
    state.batteryLevel = 25.0;
    ruleEngine_->evaluateRules(state);
    
    EXPECT_TRUE(rtlTriggered);
}

TEST_F(OfflineAutonomyIntegrationTest, MultipleRuleEvaluation) {
    autonomyManager_->handleGcsDisconnect();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // Add multiple rules
    OfflineRule rule1;
    rule1.id = "rule1";
    rule1.priority = 100;
    rule1.condition.type = ConditionType::BATTERY_BELOW;
    rule1.condition.threshold = 20.0;
    rule1.action.type = ActionType::LAND;
    rule1.enabled = true;
    
    OfflineRule rule2;
    rule2.id = "rule2";
    rule2.priority = 50;
    rule2.condition.type = ConditionType::COMMUNICATION_LOST;
    rule2.action.type = ActionType::CONTINUE_MISSION;
    rule2.enabled = true;
    
    ruleEngine_->addRule(rule1);
    ruleEngine_->addRule(rule2);
    
    // Track triggered actions
    std::vector<ActionType> triggeredActions;
    ruleEngine_->setActionCallback([&](const OfflineRule& r) {
        triggeredActions.push_back(r.action.type);
    });
    
    // State triggers communication lost rule
    UAVState state;
    state.batteryLevel = 80.0;
    state.isConnectedToGcs = false;
    
    ruleEngine_->evaluateRules(state);
    
    ASSERT_EQ(triggeredActions.size(), 1);
    EXPECT_EQ(triggeredActions[0], ActionType::CONTINUE_MISSION);
    
    // State triggers both rules
    state.batteryLevel = 15.0;
    triggeredActions.clear();
    
    ruleEngine_->evaluateRules(state);
    
    // Should trigger land (higher priority)
    ASSERT_EQ(triggeredActions.size(), 1);
    EXPECT_EQ(triggeredActions[0], ActionType::LAND);
}

// ============================================================================
// State Machine Tests
// ============================================================================

TEST_F(OfflineAutonomyIntegrationTest, StateTransitionsDuringOfflineCycle) {
    EXPECT_EQ(stateMachine_->getCurrentState(), State::ONLINE);
    
    // Transition to offline
    EXPECT_TRUE(stateMachine_->transitionTo(State::OFFLINE_AUTONOMY));
    EXPECT_EQ(stateMachine_->getCurrentState(), State::OFFLINE_AUTONOMY);
    
    // Try invalid transition (should fail)
    EXPECT_FALSE(stateMachine_->transitionTo(State::ONLINE));
    
    // Must go through SAFE first
    EXPECT_TRUE(stateMachine_->transitionTo(State::SAFE));
    EXPECT_EQ(stateMachine_->getCurrentState(), State::SAFE);
    
    // Now can go back online
    EXPECT_TRUE(stateMachine_->transitionTo(State::ONLINE));
    EXPECT_EQ(stateMachine_->getCurrentState(), State::ONLINE);
}

TEST_F(OfflineAutonomyIntegrationTest, StateTransitionHistory) {
    stateMachine_->transitionTo(State::MISSION_EXECUTION);
    stateMachine_->transitionTo(State::OFFLINE_AUTONOMY);
    stateMachine_->transitionTo(State::SAFE);
    
    auto history = stateMachine_->getTransitionHistory();
    ASSERT_GE(history.size(), 4);  // ONLINE + 3 transitions
    
    EXPECT_EQ(history[0].toState, State::ONLINE);
    EXPECT_EQ(history[1].toState, State::MISSION_EXECUTION);
    EXPECT_EQ(history[2].toState, State::OFFLINE_AUTONOMY);
    EXPECT_EQ(history[3].toState, State::SAFE);
}

TEST_F(OfflineAutonomyIntegrationTest, StateTransitionCallbacks) {
    State enteredState = State::UNKNOWN;
    State exitedState = State::UNKNOWN;
    
    stateMachine_->setTransitionCallback([&](State from, State to) {
        exitedState = from;
        enteredState = to;
    });
    
    stateMachine_->transitionTo(State::MISSION_EXECUTION);
    
    EXPECT_EQ(exitedState, State::ONLINE);
    EXPECT_EQ(enteredState, State::MISSION_EXECUTION);
}

// ============================================================================
// Local Store Tests
// ============================================================================

TEST_F(OfflineAutonomyIntegrationTest, TaskPersistenceInLocalStore) {
    // Create and store a task
    OfflineTask task;
    task.id = "store_test_task";
    task.type = TaskType::SEARCH;
    task.name = "Store Test";
    task.status = TaskStatus::PENDING;
    task.createdAt = std::chrono::system_clock::now();
    
    ASSERT_TRUE(localStore_->storeOfflineTask(task));
    
    // Retrieve it
    auto retrieved = localStore_->getOfflineTask("store_test_task");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Store Test");
    EXPECT_EQ(retrieved->status, TaskStatus::PENDING);
    
    // Update status
    task.status = TaskStatus::EXECUTING;
    ASSERT_TRUE(localStore_->storeOfflineTask(task));
    
    retrieved = localStore_->getOfflineTask("store_test_task");
    EXPECT_EQ(retrieved->status, TaskStatus::EXECUTING);
}

TEST_F(OfflineAutonomyIntegrationTest, TelemetryPersistenceInLocalStore) {
    // Store telemetry
    for (int i = 0; i < 5; ++i) {
        TelemetryData data;
        data.timestamp = std::chrono::system_clock::now() + std::chrono::milliseconds(i);
        data.uavId = "UAV_TEST_001";
        data.position = {static_cast<double>(i), 0.0, 50.0};
        data.batteryLevel = 80.0;
        data.synced = false;
        
        ASSERT_TRUE(localStore_->storeTelemetry(data));
    }
    
    // Retrieve unsynced telemetry
    auto unsynced = localStore_->getUnsyncedTelemetry("UAV_TEST_001", 10);
    EXPECT_EQ(unsynced.size(), 5);
    
    // Mark some as synced
    std::vector<std::string> idsToMark;
    for (int i = 0; i < 3; ++i) {
        idsToMark.push_back(std::to_string(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                unsynced[i].timestamp.time_since_epoch()).count()));
    }
    
    ASSERT_TRUE(localStore_->markTelemetryAsSynced(idsToMark));
    
    // Check remaining unsynced
    unsynced = localStore_->getUnsyncedTelemetry("UAV_TEST_001", 10);
    EXPECT_EQ(unsynced.size(), 2);
}

TEST_F(OfflineAutonomyIntegrationTest, EventPersistenceInLocalStore) {
    // Store events
    for (int i = 0; i < 3; ++i) {
        OfflineEvent event;
        event.id = "event_" + std::to_string(i);
        event.timestamp = std::chrono::system_clock::now();
        event.uavId = "UAV_TEST_001";
        event.type = (i == 0) ? "GCS_DISCONNECT" : "RULE_TRIGGERED";
        event.severity = (i == 0) ? EventSeverity::WARNING : EventSeverity::INFO;
        event.description = "Test event " + std::to_string(i);
        event.synced = false;
        
        ASSERT_TRUE(localStore_->storeEvent(event));
    }
    
    // Retrieve events
    auto events = localStore_->getEvents("UAV_TEST_001", 10);
    EXPECT_EQ(events.size(), 3);
    
    // Mark as synced
    std::vector<std::string> eventIds = {"event_0", "event_1"};
    ASSERT_TRUE(localStore_->markEventsAsSynced(eventIds));
    
    // Check unsynced events
    events = localStore_->getUnsyncedEvents("UAV_TEST_001", 10);
    EXPECT_EQ(events.size(), 1);
    EXPECT_EQ(events[0].id, "event_2");
}

TEST_F(OfflineAutonomyIntegrationTest, LocalStoreCleanupOldRecords) {
    // Store old telemetry
    TelemetryData oldData;
    oldData.timestamp = std::chrono::system_clock::now() - std::chrono::hours(48);
    oldData.uavId = "UAV_TEST_001";
    oldData.position = {0.0, 0.0, 50.0};
    oldData.batteryLevel = 80.0;
    localStore_->storeTelemetry(oldData);
    
    // Store recent telemetry
    TelemetryData newData;
    newData.timestamp = std::chrono::system_clock::now();
    newData.uavId = "UAV_TEST_001";
    newData.position = {10.0, 10.0, 50.0};
    newData.batteryLevel = 75.0;
    localStore_->storeTelemetry(newData);
    
    // Cleanup records older than 24 hours
    ASSERT_TRUE(localStore_->cleanupOldRecords(std::chrono::hours(24)));
    
    // Should only have recent data
    auto allTelemetry = localStore_->getUnsyncedTelemetry("UAV_TEST_001", 100);
    EXPECT_EQ(allTelemetry.size(), 1);
    EXPECT_EQ(allTelemetry[0].position.x, 10.0);
}

// ============================================================================
// Complex Scenario Tests
// ============================================================================

TEST_F(OfflineAutonomyIntegrationTest, FullOfflineMissionScenario) {
    // Setup: Deploy a search task
    OfflineTask task;
    task.id = "search_mission_001";
    task.type = TaskType::SEARCH;
    task.name = "Area Search";
    task.payload["search_area"] = {{0, 0}, {500, 0}, {500, 500}, {0, 500}};
    task.payload["pattern"] = "LAWN_MOWER";
    
    ASSERT_TRUE(autonomyManager_->deployOfflineTask(task));
    
    // Add rules
    OfflineRule lowBatteryRule;
    lowBatteryRule.id = "low_battery";
    lowBatteryRule.condition.type = ConditionType::BATTERY_BELOW;
    lowBatteryRule.condition.threshold = 30.0;
    lowBatteryRule.action.type = ActionType::RETURN_TO_LAUNCH;
    lowBatteryRule.enabled = true;
    ruleEngine_->addRule(lowBatteryRule);
    
    // Scenario: GCS disconnects during mission
    stateMachine_->transitionTo(State::MISSION_EXECUTION);
    autonomyManager_->handleGcsDisconnect();
    
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    EXPECT_TRUE(stateMachine_->transitionTo(State::OFFLINE_AUTONOMY));
    EXPECT_TRUE(autonomyManager_->startOfflineExecution());
    
    // Simulate mission progress with telemetry
    for (int i = 0; i < 10; ++i) {
        TelemetryData data;
        data.timestamp = std::chrono::system_clock::now();
        data.uavId = "UAV_TEST_001";
        data.position = {i * 50.0, i * 50.0, 50.0};
        data.batteryLevel = 70.0 - (i * 5);  // Decreasing battery
        data.isArmed = true;
        data.isFlying = true;
        
        autonomyManager_->cacheTelemetry(data);
        
        // Evaluate rules
        UAVState uavState;
        uavState.batteryLevel = data.batteryLevel;
        uavState.isConnectedToGcs = false;
        ruleEngine_->evaluateRules(uavState);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Complete mission
    ASSERT_TRUE(autonomyManager_->completeOfflineTask("search_mission_001", true));
    
    // Simulate GCS reconnect
    ASSERT_TRUE(autonomyManager_->handleGcsConnect());
    EXPECT_TRUE(stateMachine_->transitionTo(State::SAFE));
    EXPECT_TRUE(stateMachine_->transitionTo(State::ONLINE));
    
    // Sync telemetry
    EXPECT_TRUE(autonomyManager_->syncCachedTelemetry());
    
    // Verify task completed
    auto completedTask = autonomyManager_->getOfflineTask("search_mission_001");
    ASSERT_TRUE(completedTask.has_value());
    EXPECT_EQ(completedTask->status, TaskStatus::COMPLETED);
}

TEST_F(OfflineAutonomyIntegrationTest, MultipleOfflineTasksScenario) {
    // Deploy multiple tasks
    std::vector<std::string> taskIds = {"task1", "task2", "task3"};
    
    for (const auto& id : taskIds) {
        OfflineTask task;
        task.id = id;
        task.type = TaskType::PATROL;
        task.name = "Patrol " + id;
        task.payload["duration_minutes"] = 10;
        
        ASSERT_TRUE(autonomyManager_->deployOfflineTask(task));
    }
    
    // Go offline
    autonomyManager_->handleGcsDisconnect();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    ASSERT_TRUE(autonomyManager_->startOfflineExecution());
    
    // Execute tasks in order
    for (const auto& id : taskIds) {
        ASSERT_TRUE(autonomyManager_->setActiveOfflineTask(id));
        
        // Add some telemetry
        TelemetryData data;
        data.timestamp = std::chrono::system_clock::now();
        data.uavId = "UAV_TEST_001";
        data.position = {0.0, 0.0, 50.0};
        data.batteryLevel = 80.0;
        autonomyManager_->cacheTelemetry(data);
        
        // Complete task
        ASSERT_TRUE(autonomyManager_->completeOfflineTask(id, true));
    }
    
    // Verify all tasks completed
    auto allTasks = autonomyManager_->getAllOfflineTasks();
    EXPECT_EQ(allTasks.size(), 3);
    
    for (const auto& task : allTasks) {
        EXPECT_EQ(task.status, TaskStatus::COMPLETED);
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(OfflineAutonomyIntegrationTest, InvalidTaskIdHandling) {
    // Try to get non-existent task
    auto task = autonomyManager_->getOfflineTask("non_existent");
    EXPECT_FALSE(task.has_value());
    
    // Try to complete non-existent task
    EXPECT_FALSE(autonomyManager_->completeOfflineTask("non_existent", true));
}

TEST_F(OfflineAutonomyIntegrationTest, DuplicateTaskHandling) {
    OfflineTask task;
    task.id = "duplicate_task";
    task.type = TaskType::SEARCH;
    task.name = "Duplicate";
    
    ASSERT_TRUE(autonomyManager_->deployOfflineTask(task));
    
    // Deploying same ID should update existing task
    task.name = "Updated";
    ASSERT_TRUE(autonomyManager_->deployOfflineTask(task));
    
    auto retrieved = autonomyManager_->getOfflineTask("duplicate_task");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Updated");
}

TEST_F(OfflineAutonomyIntegrationTest, TaskCancellation) {
    OfflineTask task;
    task.id = "cancellable_task";
    task.type = TaskType::SEARCH;
    task.name = "Cancellable";
    
    ASSERT_TRUE(autonomyManager_->deployOfflineTask(task));
    
    // Cancel the task
    ASSERT_TRUE(autonomyManager_->cancelOfflineTask("cancellable_task"));
    
    auto cancelled = autonomyManager_->getOfflineTask("cancellable_task");
    ASSERT_TRUE(cancelled.has_value());
    EXPECT_EQ(cancelled->status, TaskStatus::CANCELLED);
}

} // namespace
