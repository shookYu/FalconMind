/**
 * @file benchmark_nodeagent.cpp
 * @brief Performance benchmarks for NodeAgent components
 * 
 * Tests:
 * - Telemetry insertion throughput
 * - Task allocation latency
 * - State machine transitions
 * - Partition detection speed
 * - Rule evaluation performance
 * - Log writing throughput
 * - Metrics collection overhead
 * 
 * @note Run with: ./benchmark_nodeagent --benchmark_filter="BM_*"
 */

#include <benchmark/benchmark.h>
#include "nodeagent/LocalStore.h"
#include "nodeagent/StateMachine.h"
#include "nodeagent/RuleEngine.h"
#include "nodeagent/MetricsCollector.h"
#include "nodeagent/AsyncLogger.h"
#include "nodeagent/DistributedTaskAllocator.h"
#include "nodeagent/SwarmPartitionManager.h"
#include <filesystem>
#include <chrono>

using namespace nodeagent;

// Helper to create temp directory
std::filesystem::path createTempDir() {
    auto path = std::filesystem::temp_directory_path() / 
                ("bench_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(path);
    return path;
}

// ============================================================================
// LocalStore Benchmarks
// ============================================================================

static void BM_TelemetryInsert(benchmark::State& state) {
    auto tempDir = createTempDir();
    LocalStore store;
    store.initialize((tempDir / "bench.db").string());
    
    TelemetryData data;
    data.uavId = "UAV_001";
    data.position = {10.0, 20.0, 50.0};
    data.batteryLevel = 80.0;
    
    for (auto _ : state) {
        data.timestamp = std::chrono::system_clock::now();
        store.storeTelemetry(data);
    }
    
    store.shutdown();
    std::filesystem::remove_all(tempDir);
}
BENCHMARK(BM_TelemetryInsert)->Iterations(1000)->Unit(benchmark::kMicrosecond);

static void BM_BatchTelemetryInsert(benchmark::State& state) {
    auto tempDir = createTempDir();
    LocalStore store;
    store.initialize((tempDir / "bench.db").string());
    
    std::vector<TelemetryData> batch;
    for (int i = 0; i < 100; ++i) {
        TelemetryData data;
        data.uavId = "UAV_001";
        data.position = {static_cast<double>(i), 20.0, 50.0};
        data.batteryLevel = 80.0;
        batch.push_back(data);
    }
    
    for (auto _ : state) {
        for (auto& data : batch) {
            data.timestamp = std::chrono::system_clock::now();
        }
        // Batch insert would be implemented in LocalStore
        for (const auto& data : batch) {
            store.storeTelemetry(data);
        }
    }
    
    store.shutdown();
    std::filesystem::remove_all(tempDir);
}
BENCHMARK(BM_BatchTelemetryInsert)->Iterations(100)->Unit(benchmark::kMillisecond);

static void BM_QueryUnsyncedTelemetry(benchmark::State& state) {
    auto tempDir = createTempDir();
    LocalStore store;
    store.initialize((tempDir / "bench.db").string());
    
    // Insert test data
    for (int i = 0; i < 1000; ++i) {
        TelemetryData data;
        data.uavId = "UAV_001";
        data.timestamp = std::chrono::system_clock::now();
        data.synced = (i % 2 == 0);
        store.storeTelemetry(data);
    }
    
    for (auto _ : state) {
        auto result = store.getUnsyncedTelemetry("UAV_001", 100);
        benchmark::DoNotOptimize(result);
    }
    
    store.shutdown();
    std::filesystem::remove_all(tempDir);
}
BENCHMARK(BM_QueryUnsyncedTelemetry)->Iterations(100)->Unit(benchmark::kMicrosecond);

// ============================================================================
// StateMachine Benchmarks
// ============================================================================

static void BM_StateTransition(benchmark::State& state) {
    StateMachine sm;
    sm.initialize(State::ONLINE);
    
    std::vector<State> states = {
        State::MISSION_EXECUTION,
        State::OFFLINE_AUTONOMY,
        State::SAFE,
        State::ONLINE
    };
    
    size_t idx = 0;
    for (auto _ : state) {
        sm.transitionTo(states[idx % states.size()]);
        idx++;
    }
    
    sm.shutdown();
}
BENCHMARK(BM_StateTransition)->Iterations(10000)->Unit(benchmark::kNanosecond);

// ============================================================================
// RuleEngine Benchmarks
// ============================================================================

static void BM_RuleEvaluation(benchmark::State& state) {
    RuleEngine engine;
    engine.initialize();
    
    // Add rules
    for (int i = 0; i < 10; ++i) {
        OfflineRule rule;
        rule.id = "rule_" + std::to_string(i);
        rule.priority = i;
        rule.enabled = true;
        rule.condition.type = ConditionType::BATTERY_BELOW;
        rule.condition.threshold = 30.0 - i;
        rule.action.type = ActionType::RETURN_TO_LAUNCH;
        engine.addRule(rule);
    }
    
    UAVState uavState;
    uavState.batteryLevel = 25.0;
    uavState.isConnectedToGcs = false;
    
    for (auto _ : state) {
        engine.evaluateRules(uavState);
    }
    
    engine.shutdown();
}
BENCHMARK(BM_RuleEvaluation)->Iterations(1000)->Unit(benchmark::kMicrosecond);

// ============================================================================
// MetricsCollector Benchmarks
// ============================================================================

static void BM_MetricsIncrement(benchmark::State& state) {
    MetricsCollector collector;
    collector.initialize();
    collector.registerMetric({"test_counter", MetricType::COUNTER, "Test", "", {}});
    
    for (auto _ : state) {
        collector.incrementCounter("test_counter");
    }
}
BENCHMARK(BM_MetricsIncrement)->Iterations(100000)->Unit(benchmark::kNanosecond);

static void BM_MetricsHistogramRecord(benchmark::State& state) {
    MetricsCollector collector;
    collector.initialize();
    collector.registerMetric({"test_histogram", MetricType::HISTOGRAM, "Test", "ms", {}});
    
    for (auto _ : state) {
        collector.recordHistogram("test_histogram", 25.0);
    }
}
BENCHMARK(BM_MetricsHistogramRecord)->Iterations(100000)->Unit(benchmark::kNanosecond);

static void BM_MetricsExport(benchmark::State& state) {
    MetricsCollector collector;
    collector.initialize();
    
    // Populate with metrics
    for (int i = 0; i < 100; ++i) {
        collector.incrementCounter("counter_" + std::to_string(i));
        collector.setGauge("gauge_" + std::to_string(i), static_cast<double>(i));
    }
    
    for (auto _ : state) {
        auto json = collector.exportToJson();
        benchmark::DoNotOptimize(json);
    }
}
BENCHMARK(BM_MetricsExport)->Iterations(100)->Unit(benchmark::kMicrosecond);

// ============================================================================
// AsyncLogger Benchmarks
// ============================================================================

static void BM_LogWrite(benchmark::State& state) {
    auto tempDir = createTempDir();
    
    LoggerConfig config;
    config.logDirectory = tempDir.string();
    config.async = false;  // Synchronous for consistent benchmarking
    config.useJsonFormat = false;
    
    AsyncLogger logger;
    logger.initialize(config);
    
    for (auto _ : state) {
        logger.info("Benchmark", "Test log message for performance testing");
    }
    
    logger.shutdown();
    std::filesystem::remove_all(tempDir);
}
BENCHMARK(BM_LogWrite)->Iterations(10000)->Unit(benchmark::kMicrosecond);

static void BM_LogWriteJson(benchmark::State& state) {
    auto tempDir = createTempDir();
    
    LoggerConfig config;
    config.logDirectory = tempDir.string();
    config.async = false;
    config.useJsonFormat = true;
    
    AsyncLogger logger;
    logger.initialize(config);
    
    nlohmann::json ctx;
    ctx["request_id"] = "12345";
    ctx["user_id"] = "user_123";
    
    for (auto _ : state) {
        logger.log(LogLevel::INFO, "Benchmark", "Test message", "", 0, "", ctx);
    }
    
    logger.shutdown();
    std::filesystem::remove_all(tempDir);
}
BENCHMARK(BM_LogWriteJson)->Iterations(10000)->Unit(benchmark::kMicrosecond);

// ============================================================================
// DistributedTaskAllocator Benchmarks
// ============================================================================

static void BM_TaskSubmission(benchmark::State& state) {
    DistributedTaskAllocator allocator("UAV_001");
    allocator.initialize(true);
    
    DistributedTask task;
    task.type = DistributedTaskType::SEARCH;
    task.name = "Benchmark Task";
    task.requirements.minBatteryLevel = 20.0;
    
    for (auto _ : state) {
        task.taskId.clear();  // Let it generate new ID
        allocator.submitTask(task);
    }
    
    allocator.shutdown();
}
BENCHMARK(BM_TaskSubmission)->Iterations(1000)->Unit(benchmark::kMicrosecond);

static void BM_BidCalculation(benchmark::State& state) {
    DistributedTaskAllocator allocator("UAV_001");
    allocator.initialize(true);
    
    TaskRequirements req;
    req.minBatteryLevel = 30.0;
    req.priority = 1.5;
    
    UavCapability cap;
    cap.batteryLevel = 80.0;
    cap.currentLoad = 0.3;
    cap.computePower = 6.0;
    cap.hasNPU = true;
    
    for (auto _ : state) {
        double score = allocator.calculateBidScore(req, cap);
        benchmark::DoNotOptimize(score);
    }
    
    allocator.shutdown();
}
BENCHMARK(BM_BidCalculation)->Iterations(100000)->Unit(benchmark::kNanosecond);

// ============================================================================
// SwarmPartitionManager Benchmarks
// ============================================================================

static void BM_PartitionDetection(benchmark::State& state) {
    SwarmPartitionManager manager("UAV_001", "SWARM_001");
    manager.initialize();
    
    // Add members
    for (int i = 1; i <= 20; ++i) {
        SwarmMember m;
        m.uavId = "UAV_" + std::to_string(i);
        m.capabilities.batteryLevel = 80;
        m.isActive = true;
        manager.addMember(m);
    }
    
    // Create connections (chain topology)
    for (int i = 1; i < 20; ++i) {
        manager.updateConnection("UAV_" + std::to_string(i), 
                                "UAV_" + std::to_string(i + 1), true);
    }
    
    for (auto _ : state) {
        auto partitions = manager.detectPartitions();
        benchmark::DoNotOptimize(partitions);
    }
    
    manager.shutdown();
}
BENCHMARK(BM_PartitionDetection)->Iterations(100)->Unit(benchmark::kMicrosecond);

static void BM_LeaderElection(benchmark::State& state) {
    SwarmPartitionManager manager("UAV_001", "SWARM_001");
    manager.initialize();
    
    // Add members with varying capabilities
    for (int i = 1; i <= 10; ++i) {
        SwarmMember m;
        m.uavId = "UAV_" + std::to_string(i);
        m.capabilities.batteryLevel = 50 + i * 5;
        m.capabilities.computePower = i;
        m.isActive = true;
        manager.addMember(m);
    }
    
    for (auto _ : state) {
        auto leader = manager.electLeader();
        benchmark::DoNotOptimize(leader);
    }
    
    manager.shutdown();
}
BENCHMARK(BM_LeaderElection)->Iterations(1000)->Unit(benchmark::kMicrosecond);

// ============================================================================
// End-to-End Scenario Benchmarks
// ============================================================================

static void BM_FullAutonomyCycle(benchmark::State& state) {
    auto tempDir = createTempDir();
    
    // Setup components
    LocalStore store;
    store.initialize((tempDir / "bench.db").string());
    
    StateMachine sm;
    sm.initialize(State::ONLINE);
    
    RuleEngine engine;
    engine.initialize();
    
    MetricsCollector metrics;
    metrics.initialize();
    
    for (auto _ : state) {
        // Simulate: GCS disconnect -> Offline autonomy -> GCS reconnect
        
        // 1. Detect disconnect
        sm.transitionTo(State::OFFLINE_AUTONOMY);
        
        // 2. Cache telemetry
        TelemetryData data;
        data.uavId = "UAV_001";
        data.batteryLevel = 80.0;
        store.storeTelemetry(data);
        
        // 3. Evaluate rules
        UAVState uavState;
        uavState.batteryLevel = 80.0;
        uavState.isConnectedToGcs = false;
        engine.evaluateRules(uavState);
        
        // 4. Record metrics
        metrics.incrementCounter("telemetry_cached");
        
        // 5. Reconnect
        sm.transitionTo(State::SAFE);
        sm.transitionTo(State::ONLINE);
        
        // 6. Sync data
        auto unsynced = store.getUnsyncedTelemetry("UAV_001", 100);
        metrics.incrementCounter("telemetry_synced", static_cast<double>(unsynced.size()));
    }
    
    metrics.shutdown();
    engine.shutdown();
    sm.shutdown();
    store.shutdown();
    std::filesystem::remove_all(tempDir);
}
BENCHMARK(BM_FullAutonomyCycle)->Iterations(100)->Unit(benchmark::kMicrosecond);

// ============================================================================
// Main
// ============================================================================

BENCHMARK_MAIN();
