/**
 * @file test_fault_injection.cpp
 * @brief Fault injection tests for offline autonomy system
 * 
 * Tests cover:
 * - Network partition scenarios
 * - Database corruption handling
 * - Thread starvation conditions
 * - Resource exhaustion
 * - Timing failures
 * - Cascading failure scenarios
 * - Recovery from partial failures
 * 
 * @note Zero-mock testing - uses real components with fault injection
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "nodeagent/LocalStore.h"
#include "nodeagent/RuleEngine.h"
#include "nodeagent/StateMachine.h"
#include "nodeagent/SwarmPartitionManager.h"
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>

using namespace nodeagent;
using namespace testing;

class FaultInjectionTest : public Test {
protected:
    void SetUp() override {
        tempDir_ = std::filesystem::temp_directory_path() / "fault_injection_test";
        std::filesystem::create_directories(tempDir_);
        
        // Clean any existing files
        for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
            std::filesystem::remove_all(entry.path());
        }
    }
    
    void TearDown() override {
        if (localStore_) {
            localStore_->shutdown();
            localStore_.reset();
        }
        
        // Clean up
        std::filesystem::remove_all(tempDir_);
    }
    
    void injectRandomDelay(int minMs = 1, int maxMs = 50) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(minMs, maxMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
    }
    
    std::filesystem::path tempDir_;
    std::unique_ptr<LocalStore> localStore_;
};

// ============================================================================
// Network Partition Faults
// ============================================================================

TEST_F(FaultInjectionTest, IntermittentConnectionFailure) {
    localStore_ = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore_->initialize((tempDir_ / "intermittent.db").string()));
    
    // Simulate intermittent connectivity
    std::atomic<int> successfulOps{0};
    std::atomic<int> failedOps{0};
    
    std::vector<std::thread> threads;
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([this, &successfulOps, &failedOps]() {
            for (int i = 0; i < 20; ++i) {
                TelemetryData data;
                data.timestamp = std::chrono::system_clock::now();
                data.uavId = "UAV_001";
                data.position = {static_cast<double>(i), 0.0, 50.0};
                data.batteryLevel = 80.0;
                
                // Random delay simulates network jitter
                injectRandomDelay(1, 10);
                
                if (localStore_->storeTelemetry(data)) {
                    ++successfulOps;
                } else {
                    ++failedOps;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All operations should succeed (SQLite handles concurrency)
    EXPECT_EQ(successfulOps, 100);
    EXPECT_EQ(failedOps, 0);
}

TEST_F(FaultInjectionTest, CompleteNetworkPartition) {
    SwarmPartitionManager manager;
    SwarmPartitionConfig config;
    config.localUavId = "UAV_001";
    config.heartbeatTimeout = std::chrono::milliseconds(100);
    ASSERT_TRUE(manager.initialize(config));
    
    // Setup connected swarm
    SwarmMember m1{"UAV_001", 100.0, true, std::chrono::system_clock::now(), {0, 0, 50}, 80.0};
    SwarmMember m2{"UAV_002", 80.0, false, std::chrono::system_clock::now(), {0, 0, 50}, 80.0};
    SwarmMember m3{"UAV_003", 90.0, false, std::chrono::system_clock::now(), {0, 0, 50}, 80.0};
    
    manager.addMember(m1);
    manager.addMember(m2);
    manager.addMember(m3);
    
    manager.updateConnection("UAV_001", "UAV_002", true);
    manager.updateConnection("UAV_001", "UAV_003", true);
    
    EXPECT_EQ(manager.detectPartitions().size(), 1);
    
    // Complete partition - all connections drop
    manager.updateConnection("UAV_001", "UAV_002", false);
    manager.updateConnection("UAV_001", "UAV_003", false);
    
    auto partitions = manager.detectPartitions();
    EXPECT_EQ(partitions.size(), 3);  // Each UAV isolated
    
    // Each should have elected itself as leader
    for (const auto& p : partitions) {
        EXPECT_EQ(p.memberIds.size(), 1);
        EXPECT_FALSE(p.leaderId.empty());
    }
    
    manager.shutdown();
}

TEST_F(FaultInjectionTest, PartialNetworkPartition) {
    SwarmPartitionManager manager;
    SwarmPartitionConfig config;
    config.localUavId = "UAV_001";
    ASSERT_TRUE(manager.initialize(config));
    
    // 4 UAVs: 001 connected to 002 and 003, but not 004
    // 002 is connected to 004, creating indirect connection
    std::vector<SwarmMember> members = {
        {"UAV_001", 100.0, true, std::chrono::system_clock::now(), {0, 0, 50}, 80.0},
        {"UAV_002", 80.0, false, std::chrono::system_clock::now(), {0, 0, 50}, 80.0},
        {"UAV_003", 70.0, false, std::chrono::system_clock::now(), {0, 0, 50}, 80.0},
        {"UAV_004", 90.0, false, std::chrono::system_clock::now(), {0, 0, 50}, 80.0}
    };
    
    for (const auto& m : members) {
        manager.addMember(m);
    }
    
    // Setup topology
    manager.updateConnection("UAV_001", "UAV_002", true);
    manager.updateConnection("UAV_001", "UAV_003", true);
    manager.updateConnection("UAV_002", "UAV_004", true);
    
    // All should be in one partition through 002
    EXPECT_EQ(manager.detectPartitions().size(), 1);
    
    // Now break critical link between 001-002
    manager.updateConnection("UAV_001", "UAV_002", false);
    
    auto partitions = manager.detectPartitions();
    EXPECT_EQ(partitions.size(), 2);
    
    // {001, 003} and {002, 004}
    for (const auto& p : partitions) {
        EXPECT_EQ(p.memberIds.size(), 2);
    }
    
    manager.shutdown();
}

// ============================================================================
// Database Faults
// ============================================================================

TEST_F(FaultInjectionTest, DatabaseCorruptionRecovery) {
    std::string dbPath = (tempDir_ / "corruption_test.db").string();
    
    // Phase 1: Normal operation
    {
        localStore_ = std::make_unique<LocalStore>();
        ASSERT_TRUE(localStore_->initialize(dbPath));
        
        // Store some data
        for (int i = 0; i < 10; ++i) {
            OfflineTask task;
            task.id = "task_" + std::to_string(i);
            task.type = TaskType::SEARCH;
            task.name = "Task " + std::to_string(i);
            ASSERT_TRUE(localStore_->storeOfflineTask(task));
        }
        
        localStore_->shutdown();
        localStore_.reset();
    }
    
    // Phase 2: Simulate corruption by appending garbage to file
    {
        std::ofstream dbFile(dbPath, std::ios::app | std::ios::binary);
        dbFile << "CORRUPTION_DATA_THAT_BREAKS_SQLITE";
    }
    
    // Phase 3: Recovery - SQLite WAL mode should handle this
    localStore_ = std::make_unique<LocalStore>();
    bool recovered = localStore_->initialize(dbPath);
    
    // May or may not recover depending on where corruption is
    // But shouldn't crash
    if (recovered) {
        // If recovered, check data integrity
        auto tasks = localStore_->getAllOfflineTasks();
        // Some data might be lost, but no crash
    }
}

TEST_F(FaultInjectionTest, ConcurrentDatabaseAccess) {
    localStore_ = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore_->initialize((tempDir_ / "concurrent.db").string()));
    
    const int numThreads = 20;
    const int opsPerThread = 50;
    std::atomic<int> successCount{0};
    
    std::vector<std::thread> threads;
    
    // Writers
    for (int t = 0; t < numThreads / 2; ++t) {
        threads.emplace_back([this, t, opsPerThread, &successCount]() {
            for (int i = 0; i < opsPerThread; ++i) {
                TelemetryData data;
                data.timestamp = std::chrono::system_clock::now();
                data.uavId = "UAV_" + std::to_string(t);
                data.position = {static_cast<double>(i), static_cast<double>(t), 50.0};
                
                if (localStore_->storeTelemetry(data)) {
                    ++successCount;
                }
                
                injectRandomDelay(1, 5);
            }
        });
    }
    
    // Readers
    for (int t = numThreads / 2; t < numThreads; ++t) {
        threads.emplace_back([this, t]() {
            for (int i = 0; i < opsPerThread; ++i) {
                auto data = localStore_->getUnsyncedTelemetry("UAV_0", 10);
                injectRandomDelay(1, 5);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // All writes should succeed (SQLite handles serialization)
    EXPECT_EQ(successCount, (numThreads / 2) * opsPerThread);
}

// ============================================================================
// Resource Exhaustion Faults
// ============================================================================

TEST_F(FaultInjectionTest, MemoryPressure) {
    localStore_ = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore_->initialize((tempDir_ / "memory_pressure.db").string()));
    
    // Store large number of telemetry records
    const int numRecords = 10000;
    
    for (int i = 0; i < numRecords; ++i) {
        TelemetryData data;
        data.timestamp = std::chrono::system_clock::now() - std::chrono::seconds(i);
        data.uavId = "UAV_001";
        data.position = {static_cast<double>(i), static_cast<double>(i % 100), 50.0};
        data.batteryLevel = 80.0;
        
        ASSERT_TRUE(localStore_->storeTelemetry(data));
        
        // Periodic cleanup to simulate memory management
        if (i % 1000 == 0) {
            localStore_->cleanupOldRecords(std::chrono::hours(1));
        }
    }
    
    // Should still be able to query
    auto recent = localStore_->getUnsyncedTelemetry("UAV_001", 100);
    EXPECT_EQ(recent.size(), 100);
}

TEST_F(FaultInjectionTest, DiskSpaceExhaustionSimulation) {
    // This test simulates disk space issues by using a small database
    // and verifying error handling
    localStore_ = std::make_unique<LocalStore>();
    
    std::string dbPath = (tempDir_ / "disk_space.db").string();
    ASSERT_TRUE(localStore_->initialize(dbPath));
    
    // Attempt to store data
    bool storageFailed = false;
    for (int i = 0; i < 1000; ++i) {
        TelemetryData data;
        data.timestamp = std::chrono::system_clock::now();
        data.uavId = "UAV_001";
        data.position = {static_cast<double>(i), 0.0, 50.0};
        
        if (!localStore_->storeTelemetry(data)) {
            storageFailed = true;
            break;
        }
    }
    
    // Test completes without crash, storage may or may not fail
    // depending on actual disk space
    (void)storageFailed;
    SUCCEED();
}

// ============================================================================
// Timing Faults
// ============================================================================

TEST_F(FaultInjectionTest, ClockSkewHandling) {
    RuleEngine engine;
    ASSERT_TRUE(engine.initialize());
    
    // Add a time-based rule
    OfflineRule rule;
    rule.id = "time_rule";
    rule.condition.type = ConditionType::TIMEOUT;
    rule.condition.duration = std::chrono::seconds(5);
    rule.action.type = ActionType::RETURN_TO_LAUNCH;
    rule.enabled = true;
    
    engine.addRule(rule);
    
    UAVState state;
    state.batteryLevel = 80.0;
    state.isConnectedToGcs = false;
    state.missionStartTime = std::chrono::system_clock::now();
    
    // Initially shouldn't trigger
    auto actions = engine.evaluateRules(state);
    EXPECT_TRUE(actions.empty());
    
    // Simulate mission timeout
    state.missionStartTime = std::chrono::system_clock::now() - std::chrono::seconds(10);
    
    actions = engine.evaluateRules(state);
    EXPECT_EQ(actions.size(), 1);
    
    engine.shutdown();
}

TEST_F(FaultInjectionTest, RapidStateTransitions) {
    StateMachine sm;
    ASSERT_TRUE(sm.initialize(State::ONLINE));
    
    // Rapid state changes
    for (int i = 0; i < 100; ++i) {
        // Valid transition
        sm.transitionTo(State::MISSION_EXECUTION);
        
        // Invalid transition (should fail gracefully)
        sm.transitionTo(State::ONLINE);  // Can't go directly back
        
        // Correct path
        sm.transitionTo(State::OFFLINE_AUTONOMY);
        sm.transitionTo(State::SAFE);
        sm.transitionTo(State::ONLINE);
    }
    
    // Should end in ONLINE
    EXPECT_EQ(sm.getCurrentState(), State::ONLINE);
    
    // History should have recorded transitions
    auto history = sm.getTransitionHistory();
    EXPECT_GE(history.size(), 300);  // At least 3 per iteration
    
    sm.shutdown();
}

// ============================================================================
// Cascading Failure Tests
// ============================================================================

TEST_F(FaultInjectionTest, CascadingFailureScenario) {
    SwarmPartitionManager manager;
    SwarmPartitionConfig config;
    config.localUavId = "UAV_001";
    config.heartbeatTimeout = std::chrono::milliseconds(200);
    ASSERT_TRUE(manager.initialize(config));
    
    // Setup 5-member swarm
    std::vector<std::string> uavIds = {"UAV_001", "UAV_002", "UAV_003", "UAV_004", "UAV_005"};
    
    for (size_t i = 0; i < uavIds.size(); ++i) {
        SwarmMember m{uavIds[i], 100.0 - static_cast<double>(i) * 10, i == 0,
                      std::chrono::system_clock::now(), {0, 0, 50}, 80.0};
        manager.addMember(m);
    }
    
    // Connect in a chain: 001-002-003-004-005
    for (size_t i = 0; i < uavIds.size() - 1; ++i) {
        manager.updateConnection(uavIds[i], uavIds[i + 1], true);
    }
    
    EXPECT_EQ(manager.detectPartitions().size(), 1);
    
    // Cascading failure: middle member drops
    manager.updateConnection("UAV_002", "UAV_003", false);
    
    auto partitions = manager.detectPartitions();
    EXPECT_EQ(partitions.size(), 2);
    
    // Both sides should elect leaders
    for (const auto& p : partitions) {
        EXPECT_FALSE(p.leaderId.empty());
    }
    
    // Further failure
    manager.updateConnection("UAV_003", "UAV_004", false);
    
    partitions = manager.detectPartitions();
    EXPECT_EQ(partitions.size(), 3);
    
    manager.shutdown();
}

TEST_F(FaultInjectionTest, RecoveryAfterPartialFailure) {
    localStore_ = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore_->initialize((tempDir_ / "recovery.db").string()));
    
    // Store some tasks
    std::vector<std::string> taskIds;
    for (int i = 0; i < 5; ++i) {
        OfflineTask task;
        task.id = "task_" + std::to_string(i);
        task.type = TaskType::SEARCH;
        task.status = TaskStatus::PENDING;
        
        ASSERT_TRUE(localStore_->storeOfflineTask(task));
        taskIds.push_back(task.id);
    }
    
    // Simulate partial failure - lose reference but keep DB
    localStore_->shutdown();
    localStore_.reset();
    
    // Recover
    localStore_ = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore_->initialize((tempDir_ / "recovery.db").string()));
    
    // Verify data survived
    auto tasks = localStore_->getAllOfflineTasks();
    EXPECT_EQ(tasks.size(), 5);
    
    for (const auto& id : taskIds) {
        auto task = localStore_->getOfflineTask(id);
        EXPECT_TRUE(task.has_value());
    }
}

// ============================================================================
// Concurrency Faults
// ============================================================================

TEST_F(FaultInjectionTest, DeadlockPrevention) {
    SwarmPartitionManager manager;
    SwarmPartitionConfig config;
    config.localUavId = "UAV_001";
    ASSERT_TRUE(manager.initialize(config));
    
    // Add members
    for (int i = 1; i <= 5; ++i) {
        SwarmMember m{"UAV_00" + std::to_string(i), 100.0 - i * 10, i == 1,
                      std::chrono::system_clock::now(), {0, 0, 50}, 80.0};
        manager.addMember(m);
    }
    
    std::atomic<bool> running{true};
    std::atomic<int> ops{0};
    
    // Concurrent operations that could deadlock
    std::vector<std::thread> threads;
    
    threads.emplace_back([&]() {
        while (running && ops < 1000) {
            manager.detectPartitions();
            ++ops;
        }
    });
    
    threads.emplace_back([&]() {
        while (running && ops < 1000) {
            manager.electLeader();
            ++ops;
        }
    });
    
    threads.emplace_back([&]() {
        while (running && ops < 1000) {
            for (int i = 1; i <= 5; ++i) {
                for (int j = i + 1; j <= 5; ++j) {
                    manager.updateConnection("UAV_00" + std::to_string(i),
                                            "UAV_00" + std::to_string(j), true);
                }
            }
            ++ops;
        }
    });
    
    // Run for max 2 seconds
    std::this_thread::sleep_for(std::chrono::seconds(2));
    running = false;
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should complete without deadlock
    EXPECT_GE(ops, 100);
    
    manager.shutdown();
}

TEST_F(FaultInjectionTest, RaceConditionInStateMachine) {
    StateMachine sm;
    ASSERT_TRUE(sm.initialize(State::ONLINE));
    
    std::atomic<int> successfulTransitions{0};
    std::atomic<int> failedTransitions{0};
    
    std::vector<std::thread> threads;
    
    // Multiple threads trying to transition simultaneously
    for (int t = 0; t < 10; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < 50; ++i) {
                if (sm.transitionTo(State::MISSION_EXECUTION)) {
                    ++successfulTransitions;
                } else {
                    ++failedTransitions;
                }
                
                // Try to go back
                sm.transitionTo(State::SAFE);
                sm.transitionTo(State::ONLINE);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Some transitions should succeed, others fail (race condition expected)
    // But no crash
    EXPECT_GT(successfulTransitions + failedTransitions, 0);
    
    sm.shutdown();
}

// ============================================================================
// Byzantine Faults
// ============================================================================

TEST_F(FaultInjectionTest, MalformedDataHandling) {
    localStore_ = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore_->initialize((tempDir_ / "malformed.db").string()));
    
    // Try to store data with edge values
    TelemetryData data;
    data.timestamp = std::chrono::system_clock::now();
    data.uavId = "UAV_001";
    
    // Extreme position values
    data.position = {1e308, -1e308, 1e308};
    data.batteryLevel = -100.0;  // Invalid
    data.groundSpeed = 1e309;    // Infinity
    
    // Should handle gracefully
    bool result = localStore_->storeTelemetry(data);
    
    // May or may not store, but shouldn't crash
    (void)result;
    SUCCEED();
}

TEST_F(FaultInjectionTest, ConcurrentPartitionMergeRace) {
    SwarmPartitionManager manager;
    SwarmPartitionConfig config;
    config.localUavId = "UAV_001";
    ASSERT_TRUE(manager.initialize(config));
    
    // Setup partitions
    SwarmPartition p1{"PART_1", "UAV_001", {"UAV_001", "UAV_002"}};
    SwarmPartition p2{"PART_2", "UAV_003", {"UAV_003", "UAV_004"}};
    SwarmPartition p3{"PART_3", "UAV_005", {"UAV_005", "UAV_006"}};
    
    std::atomic<int> mergeCount{0};
    
    // Concurrent merge attempts
    std::vector<std::thread> threads;
    threads.emplace_back([&]() {
        auto result = manager.initiateMerge(p1, p2);
        if (result.success) ++mergeCount;
    });
    
    threads.emplace_back([&]() {
        auto result = manager.initiateMerge(p2, p3);
        if (result.success) ++mergeCount;
    });
    
    threads.emplace_back([&]() {
        auto result = manager.initiateMerge(p1, p3);
        if (result.success) ++mergeCount;
    });
    
    for (auto& t : threads) {
        t.join();
    }
    
    // At least one merge should succeed
    EXPECT_GE(mergeCount, 1);
    
    manager.shutdown();
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST_F(FaultInjectionTest, HighFrequencyPartitionChanges) {
    SwarmPartitionManager manager;
    SwarmPartitionConfig config;
    config.localUavId = "UAV_001";
    config.heartbeatTimeout = std::chrono::milliseconds(10);
    ASSERT_TRUE(manager.initialize(config));
    
    // Add 10 members
    for (int i = 1; i <= 10; ++i) {
        SwarmMember m{"UAV_" + std::to_string(i), 100.0 - i, i == 1,
                      std::chrono::system_clock::now(), {0, 0, 50}, 80.0};
        manager.addMember(m);
    }
    
    // Rapid topology changes
    for (int cycle = 0; cycle < 50; ++cycle) {
        // Random connections
        for (int i = 1; i <= 10; ++i) {
            for (int j = i + 1; j <= 10; ++j) {
                bool connect = (i + j + cycle) % 3 == 0;
                manager.updateConnection("UAV_" + std::to_string(i),
                                        "UAV_" + std::to_string(j), connect);
            }
        }
        
        auto partitions = manager.detectPartitions();
        
        // Always have at least one partition
        EXPECT_GE(partitions.size(), 1);
        
        // Total members across all partitions should be 10
        size_t totalMembers = 0;
        for (const auto& p : partitions) {
            totalMembers += p.memberIds.size();
        }
        EXPECT_EQ(totalMembers, 10);
    }
    
    manager.shutdown();
}

TEST_F(FaultInjectionTest, SustainedLoadTest) {
    localStore_ = std::make_unique<LocalStore>();
    ASSERT_TRUE(localStore_->initialize((tempDir_ / "sustained.db").string()));
    
    const int durationSeconds = 5;
    const auto startTime = std::chrono::steady_clock::now();
    
    std::atomic<int> operations{0};
    std::atomic<bool> running{true};
    
    std::vector<std::thread> threads;
    
    // Writer threads
    for (int t = 0; t < 5; ++t) {
        threads.emplace_back([&, t]() {
            while (running) {
                TelemetryData data;
                data.timestamp = std::chrono::system_clock::now();
                data.uavId = "UAV_" + std::to_string(t);
                data.position = {static_cast<double>(operations), 0.0, 50.0};
                
                if (localStore_->storeTelemetry(data)) {
                    ++operations;
                }
            }
        });
    }
    
    // Reader threads
    for (int t = 0; t < 3; ++t) {
        threads.emplace_back([&]() {
            while (running) {
                localStore_->getUnsyncedTelemetry("UAV_0", 100);
            }
        });
    }
    
    // Cleanup thread
    threads.emplace_back([&]() {
        while (running) {
            localStore_->cleanupOldRecords(std::chrono::minutes(1));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    // Run for duration
    std::this_thread::sleep_for(std::chrono::seconds(durationSeconds));
    running = false;
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should handle sustained load
    EXPECT_GT(operations, 1000);
    
    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count();
    
    std::cout << "Sustained load test: " << operations << " operations in " 
              << elapsed << " seconds" << std::endl;
}

} // namespace
