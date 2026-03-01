#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>
#include <functional>

namespace nodeagent {

enum class MetricType {
    COUNTER,      // Monotonically increasing
    GAUGE,        // Can go up or down
    HISTOGRAM,    // Distribution of values
    TIMER         // Time duration
};

struct MetricValue {
    std::string name;
    MetricType type;
    double value;
    std::chrono::steady_clock::time_point timestamp;
    std::map<std::string, std::string> labels;
    std::string unit;
};

struct MetricDefinition {
    std::string name;
    MetricType type;
    std::string description;
    std::string unit;
    std::vector<std::string> labelNames;
};

struct HistogramBucket {
    double upperBound;
    uint64_t count;
};

struct HistogramData {
    std::vector<HistogramBucket> buckets;
    double sum;
    uint64_t count;
};

class MetricsCollector {
public:
    using MetricsExportCallback = std::function<void(const std::vector<MetricValue>& metrics)>;
    using AlertCallback = std::function<void(const std::string& metricName, double value, const std::string& threshold)>;

    MetricsCollector();
    ~MetricsCollector();

    // Initialization
    bool initialize();
    void shutdown();

    // Metric registration
    void registerMetric(const MetricDefinition& definition);
    void unregisterMetric(const std::string& name);
    bool isMetricRegistered(const std::string& name) const;
    std::vector<MetricDefinition> getRegisteredMetrics() const;

    // Metric recording
    void incrementCounter(const std::string& name, 
                          double amount = 1.0,
                          const std::map<std::string, std::string>& labels = {});
    
    void setGauge(const std::string& name, 
                  double value,
                  const std::map<std::string, std::string>& labels = {});
    
    void recordHistogram(const std::string& name, 
                         double value,
                         const std::map<std::string, std::string>& labels = {});
    
    void startTimer(const std::string& name,
                    const std::map<std::string, std::string>& labels = {});
    
    void stopTimer(const std::string& name,
                   const std::map<std::string, std::string>& labels = {});

    // Metric retrieval
    double getCounter(const std::string& name, 
                      const std::map<std::string, std::string>& labels = {}) const;
    
    double getGauge(const std::string& name,
                    const std::map<std::string, std::string>& labels = {}) const;
    
    HistogramData getHistogram(const std::string& name,
                               const std::map<std::string, std::string>& labels = {}) const;

    // Batch operations
    std::vector<MetricValue> getAllMetrics() const;
    nlohmann::json exportToJson() const;
    std::string exportToPrometheusFormat() const;
    void clearAllMetrics();

    // Alerting
    void setAlertThreshold(const std::string& metricName, 
                           double threshold,
                           const std::string& comparison = ">");
    void removeAlertThreshold(const std::string& metricName);
    void setAlertCallback(AlertCallback callback);
    void checkAlerts();

    // Export
    void setExportCallback(MetricsExportCallback callback);
    bool enablePeriodicExport(std::chrono::seconds interval);
    void disablePeriodicExport();

    // System metrics
    void recordSystemMetrics();
    double getCpuUsage() const;
    double getMemoryUsage() const;
    double getDiskUsage() const;

    // Component-specific metrics helpers
    void recordTelemetryMetrics(size_t bufferSize, size_t syncedCount, size_t unsyncedCount);
    void recordTaskMetrics(const std::string& taskType, 
                          std::chrono::milliseconds duration,
                          bool success);
    void recordConnectionMetrics(const std::string& endpoint,
                                 bool connected,
                                 double latencyMs);
    void recordPartitionMetrics(int partitionCount, 
                                int totalMembers,
                                int activeMembers);
    void recordLeaderElectionMetrics(std::chrono::milliseconds electionTime, 
                                     bool success);
    void recordRuleEvaluationMetrics(int rulesEvaluated,
                                     int rulesTriggered,
                                     std::chrono::milliseconds evaluationTime);

    // Statistics
    struct Statistics {
        uint64_t totalMetricsRecorded;
        uint64_t totalCounterIncrements;
        uint64_t totalGaugeSets;
        uint64_t totalHistogramRecords;
        uint64_t totalTimerRecords;
        uint64_t totalAlertsTriggered;
        std::chrono::seconds uptime;
    };
    Statistics getStatistics() const;

private:
    std::string getMetricKey(const std::string& name, 
                            const std::map<std::string, std::string>& labels) const;
    void checkAlert(const std::string& name, double value);
    void exportThreadFunc();
    void collectSystemMetrics();
    
    std::map<std::string, MetricDefinition> metricDefinitions_;
    std::map<std::string, double> counters_;
    std::map<std::string, double> gauges_;
    std::map<std::string, HistogramData> histograms_;
    std::map<std::string, std::chrono::steady_clock::time_point> timers_;
    
    std::map<std::string, std::pair<double, std::string>> alertThresholds_;
    AlertCallback alertCallback_;
    MetricsExportCallback exportCallback_;
    
    std::atomic<bool> running_;
    std::atomic<bool> periodicExportEnabled_;
    std::chrono::seconds exportInterval_;
    std::thread exportThread_;
    
    std::chrono::steady_clock::time_point startTime_;
    
    mutable std::mutex mutex_;
    
    Statistics stats_;
};

} // namespace nodeagent
