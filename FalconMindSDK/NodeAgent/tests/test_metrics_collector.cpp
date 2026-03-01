/**
 * @file test_metrics_collector.cpp
 * @brief Comprehensive unit tests for MetricsCollector
 * 
 * Tests cover:
 * - Metric registration and type validation
 * - Counter increment operations
 * - Gauge set operations  
 * - Histogram recording with buckets
 * - Timer start/stop measurements
 * - Prometheus format export
 * - JSON format export
 * - Alert threshold checking
 * - Thread safety under concurrent access
 * - Component-specific metric helpers
 * 
 * @note Zero-mock testing - uses real MetricsCollector implementation
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "nodeagent/MetricsCollector.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace nodeagent;
using namespace testing;

class MetricsCollectorTest : public Test {
protected:
    void SetUp() override {
        collector_ = std::make_unique<MetricsCollector>();
        ASSERT_TRUE(collector_>initialize());
    }
    
    void TearDown() override {
        collector_>shutdown();
        collector_.reset();
    }
    
    std::unique_ptr<MetricsCollector> collector_;
};

// ============================================================================
// Basic Initialization Tests
// ============================================================================

TEST_F(MetricsCollectorTest, InitializeCreatesDefaultMetrics) {
    // Should have system metrics registered by default
    EXPECT_TRUE(collector_>isMetricRegistered("system_cpu_usage"));
    EXPECT_TRUE(collector_>isMetricRegistered("system_memory_usage"));
    EXPECT_TRUE(collector_>isMetricRegistered("system_disk_usage"));
    EXPECT_TRUE(collector_>isMetricRegistered("system_uptime_seconds"));
}

TEST_F(MetricsCollectorTest, InitializeIsIdempotent) {
    // Second initialization should succeed
    EXPECT_TRUE(collector_>initialize());
    
    // Metrics should still be there
    EXPECT_TRUE(collector_>isMetricRegistered("system_cpu_usage"));
}

TEST_F(MetricsCollectorTest, ShutdownClearsResources) {
    collector_>shutdown();
    
    // After shutdown, metrics should be cleared
    auto metrics = collector_>getAllMetrics();
    EXPECT_TRUE(metrics.empty());
}

// ============================================================================
// Metric Registration Tests
// ============================================================================

TEST_F(MetricsCollectorTest, RegisterCounterMetric) {
    MetricDefinition def{
        "test_counter",
        MetricType::COUNTER,
        "A test counter metric",
        "requests",
        {"endpoint", "status"}
    };
    
    collector_>registerMetric(def);
    
    EXPECT_TRUE(collector_>isMetricRegistered("test_counter"));
    
    auto metrics = collector_>getRegisteredMetrics();
    auto it = std::find_if(metrics.begin(), metrics.end(),
        [](const auto& m) { return m.name == "test_counter"; });
    
    ASSERT_NE(it, metrics.end());
    EXPECT_EQ(it->type, MetricType::COUNTER);
    EXPECT_EQ(it->description, "A test counter metric");
    EXPECT_EQ(it->unit, "requests");
    EXPECT_THAT(it->labelNames, ElementsAre("endpoint", "status"));
}

TEST_F(MetricsCollectorTest, RegisterGaugeMetric) {
    MetricDefinition def{
        "test_gauge",
        MetricType::GAUGE,
        "A test gauge metric",
        "percent",
        {}
    };
    
    collector_>registerMetric(def);
    EXPECT_TRUE(collector_>isMetricRegistered("test_gauge"));
}

TEST_F(MetricsCollectorTest, RegisterHistogramMetric) {
    MetricDefinition def{
        "test_histogram",
        MetricType::HISTOGRAM,
        "Request latency distribution",
        "milliseconds",
        {"method"}
    };
    
    collector_>registerMetric(def);
    EXPECT_TRUE(collector_>isMetricRegistered("test_histogram"));
}

TEST_F(MetricsCollectorTest, UnregisterMetricRemovesFromRegistry) {
    MetricDefinition def{"temp_metric", MetricType::GAUGE, "Temp", "", {}};
    collector_>registerMetric(def);
    
    EXPECT_TRUE(collector_>isMetricRegistered("temp_metric"));
    
    collector_>unregisterMetric("temp_metric");
    
    EXPECT_FALSE(collector_>isMetricRegistered("temp_metric"));
}

// ============================================================================
// Counter Tests
// ============================================================================

TEST_F(MetricsCollectorTest, IncrementCounterBasic) {
    collector_>registerMetric({"requests", MetricType::COUNTER, "Total requests", "", {}});
    
    collector_>incrementCounter("requests", 1.0);
    EXPECT_DOUBLE_EQ(collector_>getCounter("requests"), 1.0);
    
    collector_>incrementCounter("requests", 5.0);
    EXPECT_DOUBLE_EQ(collector_>getCounter("requests"), 6.0);
}

TEST_F(MetricsCollectorTest, IncrementCounterWithLabels) {
    collector_>registerMetric({"requests", MetricType::COUNTER, "Total requests", "", {"method", "status"}});
    
    std::map<std::string, std::string> labels1{{"method", "GET"}, {"status", "200"}};
    std::map<std::string, std::string> labels2{{"method", "POST"}, {"status", "200"}};
    
    collector_>incrementCounter("requests", 1.0, labels1);
    collector_>incrementCounter("requests", 2.0, labels1);
    collector_>incrementCounter("requests", 3.0, labels2);
    
    EXPECT_DOUBLE_EQ(collector_>getCounter("requests", labels1), 3.0);
    EXPECT_DOUBLE_EQ(collector_>getCounter("requests", labels2), 3.0);
}

TEST_F(MetricsCollectorTest, IncrementCounterNegativeValueIgnored) {
    collector_>registerMetric({"requests", MetricType::COUNTER, "Total requests", "", {}});
    
    collector_>incrementCounter("requests", 5.0);
    collector_>incrementCounter("requests", -3.0);  // Should be ignored
    
    EXPECT_DOUBLE_EQ(collector_>getCounter("requests"), 5.0);
}

TEST_F(MetricsCollectorTest, IncrementCounterDefaultAmount) {
    collector_>registerMetric({"requests", MetricType::COUNTER, "Total requests", "", {}});
    
    collector_>incrementCounter("requests");  // Default amount = 1.0
    collector_>incrementCounter("requests");
    
    EXPECT_DOUBLE_EQ(collector_>getCounter("requests"), 2.0);
}

// ============================================================================
// Gauge Tests
// ============================================================================

TEST_F(MetricsCollectorTest, SetGaugeBasic) {
    collector_>registerMetric({"cpu_usage", MetricType::GAUGE, "CPU usage", "percent", {}});
    
    collector_>setGauge("cpu_usage", 45.5);
    EXPECT_DOUBLE_EQ(collector_>getGauge("cpu_usage"), 45.5);
    
    collector_>setGauge("cpu_usage", 78.2);
    EXPECT_DOUBLE_EQ(collector_>getGauge("cpu_usage"), 78.2);
}

TEST_F(MetricsCollectorTest, SetGaugeWithLabels) {
    collector_>registerMetric({"memory_usage", MetricType::GAUGE, "Memory usage", "bytes", {"type"}});
    
    std::map<std::string, std::string> heap{{"type", "heap"}};
    std::map<std::string, std::string> stack{{"type", "stack"}};
    
    collector_>setGauge("memory_usage", 1024000.0, heap);
    collector_>setGauge("memory_usage", 512000.0, stack);
    
    EXPECT_DOUBLE_EQ(collector_>getGauge("memory_usage", heap), 1024000.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("memory_usage", stack), 512000.0);
}

TEST_F(MetricsCollectorTest, SetGaugeCanDecrease) {
    collector_>registerMetric({"queue_size", MetricType::GAUGE, "Queue size", "items", {}});
    
    collector_>setGauge("queue_size", 100.0);
    collector_>setGauge("queue_size", 50.0);  // Can go down
    collector_>setGauge("queue_size", 0.0);   // Can go to zero
    
    EXPECT_DOUBLE_EQ(collector_>getGauge("queue_size"), 0.0);
}

// ============================================================================
// Histogram Tests
// ============================================================================

TEST_F(MetricsCollectorTest, RecordHistogramUpdatesBuckets) {
    collector_>registerMetric({"latency", MetricType::HISTOGRAM, "Request latency", "ms", {}});
    
    collector_>recordHistogram("latency", 5.0);
    collector_>recordHistogram("latency", 15.0);
    collector_>recordHistogram("latency", 50.0);
    collector_>recordHistogram("latency", 150.0);
    
    auto data = collector_>getHistogram("latency");
    EXPECT_EQ(data.count, 4);
    EXPECT_DOUBLE_EQ(data.sum, 220.0);
    
    // Check that buckets are updated correctly
    ASSERT_FALSE(data.buckets.empty());
    
    // Count values <= 10.0 should be 1 (only 5.0)
    auto it = std::find_if(data.buckets.begin(), data.buckets.end(),
        [](const auto& b) { return b.upperBound == 10.0; });
    if (it != data.buckets.end()) {
        EXPECT_EQ(it->count, 1);
    }
}

TEST_F(MetricsCollectorTest, RecordHistogramWithLabels) {
    collector_>registerMetric({"latency", MetricType::HISTOGRAM, "Request latency", "ms", {"endpoint"}});
    
    std::map<std::string, std::string> api1{{"endpoint", "/api/v1"}};
    std::map<std::string, std::string> api2{{"endpoint", "/api/v2"}};
    
    collector_>recordHistogram("latency", 10.0, api1);
    collector_>recordHistogram("latency", 20.0, api1);
    collector_>recordHistogram("latency", 30.0, api2);
    
    auto data1 = collector_>getHistogram("latency", api1);
    auto data2 = collector_>getHistogram("latency", api2);
    
    EXPECT_EQ(data1.count, 2);
    EXPECT_DOUBLE_EQ(data1.sum, 30.0);
    
    EXPECT_EQ(data2.count, 1);
    EXPECT_DOUBLE_EQ(data2.sum, 30.0);
}

// ============================================================================
// Timer Tests
// ============================================================================

TEST_F(MetricsCollectorTest, TimerMeasuresDuration) {
    collector_>registerMetric({"operation_time", MetricType::TIMER, "Operation duration", "ms", {}});
    
    collector_>startTimer("operation_time");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    collector_>stopTimer("operation_time");
    
    auto data = collector_>getHistogram("operation_time");
    EXPECT_EQ(data.count, 1);
    EXPECT_GE(data.sum, 10.0);  // Should be at least 10ms
}

TEST_F(MetricsCollectorTest, TimerWithoutStartIsIgnored) {
    collector_>registerMetric({"operation_time", MetricType::TIMER, "Operation duration", "ms", {}});
    
    // Stop without start should be ignored
    collector_>stopTimer("operation_time");
    
    auto data = collector_>getHistogram("operation_time");
    EXPECT_EQ(data.count, 0);
}

TEST_F(MetricsCollectorTest, TimerMultipleMeasurements) {
    collector_>registerMetric({"operation_time", MetricType::TIMER, "Operation duration", "ms", {}});
    
    for (int i = 0; i < 5; ++i) {
        collector_>startTimer("operation_time");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        collector_>stopTimer("operation_time");
    }
    
    auto data = collector_>getHistogram("operation_time");
    EXPECT_EQ(data.count, 5);
    EXPECT_GE(data.sum, 25.0);
}

// ============================================================================
// Export Tests
// ============================================================================

TEST_F(MetricsCollectorTest, ExportToJsonStructure) {
    collector_>registerMetric({"counter1", MetricType::COUNTER, "", "", {}});
    collector_>registerMetric({"gauge1", MetricType::GAUGE, "", "", {}});
    
    collector_>incrementCounter("counter1", 42.0);
    collector_>setGauge("gauge1", 99.5);
    
    auto json = collector_>exportToJson();
    
    EXPECT_TRUE(json.contains("timestamp"));
    EXPECT_TRUE(json.contains("counters"));
    EXPECT_TRUE(json.contains("gauges"));
    EXPECT_TRUE(json.contains("histograms"));
    EXPECT_TRUE(json.contains("statistics"));
    
    EXPECT_TRUE(json["counters"].contains("counter1"));
    EXPECT_DOUBLE_EQ(json["counters"]["counter1"], 42.0);
    
    EXPECT_TRUE(json["gauges"].contains("gauge1"));
    EXPECT_DOUBLE_EQ(json["gauges"]["gauge1"], 99.5);
}

TEST_F(MetricsCollectorTest, ExportToPrometheusFormat) {
    collector_>registerMetric({"http_requests", MetricType::COUNTER, "Total HTTP requests", "", {"method"}});
    
    std::map<std::string, std::string> labels{{"method", "GET"}};
    collector_>incrementCounter("http_requests", 100.0, labels);
    
    auto prom = collector_>exportToPrometheusFormat();
    
    EXPECT_THAT(prom, HasSubstr("# HELP http_requests"));
    EXPECT_THAT(prom, HasSubstr("# TYPE http_requests counter"));
    EXPECT_THAT(prom, HasSubstr("http_requests"));
    EXPECT_THAT(prom, HasSubstr("100.000000"));
}

TEST_F(MetricsCollectorTest, ExportToPrometheusHistogramFormat) {
    collector_>registerMetric({"request_latency", MetricType::HISTOGRAM, "Request latency", "ms", {}});
    
    collector_>recordHistogram("request_latency", 25.0);
    collector_>recordHistogram("request_latency", 75.0);
    
    auto prom = collector_>exportToPrometheusFormat();
    
    EXPECT_THAT(prom, HasSubstr("# TYPE request_latency histogram"));
    EXPECT_THAT(prom, HasSubstr("request_latency_bucket"));
    EXPECT_THAT(prom, HasSubstr("request_latency_sum"));
    EXPECT_THAT(prom, HasSubstr("request_latency_count"));
}

// ============================================================================
// Alert Tests
// ============================================================================

TEST_F(MetricsCollectorTest, AlertTriggeredWhenThresholdExceeded) {
    collector_>registerMetric({"cpu_temp", MetricType::GAUGE, "CPU temperature", "celsius", {}});
    
    bool alertTriggered = false;
    std::string alertMetric;
    double alertValue = 0.0;
    
    collector_>setAlertCallback([&](const std::string& name, double value, const std::string&) {
        alertTriggered = true;
        alertMetric = name;
        alertValue = value;
    });
    
    collector_>setAlertThreshold("cpu_temp", 80.0, ">");
    
    collector_>setGauge("cpu_temp", 85.0);
    
    EXPECT_TRUE(alertTriggered);
    EXPECT_EQ(alertMetric, "cpu_temp");
    EXPECT_DOUBLE_EQ(alertValue, 85.0);
}

TEST_F(MetricsCollectorTest, AlertNotTriggeredBelowThreshold) {
    collector_>registerMetric({"cpu_temp", MetricType::GAUGE, "CPU temperature", "celsius", {}});
    
    bool alertTriggered = false;
    collector_>setAlertCallback([&](const std::string&, double, const std::string&) {
        alertTriggered = true;
    });
    
    collector_>setAlertThreshold("cpu_temp", 80.0, ">");
    
    collector_>setGauge("cpu_temp", 75.0);  // Below threshold
    
    EXPECT_FALSE(alertTriggered);
}

TEST_F(MetricsCollectorTest, AlertWithDifferentComparisons) {
    collector_>registerMetric({"battery", MetricType::GAUGE, "Battery level", "percent", {}});
    collector_>registerMetric({"queue", MetricType::GAUGE, "Queue size", "items", {}});
    
    std::atomic<int> alertCount{0};
    collector_>setAlertCallback([&](const std::string&, double, const std::string&) {
        ++alertCount;
    });
    
    // Low battery alert (<)
    collector_>setAlertThreshold("battery", 20.0, "<");
    collector_>setGauge("battery", 15.0);
    EXPECT_EQ(alertCount, 1);
    
    // High queue alert (>=)
    collector_>setAlertThreshold("queue", 100.0, ">=");
    collector_>setGauge("queue", 100.0);  // Equal triggers >=
    EXPECT_EQ(alertCount, 2);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(MetricsCollectorTest, StatisticsTracking) {
    collector_>registerMetric({"counter1", MetricType::COUNTER, "", "", {}});
    collector_>registerMetric({"gauge1", MetricType::GAUGE, "", "", {}});
    collector_>registerMetric({"hist1", MetricType::HISTOGRAM, "", "", {}});
    collector_>registerMetric({"timer1", MetricType::TIMER, "", "", {}});
    
    collector_>incrementCounter("counter1");
    collector_>incrementCounter("counter1");
    collector_>setGauge("gauge1", 50.0);
    collector_>recordHistogram("hist1", 10.0);
    collector_>startTimer("timer1");
    collector_>stopTimer("timer1");
    
    auto stats = collector_>getStatistics();
    
    EXPECT_EQ(stats.totalCounterIncrements, 2);
    EXPECT_EQ(stats.totalGaugeSets, 1);
    EXPECT_EQ(stats.totalHistogramRecords, 1);
    EXPECT_EQ(stats.totalTimerRecords, 1);
    EXPECT_EQ(stats.totalMetricsRecorded, 5);
    EXPECT_GE(stats.uptime.count(), 0);
}

// ============================================================================
// System Metrics Tests
// ============================================================================

TEST_F(MetricsCollectorTest, SystemMetricsCollection) {
    // Record system metrics
    collector_>recordSystemMetrics();
    
    // Values should be set (exact values depend on system state)
    double cpu = collector_>getGauge("system_cpu_usage");
    double mem = collector_>getGauge("system_memory_usage");
    double disk = collector_>getGauge("system_disk_usage");
    
    // All should be between 0 and 100
    EXPECT_GE(cpu, 0.0);
    EXPECT_LE(cpu, 100.0);
    EXPECT_GE(mem, 0.0);
    EXPECT_LE(mem, 100.0);
    EXPECT_GE(disk, 0.0);
    EXPECT_LE(disk, 100.0);
}

// ============================================================================
// Component Helper Tests
// ============================================================================

TEST_F(MetricsCollectorTest, TelemetryMetricsRecording) {
    collector_>recordTelemetryMetrics(1000, 800, 200);
    
    EXPECT_DOUBLE_EQ(collector_>getGauge("telemetry_buffer_size"), 1000.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("telemetry_synced_count"), 800.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("telemetry_unsynced_count"), 200.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("telemetry_sync_rate"), 80.0);
}

TEST_F(MetricsCollectorTest, TaskMetricsRecording) {
    collector_>recordTaskMetrics("SEARCH", std::chrono::milliseconds(1500), true);
    collector_>recordTaskMetrics("PATROL", std::chrono::milliseconds(2000), false);
    
    // Check counters
    std::map<std::string, std::string> searchSuccess{{"task_type", "SEARCH"}, {"result", "success"}};
    std::map<std::string, std::string> patrolFailure{{"task_type", "PATROL"}, {"result", "failure"}};
    
    EXPECT_DOUBLE_EQ(collector_>getCounter("task_execution_total", searchSuccess), 1.0);
    EXPECT_DOUBLE_EQ(collector_>getCounter("task_execution_total", patrolFailure), 1.0);
    
    // Check success/failure totals
    EXPECT_DOUBLE_EQ(collector_>getCounter("task_success_total", {{"task_type", "SEARCH"}}), 1.0);
    EXPECT_DOUBLE_EQ(collector_>getCounter("task_failure_total", {{"task_type", "PATROL"}}), 1.0);
}

TEST_F(MetricsCollectorTest, ConnectionMetricsRecording) {
    collector_>recordConnectionMetrics("gcs_192.168.1.1", true, 25.5);
    collector_>recordConnectionMetrics("gcs_192.168.1.1", false, 0.0);
    
    std::map<std::string, std::string> labels{{"endpoint", "gcs_192.168.1.1"}};
    
    EXPECT_DOUBLE_EQ(collector_>getGauge("connection_status", labels), 0.0);  // Last was disconnected
    EXPECT_DOUBLE_EQ(collector_>getCounter("connection_disconnects_total", labels), 1.0);
}

TEST_F(MetricsCollectorTest, PartitionMetricsRecording) {
    collector_>recordPartitionMetrics(3, 20, 18);
    
    EXPECT_DOUBLE_EQ(collector_>getGauge("swarm_partition_count"), 3.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("swarm_total_members"), 20.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("swarm_active_members"), 18.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("swarm_participation_rate"), 90.0);
}

TEST_F(MetricsCollectorTest, LeaderElectionMetricsRecording) {
    collector_>recordLeaderElectionMetrics(std::chrono::milliseconds(500), true);
    collector_>recordLeaderElectionMetrics(std::chrono::milliseconds(1000), false);
    
    EXPECT_DOUBLE_EQ(collector_>getCounter("leader_election_success_total"), 1.0);
    EXPECT_DOUBLE_EQ(collector_>getCounter("leader_election_failure_total"), 1.0);
}

TEST_F(MetricsCollectorTest, RuleEvaluationMetricsRecording) {
    collector_>recordRuleEvaluationMetrics(10, 3, std::chrono::milliseconds(50));
    
    EXPECT_DOUBLE_EQ(collector_>getGauge("rules_evaluated_total"), 10.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("rules_triggered_total"), 3.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("rule_trigger_rate"), 30.0);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(MetricsCollectorTest, ConcurrentCounterIncrements) {
    collector_>registerMetric({"concurrent_counter", MetricType::COUNTER, "", "", {}});
    
    const int numThreads = 10;
    const int incrementsPerThread = 1000;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < incrementsPerThread; ++j) {
                collector_>incrementCounter("concurrent_counter", 1.0);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    EXPECT_DOUBLE_EQ(collector_>getCounter("concurrent_counter"), 
                     numThreads * incrementsPerThread);
}

TEST_F(MetricsCollectorTest, ConcurrentGaugeSets) {
    collector_>registerMetric({"concurrent_gauge", MetricType::GAUGE, "", "", {}});
    
    const int numThreads = 10;
    const int setsPerThread = 1000;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < setsPerThread; ++j) {
                collector_>setGauge("concurrent_gauge", static_cast<double>(i * setsPerThread + j));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Just verify no crashes and gauge has a valid value
    double value = collector_>getGauge("concurrent_gauge");
    EXPECT_GE(value, 0.0);
}

TEST_F(MetricsCollectorTest, ConcurrentHistogramRecords) {
    collector_>registerMetric({"concurrent_histogram", MetricType::HISTOGRAM, "", "", {}});
    
    const int numThreads = 10;
    const int recordsPerThread = 1000;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([&]() {
            for (int j = 0; j < recordsPerThread; ++j) {
                collector_>recordHistogram("concurrent_histogram", static_cast<double>(j % 100));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto data = collector_>getHistogram("concurrent_histogram");
    EXPECT_EQ(data.count, numThreads * recordsPerThread);
}

// ============================================================================
// Clear Tests
// ============================================================================

TEST_F(MetricsCollectorTest, ClearAllMetrics) {
    collector_>registerMetric({"counter1", MetricType::COUNTER, "", "", {}});
    collector_>registerMetric({"gauge1", MetricType::GAUGE, "", "", {}});
    
    collector_>incrementCounter("counter1", 100.0);
    collector_>setGauge("gauge1", 50.0);
    
    collector_>clearAllMetrics();
    
    EXPECT_DOUBLE_EQ(collector_>getCounter("counter1"), 0.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("gauge1"), 0.0);
}

// ============================================================================
// GetAllMetrics Tests
// ============================================================================

TEST_F(MetricsCollectorTest, GetAllMetricsReturnsAllTypes) {
    collector_>registerMetric({"counter1", MetricType::COUNTER, "", "", {}});
    collector_>registerMetric({"gauge1", MetricType::GAUGE, "", "", {}});
    collector_>registerMetric({"hist1", MetricType::HISTOGRAM, "", "", {}});
    
    collector_>incrementCounter("counter1", 1.0);
    collector_>setGauge("gauge1", 2.0);
    collector_>recordHistogram("hist1", 3.0);
    
    auto all = collector_>getAllMetrics();
    
    EXPECT_GE(all.size(), 4);  // counter + gauge + hist_count + hist_sum
}

// ============================================================================
// Complex Scenario Tests
// ============================================================================

TEST_F(MetricsCollectorTest, ComplexScenarioMultipleMetrics) {
    // Simulate a realistic monitoring scenario
    collector_>registerMetric({"http_requests", MetricType::COUNTER, "HTTP requests", "", {"method", "status"}});
    collector_>registerMetric({"active_connections", MetricType::GAUGE, "Active connections", "", {}});
    collector_>registerMetric({"request_latency", MetricType::HISTOGRAM, "Request latency", "ms", {"endpoint"}});
    
    // Simulate traffic
    std::map<std::string, std::string> get200{{"method", "GET"}, {"status", "200"}};
    std::map<std::string, std::string> post201{{"method", "POST"}, {"status", "201"}};
    std::map<std::string, std::string> apiLabels{{"endpoint", "/api/v1"}};
    
    collector_>setGauge("active_connections", 100.0);
    
    for (int i = 0; i < 50; ++i) {
        collector_>incrementCounter("http_requests", 1.0, get200);
        collector_>recordHistogram("request_latency", 25.0 + (i % 50), apiLabels);
    }
    
    for (int i = 0; i < 20; ++i) {
        collector_>incrementCounter("http_requests", 1.0, post201);
    }
    
    collector_>setGauge("active_connections", 95.0);
    
    // Verify
    EXPECT_DOUBLE_EQ(collector_>getCounter("http_requests", get200), 50.0);
    EXPECT_DOUBLE_EQ(collector_>getCounter("http_requests", post201), 20.0);
    EXPECT_DOUBLE_EQ(collector_>getGauge("active_connections"), 95.0);
    
    auto hist = collector_>getHistogram("request_latency", apiLabels);
    EXPECT_EQ(hist.count, 50);
}

} // namespace
