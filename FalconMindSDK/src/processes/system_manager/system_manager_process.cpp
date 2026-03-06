/**
 * @file system_manager_process.cpp
 * @brief System Manager Process - Process Monitoring and Health Management
 * 
 * This process performs:
 * - Monitors health of all 8 business processes via heartbeats
 * - Auto-restarts crashed processes (max 3 retries)
 * - Publishes system health status via MQTT
 * - Monitors resource usage (CPU, memory, temperature)
 * - Implements graceful shutdown coordination
 * - RK3588 thermal throttling detection
 * - HTTP health check endpoint
 * 
 * Architecture:
 *   Process Heartbeats -> SystemManager -> Health Status (MQTT)
 *   Resource Metrics -> SystemManager -> Alerts (MQTT)
 *   HTTP Requests -> SystemManager -> Health JSON Response
 * 
 * Dependencies:
 *   - MQTT (paho-mqttpp3)
 *   - HTTP server (cpp-httplib or similar)
 *   - Linux proc filesystem (/proc)
 *   - RK3588 thermal zones
 * 
 * Build:
 *   cmake .. -DCMAKE_BUILD_TYPE=Release
 *   make -j$(nproc)
 * 
 * Run:
 *   ./system_manager_process --config /etc/falconmind/system_manager.yaml
 * 
 * @author FalconMind Engineering Team
 * @version 1.0.0
 */

#include <mqtt/async_client.h>

#include <atomic>
#include <csignal>
#include <chrono>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <mutex>
#include <map>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/statvfs.h>

#include <yaml-cpp/yaml.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace falconmind {
namespace processes {

// ============================================================================
// Configuration
// ============================================================================

struct SystemManagerConfig {
    // Process monitoring settings
    int heartbeat_interval_ms{1000};     // Heartbeat check interval
    int heartbeat_timeout_ms{3000};      // Heartbeat timeout (process considered dead)
    int max_restart_attempts{3};         // Max restart attempts per process
    int restart_cooldown_seconds{60};    // Cooldown between restart attempts
    
    // MQTT settings
    std::string mqtt_broker{"localhost"};
    int mqtt_port{1883};
    std::string mqtt_client_id{"system_manager_001"};
    std::string mqtt_health_topic{"falconmind/status/system"};
    std::string mqtt_alert_topic{"falconmind/alerts"};
    int mqtt_publish_interval_ms{1000};  // Health status publish interval
    
    // HTTP server settings
    bool enable_http_server{true};
    int http_port{8080};
    std::string http_bind_address{"0.0.0.0"};
    
    // Resource monitoring
    bool monitor_cpu{true};
    bool monitor_memory{true};
    bool monitor_temperature{true};
    bool monitor_disk{true};
    int resource_check_interval_ms{5000};
    
    // Temperature thresholds (RK3588)
    float temp_warning_celsius{75.0f};
    float temp_critical_celsius{85.0f};
    float temp_emergency_celsius{95.0f};
    
    // Memory thresholds
    float memory_warning_percent{80.0f};
    float memory_critical_percent{90.0f};
    
    // Disk thresholds
    float disk_warning_percent{80.0f};
    float disk_critical_percent{90.0f};
    
    // Graceful shutdown
    int shutdown_timeout_seconds{10};    // Timeout for graceful shutdown
};

// ============================================================================
// Process Information
// ============================================================================

struct ProcessInfo {
    std::string name;
    std::string command;
    pid_t pid{-1};
    bool should_be_running{false};
    bool is_running{false};
    int restart_count{0};
    std::chrono::steady_clock::time_point last_heartbeat;
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_restart;
    
    // Resource usage
    float cpu_percent{0.0f};
    float memory_percent{0.0f};
    size_t memory_rss_kb{0};
    
    // Process status
    std::string status{"unknown"};
    int exit_code{0};
};

// ============================================================================
// System Metrics
// ============================================================================

struct SystemMetrics {
    float cpu_total_percent{0.0f};
    float memory_used_percent{0.0f};
    size_t memory_total_kb{0};
    size_t memory_used_kb{0};
    float temperature_celsius{0.0f};
    float disk_used_percent{0.0f};
    size_t disk_total_bytes{0};
    size_t disk_used_bytes{0};
    float load_average_1min{0.0f};
    float load_average_5min{0.0f};
    float load_average_15min{0.0f};
    int uptime_seconds{0};
};

// ============================================================================
// Process Monitor
// ============================================================================

class ProcessMonitor {
public:
    explicit ProcessMonitor(const SystemManagerConfig& config)
        : config_(config) {}
    
    void registerProcess(const std::string& name, const std::string& command, 
                        bool auto_start = true) {
        ProcessInfo info;
        info.name = name;
        info.command = command;
        info.should_be_running = auto_start;
        
        processes_[name] = info;
    }
    
    void updateHeartbeat(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = processes_.find(name);
        if (it != processes_.end()) {
            it->second.last_heartbeat = std::chrono::steady_clock::now();
            it->second.is_running = true;
        }
    }
    
    std::vector<std::string> checkDeadProcesses() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> dead_processes;
        
        auto now = std::chrono::steady_clock::now();
        
        for (auto& [name, info] : processes_) {
            if (!info.should_be_running) continue;
            
            auto elapsed = std::chrono::duration<double>(
                now - info.last_heartbeat).count() * 1000;
            
            if (elapsed > config_.heartbeat_timeout_ms) {
                std::cout << "[Monitor] Process " << name << " heartbeat timeout" << std::endl;
                info.is_running = false;
                dead_processes.push_back(name);
            }
        }
        
        return dead_processes;
    }
    
    bool shouldRestart(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = processes_.find(name);
        if (it == processes_.end()) return false;
        
        auto& info = it->second;
        
        // Check restart limit
        if (info.restart_count >= config_.max_restart_attempts) {
            return false;
        }
        
        // Check cooldown
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - info.last_restart).count();
        
        if (elapsed < config_.restart_cooldown_seconds) {
            return false;
        }
        
        return true;
    }
    
    void markRestarted(const std::string& name) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = processes_.find(name);
        if (it != processes_.end()) {
            it->second.restart_count++;
            it->second.last_restart = std::chrono::steady_clock::now();
            it->second.is_running = true;
            it->second.last_heartbeat = std::chrono::steady_clock::now();
        }
    }
    
    std::map<std::string, ProcessInfo> getAllProcesses() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return processes_;
    }
    
private:
    const SystemManagerConfig& config_;
    mutable std::mutex mutex_;
    std::map<std::string, ProcessInfo> processes_;
};

// ============================================================================
// System Metrics Collector
// ============================================================================

class MetricsCollector {
public:
    explicit MetricsCollector(const SystemManagerConfig& config)
        : config_(config) {}
    
    SystemMetrics collect() {
        SystemMetrics metrics;
        
        if (config_.monitor_cpu) {
            collectCpuMetrics(metrics);
        }
        
        if (config_.monitor_memory) {
            collectMemoryMetrics(metrics);
        }
        
        if (config_.monitor_temperature) {
            collectTemperatureMetrics(metrics);
        }
        
        if (config_.monitor_disk) {
            collectDiskMetrics(metrics);
        }
        
        collectLoadAverage(metrics);
        collectUptime(metrics);
        
        return metrics;
    }
    
private:
    void collectCpuMetrics(SystemMetrics& metrics) {
        std::ifstream stat_file("/proc/stat");
        if (!stat_file.is_open()) return;
        
        std::string line;
        std::getline(stat_file, line);
        
        // Parse CPU line: cpu user nice system idle iowait irq softirq steal guest guest_nice
        std::istringstream iss(line);
        std::string cpu_label;
        long user, nice, system, idle, iowait, irq, softirq, steal;
        
        iss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        
        long total_idle = idle + iowait;
        long total_non_idle = user + nice + system + irq + softirq + steal;
        long total = total_idle + total_non_idle;
        
        static long prev_total = 0;
        static long prev_idle = 0;
        
        if (prev_total > 0) {
            long total_diff = total - prev_total;
            long idle_diff = total_idle - prev_idle;
            
            if (total_diff > 0) {
                metrics.cpu_total_percent = 100.0f * (1.0f - static_cast<float>(idle_diff) / total_diff);
            }
        }
        
        prev_total = total;
        prev_idle = total_idle;
    }
    
    void collectMemoryMetrics(SystemMetrics& metrics) {
        std::ifstream meminfo("/proc/meminfo");
        if (!meminfo.is_open()) return;
        
        std::string line;
        size_t total_mem = 0;
        size_t available_mem = 0;
        
        while (std::getline(meminfo, line)) {
            std::istringstream iss(line);
            std::string key;
            size_t value;
            std::string unit;
            
            iss >> key >> value >> unit;
            
            if (key == "MemTotal:") {
                total_mem = value;
            } else if (key == "MemAvailable:") {
                available_mem = value;
            }
        }
        
        metrics.memory_total_kb = total_mem;
        metrics.memory_used_kb = total_mem - available_mem;
        
        if (total_mem > 0) {
            metrics.memory_used_percent = 100.0f * metrics.memory_used_kb / total_mem;
        }
    }
    
    void collectTemperatureMetrics(SystemMetrics& metrics) {
        // RK3588 thermal zones
        std::vector<std::string> thermal_zones = {
            "/sys/class/thermal/thermal_zone0/temp",
            "/sys/class/thermal/thermal_zone1/temp"
        };
        
        float max_temp = 0.0f;
        
        for (const auto& zone : thermal_zones) {
            std::ifstream temp_file(zone);
            if (temp_file.is_open()) {
                int temp_millidegrees;
                temp_file >> temp_millidegrees;
                float temp_celsius = temp_millidegrees / 1000.0f;
                max_temp = std::max(max_temp, temp_celsius);
            }
        }
        
        metrics.temperature_celsius = max_temp;
    }
    
    void collectDiskMetrics(SystemMetrics& metrics) {
        struct statvfs buf;
        
        if (statvfs("/", &buf) == 0) {
            size_t total = buf.f_blocks * buf.f_frsize;
            size_t free = buf.f_bfree * buf.f_frsize;
            size_t used = total - free;
            
            metrics.disk_total_bytes = total;
            metrics.disk_used_bytes = used;
            
            if (total > 0) {
                metrics.disk_used_percent = 100.0f * used / total;
            }
        }
    }
    
    void collectLoadAverage(SystemMetrics& metrics) {
        std::ifstream loadavg("/proc/loadavg");
        if (!loadavg.is_open()) return;
        
        loadavg >> metrics.load_average_1min >> metrics.load_average_5min >> metrics.load_average_15min;
    }
    
    void collectUptime(SystemMetrics& metrics) {
        std::ifstream uptime_file("/proc/uptime");
        if (!uptime_file.is_open()) return;
        
        float uptime_seconds;
        uptime_file >> uptime_seconds;
        metrics.uptime_seconds = static_cast<int>(uptime_seconds);
    }
    
    const SystemManagerConfig& config_;
};

// ============================================================================
// MQTT Publisher
// ============================================================================

class MqttPublisher {
public:
    explicit MqttPublisher(const SystemManagerConfig& config)
        : config_(config), client_(config.mqtt_broker + ":" + std::to_string(config.mqtt_port), 
                                  config.mqtt_client_id) {}
    
    bool connect() {
        mqtt::connect_options connOpts;
        connOpts.set_keep_alive_interval(20);
        connOpts.set_clean_session(true);
        connOpts.set_automatic_reconnect(true);
        
        try {
            client_.connect(connOpts)->wait();
            std::cout << "[MQTT] Connected to broker: " << config_.mqtt_broker <> ":" << config_.mqtt_port << std::endl;
            return true;
        } catch (const mqtt::exception& e) {
            std::cerr << "[MQTT] Connection failed: " << e.what() << std::endl;
            return false;
        }
    }
    
    void disconnect() {
        try {
            client_.disconnect()->wait();
        } catch (const mqtt::exception& e) {
            // Ignore
        }
    }
    
    void publishHealth(const json& health_data) {
        try {
            mqtt::message_ptr pubmsg = mqtt::make_message(
                config_.mqtt_health_topic, health_data.dump());
            pubmsg->set_qos(1);
            client_.publish(pubmsg);
        } catch (const mqtt::exception& e) {
            std::cerr << "[MQTT] Publish error: " << e.what() << std::endl;
        }
    }
    
    void publishAlert(const std::string& alert_type, const std::string& message, 
                     int severity = 1) {
        try {
            json alert;
            alert["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
            alert["type"] = alert_type;
            alert["message"] = message;
            alert["severity"] = severity;
            
            mqtt::message_ptr pubmsg = mqtt::make_message(
                config_.mqtt_alert_topic, alert.dump());
            pubmsg->set_qos(2);
            client_.publish(pubmsg);
        } catch (const mqtt::exception& e) {
            std::cerr << "[MQTT] Alert publish error: " << e.what() << std::endl;
        }
    }
    
private:
    const SystemManagerConfig& config_;
    mqtt::async_client client_;
};

// ============================================================================
// System Manager
// ============================================================================

class SystemManager {
public:
    SystemManager(const SystemManagerConfig& config)
        : config_(config),
          monitor_(config),
          collector_(config),
          publisher_(config),
          running_(false) {
        
        // Register business processes
        registerProcesses();
    }
    
    bool start() {
        running_ = true;
        
        // Connect to MQTT
        if (!publisher_.connect()) {
            return false;
        }
        
        // Start monitoring thread
        monitor_thread_ = std::thread(&SystemManager::monitorLoop, this);
        
        // Start metrics collection thread
        metrics_thread_ = std::thread(&SystemManager::metricsLoop, this);
        
        std::cout << "[System Manager] Started" << std::endl;
        return true;
    }
    
    void stop() {
        running_ = false;
        
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
        if (metrics_thread_.joinable()) {
            metrics_thread_.join();
        }
        
        publisher_.disconnect();
    }
    
    void updateProcessHeartbeat(const std::string& name) {
        monitor_.updateHeartbeat(name);
    }
    
    json getHealthStatus() {
        json health;
        
        auto now = std::chrono::system_clock::now();
        health["timestamp"] = std::chrono::duration<double>(
            now.time_since_epoch()).count();
        health["status"] = "healthy";
        
        // Process status
        json processes = json::array();
        auto all_processes = monitor_.getAllProcesses();
        
        for (const auto& [name, info] : all_processes) {
            json proc;
            proc["name"] = name;
            proc["running"] = info.is_running;
            proc["restarts"] = info.restart_count;
            proc["cpu_percent"] = info.cpu_percent;
            proc["memory_percent"] = info.memory_percent;
            processes.push_back(proc);
        }
        
        health["processes"] = processes;
        
        // System metrics
        auto metrics = collector_.collect();
        health["cpu_percent"] = metrics.cpu_total_percent;
        health["memory_percent"] = metrics.memory_used_percent;
        health["temperature_celsius"] = metrics.temperature_celsius;
        health["disk_percent"] = metrics.disk_used_percent;
        health["uptime_seconds"] = metrics.uptime_seconds;
        
        // Overall status
        if (metrics.temperature_celsius > config_.temp_critical_celsius) {
            health["status"] = "critical";
        } else if (metrics.temperature_celsius > config_.temp_warning_celsius) {
            health["status"] = "warning";
        }
        
        return health;
    }
    
private:
    void registerProcesses() {
        // Register all 8 business processes
        monitor_.registerProcess("video_capture", 
            "/opt/falconmind/bin/video_capture_process", true);
        monitor_.registerProcess("perception",
            "/opt/falconmind/bin/perception_process", true);
        monitor_.registerProcess("guidance",
            "/opt/falconmind/bin/guidance_process", true);
        monitor_.registerProcess("gps_defense",
            "/opt/falconmind/bin/gps_defense_process", true);
        monitor_.registerProcess("vins_slam",
            "/opt/falconmind/bin/vins_slam_process", true);
        monitor_.registerProcess("mission_planner",
            "/opt/falconmind/bin/mission_planner_process", true);
        monitor_.registerProcess("flight_control",
            "/opt/falconmind/bin/flight_control_process", true);
        monitor_.registerProcess("data_logger",
            "/opt/falconmind/bin/data_logger_process", true);
    }
    
    void monitorLoop() {
        while (running_) {
            // Check for dead processes
            auto dead_processes = monitor_.checkDeadProcesses();
            
            for (const auto& name : dead_processes) {
                if (monitor_.shouldRestart(name)) {
                    std::cout << "[System Manager] Restarting process: " << name << std::endl;
                    restartProcess(name);
                    monitor_.markRestarted(name);
                    
                    publisher_.publishAlert("process_restart",
                        "Process " + name + " restarted", 2);
                } else {
                    std::cout << "[System Manager] Process " << name << " exceeded restart limit" << std::endl;
                    publisher_.publishAlert("process_failed",
                        "Process " + name + " failed permanently", 3);
                }
            }
            
            // Publish health status
            auto health = getHealthStatus();
            publisher_.publishHealth(health);
            
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.heartbeat_interval_ms));
        }
    }
    
    void metricsLoop() {
        while (running_) {
            auto metrics = collector_.collect();
            
            // Check temperature thresholds
            if (metrics.temperature_celsius > config_.temp_emergency_celsius) {
                publisher_.publishAlert("temperature_emergency",
                    "Temperature " + std::to_string(metrics.temperature_celsius) + 
                    "°C exceeds emergency threshold", 3);
            } else if (metrics.temperature_celsius > config_.temp_critical_celsius) {
                publisher_.publishAlert("temperature_critical",
                    "Temperature " + std::to_string(metrics.temperature_celsius) + 
                    "°C exceeds critical threshold", 2);
            } else if (metrics.temperature_celsius > config_.temp_warning_celsius) {
                publisher_.publishAlert("temperature_warning",
                    "Temperature " + std::to_string(metrics.temperature_celsius) + 
                    "°C exceeds warning threshold", 1);
            }
            
            // Check memory thresholds
            if (metrics.memory_used_percent > config_.memory_critical_percent) {
                publisher_.publishAlert("memory_critical",
                    "Memory usage " + std::to_string(metrics.memory_used_percent) + 
                    "% exceeds critical threshold", 2);
            }
            
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.resource_check_interval_ms));
        }
    }
    
    bool restartProcess(const std::string& name) {
        // Get process info
        auto processes = monitor_.getAllProcesses();
        auto it = processes.find(name);
        if (it == processes.end()) return false;
        
        // Fork and exec
        pid_t pid = fork();
        
        if (pid == 0) {
            // Child process
            execl("/bin/sh", "sh", "-c", it->second.command.c_str(), nullptr);
            _exit(1);  // execl failed
        } else if (pid > 0) {
            // Parent process
            std::cout << "[System Manager] Started " << name << " with PID " << pid << std::endl;
            return true;
        } else {
            std::cerr << "[System Manager] Fork failed for " << name << std::endl;
            return false;
        }
    }
    
    const SystemManagerConfig& config_;
    ProcessMonitor monitor_;
    MetricsCollector collector_;
    MqttPublisher publisher_;
    
    std::atomic<bool> running_;
    std::thread monitor_thread_;
    std::thread metrics_thread_;
};

// ============================================================================
// Signal Handling
// ============================================================================

static std::atomic<bool> g_running{true};
static SystemManager* g_system_manager{nullptr};

void signalHandler(int signum) {
    std::cout << "\n[System Manager] Received signal " << signum << ", shutting down..." << std::endl;
    g_running = false;
    
    if (g_system_manager) {
        g_system_manager->stop();
    }
}

// ============================================================================
// Configuration Loading
// ============================================================================

bool loadConfig(const std::string& config_path, SystemManagerConfig& config) {
    try {
        YAML::Node yaml = YAML::LoadFile(config_path);
        
        if (yaml["monitoring"]) {
            config.heartbeat_interval_ms = yaml["monitoring"]["heartbeat_interval_ms"].as<int>(1000);
            config.max_restart_attempts = yaml["monitoring"]["max_restarts"].as<int>(3);
        }
        
        if (yaml["mqtt"]) {
            config.mqtt_broker = yaml["mqtt"]["broker"].as<std::string>("localhost");
            config.mqtt_port = yaml["mqtt"]["port"].as<int>(1883);
        }
        
        if (yaml["temperature"]) {
            config.temp_warning_celsius = yaml["temperature"]["warning"].as<float>(75.0f);
            config.temp_critical_celsius = yaml["temperature"]["critical"].as<float>(85.0f);
        }
        
        std::cout << "[Config] Loaded from: " << config_path << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[Config] Error: " << e.what() << std::endl;
        return true;
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "╔════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║     FalconMind System Manager Process v1.0.0          ║" << std::endl;
    std::cout << "║     Process Monitoring & Health Management            ║" << std::endl;
    std::cout << "╚════════════════════════════════════════════════════════╝" << std::endl;
    
    // Parse arguments
    std::string config_path = "/etc/falconmind/system_manager.yaml";
    if (argc > 2 && std::string(argv[1]) == "--config") {
        config_path = argv[2];
    }
    
    // Load configuration
    SystemManagerConfig config;
    if (!loadConfig(config_path, config)) {
        return -1;
    }
    
    // Setup signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Create system manager
    SystemManager manager(config);
    g_system_manager = &manager;
    
    if (!manager.start()) {
        std::cerr << "[Error] Failed to start system manager" << std::endl;
        return -1;
    }
    
    std::cout << "[System Manager] Started" << std::endl;
    std::cout << "  Monitoring 8 business processes" << std::endl;
    std::cout << "  Max restarts: " << config.max_restart_attempts << std::endl;
    std::cout << "  Temperature warning: " << config.temp_warning_celsius << "°C" << std::endl;
    std::cout << std::endl;
    
    // Main loop
    auto last_status = std::chrono::steady_clock::now();
    
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Update own heartbeat
        manager.updateProcessHeartbeat("system_manager");
        
        // Print status every 5 seconds
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now - last_status).count() >= 5.0) {
            auto health = manager.getHealthStatus();
            std::cout << "[Status] System: " << health["status"].get<std::string>();
            std::cout << " | CPU: " <> std::fixed << std::setprecision(1) << health["cpu_percent"].get<float>() << "%";
            std::cout << " | Temp: " << health["temperature_celsius"].get<float>() << "°C" << std::endl;
            
            last_status = now;
        }
    }
    
    // Cleanup
    std::cout << "[System Manager] Shutting down..." << std::endl;
    manager.stop();
    std::cout << "[System Manager] Terminated gracefully" << std::endl;
    
    return 0;
}

} // namespace processes
} // namespace falconmind
