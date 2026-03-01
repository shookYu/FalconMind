/**
 * ConfigurationManager.cpp - Production-grade configuration management
 */

#include "nodeagent/ConfigurationManager.h"
#include "nodeagent/Logger.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdlib>

namespace nodeagent {

ConfigurationManager::ConfigurationManager()
    : hotReloadInterval_(std::chrono::seconds(5))
    , hotReloadEnabled_(false)
    , running_(false)
    , initialized_(false) {
    LOG_INFO("ConfigurationManager", "Constructor called");
}

ConfigurationManager::~ConfigurationManager() {
    LOG_INFO("ConfigurationManager", "Destructor called");
    shutdown();
}

bool ConfigurationManager::initialize(const std::string& configFilePath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        LOG_WARNING("ConfigurationManager", "Already initialized");
        return true;
    }
    
    LOG_INFO("ConfigurationManager", "Initializing configuration manager");
    
    // Define configuration schema
    loadDefaults();
    
    // Load from file if specified
    if (!configFilePath.empty()) {
        configFilePath_ = configFilePath;
        if (!loadFromFile(configFilePath)) {
            LOG_WARNING("ConfigurationManager", 
                       "Failed to load config file, using defaults: " + configFilePath);
        }
    }
    
    // Override with environment variables
    loadFromEnvironment();
    
    // Validate configuration
    if (!validate()) {
        LOG_ERROR("ConfigurationManager", "Configuration validation failed");
        return false;
    }
    
    initialized_ = true;
    running_ = true;
    
    LOG_INFO("ConfigurationManager", "Initialization complete with " + 
             std::to_string(configValues_.size()) + " configuration values");
    
    return true;
}

void ConfigurationManager::shutdown() {
    LOG_INFO("ConfigurationManager", "Shutting down");
    
    running_ = false;
    disableHotReload();
    
    initialized_ = false;
    
    LOG_INFO("ConfigurationManager", "Shutdown complete");
}

bool ConfigurationManager::isInitialized() const {
    return initialized_;
}

void ConfigurationManager::loadDefaults() {
    LOG_INFO("ConfigurationManager", "Loading default configuration");
    
    // Offline Autonomy defaults
    defineConfig("autonomy.heartbeat_timeout_seconds",
                "Seconds before declaring GCS connection lost",
                false, 10);
    
    defineConfig("autonomy.max_offline_duration_minutes",
                "Maximum minutes to operate in offline mode",
                false, 30);
    
    defineConfig("autonomy.low_battery_threshold",
                "Battery percentage to trigger RTL",
                false, 30);
    
    defineConfig("autonomy.critical_battery_threshold",
                "Battery percentage for emergency landing",
                false, 15);
    
    defineConfig("autonomy.on_low_battery",
                "Action when battery is low",
                false, std::string("RTL"));
    
    defineConfig("autonomy.on_critical_battery",
                "Action when battery is critical",
                false, std::string("LAND"));
    
    defineConfig("autonomy.max_telemetry_buffer_size",
                "Maximum telemetry records to buffer",
                false, 1000);
    
    // Swarm defaults
    defineConfig("swarm.heartbeat_interval_ms",
                "Milliseconds between heartbeat broadcasts",
                false, 1000);
    
    defineConfig("swarm.heartbeat_timeout_ms",
                "Milliseconds before declaring member lost",
                false, 5000);
    
    defineConfig("swarm.leader_election_timeout_ms",
                "Milliseconds to wait for leader election",
                false, 15000);
    
    defineConfig("swarm.partition_detection_interval_ms",
                "Milliseconds between partition checks",
                false, 5000);
    
    defineConfig("swarm.min_signal_strength",
                "Minimum signal strength for connectivity (percent)",
                false, 20);
    
    defineConfig("swarm.enable_auto_merge",
                "Automatically merge partitions when connectivity restored",
                false, true);
    
    // Communication defaults
    defineConfig("communication.gcs_address",
                "Ground control station IP address",
                false, std::string("127.0.0.1"));
    
    defineConfig("communication.gcs_port",
                "Ground control station port",
                false, 9000);
    
    defineConfig("communication.reconnect_interval_ms",
                "Milliseconds between reconnection attempts",
                false, 1000);
    
    defineConfig("communication.max_reconnect_attempts",
                "Maximum reconnection attempts before giving up",
                false, 10);
    
    defineConfig("communication.enable_encryption",
                "Enable TLS encryption for communication",
                false, true);
    
    // Logging defaults
    defineConfig("logging.level",
                "Log level (TRACE, DEBUG, INFO, WARN, ERROR, FATAL)",
                false, std::string("INFO"));
    
    defineConfig("logging.enable_console",
                "Enable console logging",
                false, true);
    
    defineConfig("logging.enable_file",
                "Enable file logging",
                false, true);
    
    defineConfig("logging.file_path",
                "Path to log file",
                false, std::string("/var/log/nodeagent/nodeagent.log"));
    
    defineConfig("logging.max_file_size_mb",
                "Maximum log file size in MB before rotation",
                false, 100);
    
    defineConfig("logging.max_files",
                "Maximum number of rotated log files",
                false, 10);
    
    LOG_INFO("ConfigurationManager", "Default configuration loaded with " + 
             std::to_string(configSchema_.size()) + " schema entries");
}

void ConfigurationManager::defineConfig(const std::string& key, 
                                        const std::string& description,
                                        bool required,
                                        const std::optional<ConfigValue>& defaultValue,
                                        const std::vector<std::string>& validators) {
    ConfigEntry entry;
    entry.key = key;
    entry.description = description;
    entry.required = required;
    entry.defaultValue = defaultValue;
    entry.validators = validators;
    entry.lastModified = getCurrentTimestamp();
    entry.source = "default";
    
    configSchema_[key] = entry;
    
    // Set default value if provided
    if (defaultValue) {
        configValues_[key] = *defaultValue;
        configSources_[key] = ConfigSource::DEFAULT;
    }
}

bool ConfigurationManager::loadFromFile(const std::string& filepath) {
    LOG_INFO("ConfigurationManager", "Loading configuration from: " + filepath);
    
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("ConfigurationManager", "Failed to open config file: " + filepath);
        return false;
    }
    
    try {
        nlohmann::json json;
        file >> json;
        file.close();
        
        return loadFromJson(json);
    } catch (const std::exception& e) {
        LOG_ERROR("ConfigurationManager", 
                 "Failed to parse config file: " + std::string(e.what()));
        return false;
    }
}

bool ConfigurationManager::loadFromEnvironment() {
    LOG_INFO("ConfigurationManager", "Loading configuration from environment variables");
    
    int count = 0;
    
    for (const auto& pair : configSchema_) {
        std::string envKey = "NODEAGENT_" + pair.first;
        
        // Replace dots with underscores and convert to uppercase
        std::replace(envKey.begin(), envKey.end(), '.', '_');
        std::transform(envKey.begin(), envKey.end(), envKey.begin(), ::toupper);
        
        const char* envValue = std::getenv(envKey.c_str());
        if (envValue != nullptr) {
            auto parsedValue = parseValue(envValue);
            if (parsedValue) {
                set(pair.first, *parsedValue, ConfigSource::ENVIRONMENT);
                count++;
            }
        }
    }
    
    LOG_INFO("ConfigurationManager", 
             "Loaded " + std::to_string(count) + " values from environment");
    
    return true;
}

bool ConfigurationManager::loadFromCommandLine(int argc, char* argv[]) {
    LOG_INFO("ConfigurationManager", "Loading configuration from command line");
    
    int count = 0;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        
        if (arg.substr(0, 2) == "--") {
            if (parseCommandLineArg(arg)) {
                count++;
            }
        }
    }
    
    LOG_INFO("ConfigurationManager", 
             "Loaded " + std::to_string(count) + " values from command line");
    
    return true;
}

bool ConfigurationManager::parseCommandLineArg(const std::string& arg) {
    // Format: --key=value or --key value
    size_t pos = arg.find('=');
    
    if (pos == std::string::npos) {
        return false;
    }
    
    std::string key = arg.substr(2, pos - 2);
    std::string value = arg.substr(pos + 1);
    
    auto parsedValue = parseValue(value);
    if (parsedValue) {
        set(key, *parsedValue, ConfigSource::COMMAND_LINE);
        return true;
    }
    
    return false;
}

bool ConfigurationManager::loadFromJson(const nlohmann::json& json) {
    LOG_INFO("ConfigurationManager", "Loading configuration from JSON");
    
    int count = 0;
    
    for (auto& [key, value] : json.items()) {
        // Convert JSON value to ConfigValue
        if (value.is_string()) {
            set(key, value.get<std::string>(), ConfigSource::FILE);
        } else if (value.is_number_integer()) {
            set(key, value.get<int>(), ConfigSource::FILE);
        } else if (value.is_number_float()) {
            set(key, value.get<double>(), ConfigSource::FILE);
        } else if (value.is_boolean()) {
            set(key, value.get<bool>(), ConfigSource::FILE);
        } else if (value.is_array() && value.size() > 0 && value[0].is_string()) {
            set(key, value.get<std::vector<std::string>>(), ConfigSource::FILE);
        } else {
            set(key, value, ConfigSource::FILE);
        }
        
        count++;
    }
    
    LOG_INFO("ConfigurationManager", 
             "Loaded " + std::to_string(count) + " values from JSON");
    
    return true;
}

std::optional<ConfigValue> ConfigurationManager::parseValue(const std::string& valueStr) const {
    // Try bool
    if (valueStr == "true" || valueStr == "True" || valueStr == "TRUE") {
        return true;
    }
    if (valueStr == "false" || valueStr == "False" || valueStr == "FALSE") {
        return false;
    }
    
    // Try int
    try {
        size_t pos;
        int intValue = std::stoi(valueStr, &pos);
        if (pos == valueStr.length()) {
            return intValue;
        }
    } catch (...) {}
    
    // Try double
    try {
        size_t pos;
        double doubleValue = std::stod(valueStr, &pos);
        if (pos == valueStr.length()) {
            return doubleValue;
        }
    } catch (...) {}
    
    // Default to string
    return valueStr;
}

bool ConfigurationManager::saveToFile(const std::string& filepath) const {
    LOG_INFO("ConfigurationManager", "Saving configuration to: " + filepath);
    
    nlohmann::json json = exportToJson();
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("ConfigurationManager", "Failed to open file for writing: " + filepath);
        return false;
    }
    
    file << json.dump(2);
    file.close();
    
    LOG_INFO("ConfigurationManager", "Configuration saved successfully");
    return true;
}

nlohmann::json ConfigurationManager::exportToJson() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json json;
    
    for (const auto& pair : configValues_) {
        const ConfigValue& value = pair.second;
        
        if (std::holds_alternative<std::string>(value)) {
            json[pair.first] = std::get<std::string>(value);
        } else if (std::holds_alternative<int>(value)) {
            json[pair.first] = std::get<int>(value);
        } else if (std::holds_alternative<double>(value)) {
            json[pair.first] = std::get<double>(value);
        } else if (std::holds_alternative<bool>(value)) {
            json[pair.first] = std::get<bool>(value);
        } else if (std::holds_alternative<std::vector<std::string>>(value)) {
            json[pair.first] = std::get<std::vector<std::string>>(value);
        } else if (std::holds_alternative<nlohmann::json>(value)) {
            json[pair.first] = std::get<nlohmann::json>(value);
        }
    }
    
    return json;
}

bool ConfigurationManager::has(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return configSchema_.count(key) > 0;
}

bool ConfigurationManager::isSet(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return configValues_.count(key) > 0;
}

bool ConfigurationManager::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configValues_.find(key);
    if (it == configValues_.end()) {
        return false;
    }
    
    configValues_.erase(it);
    configSources_.erase(key);
    
    LOG_INFO("ConfigurationManager", "Removed configuration: " + key);
    return true;
}

std::optional<ConfigValue> ConfigurationManager::getValue(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configValues_.find(key);
    if (it != configValues_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<std::string> ConfigurationManager::getAllKeys() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> keys;
    for (const auto& pair : configValues_) {
        keys.push_back(pair.first);
    }
    return keys;
}

bool ConfigurationManager::validate() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    bool valid = true;
    
    for (const auto& pair : configSchema_) {
        const ConfigEntry& entry = pair.second;
        
        // Check required fields
        if (entry.required) {
            if (configValues_.count(entry.key) == 0) {
                LOG_ERROR("ConfigurationManager", 
                         "Required configuration missing: " + entry.key);
                valid = false;
            }
        }
        
        // Run custom validators if value is set
        if (configValues_.count(entry.key) > 0) {
            for (const auto& validator : entry.validators) {
                // Validator implementation would go here
                // e.g., range check, regex match, etc.
            }
        }
    }
    
    return valid;
}

void ConfigurationManager::setConfigChangeCallback(ConfigChangeCallback callback) {
    changeCallback_ = callback;
}

void ConfigurationManager::notifyConfigChange(const std::string& key, 
                                               const ConfigValue& oldValue, 
                                               const ConfigValue& newValue) {
    if (changeCallback_) {
        changeCallback_(key, oldValue, newValue);
    }
}

ConfigurationManager::OfflineAutonomyConfig ConfigurationManager::getOfflineAutonomyConfig() const {
    OfflineAutonomyConfig config;
    config.heartbeatTimeoutSeconds = getOrDefault<int>("autonomy.heartbeat_timeout_seconds", 10);
    config.maxOfflineDurationMinutes = getOrDefault<int>("autonomy.max_offline_duration_minutes", 30);
    config.lowBatteryThreshold = getOrDefault<int>("autonomy.low_battery_threshold", 30);
    config.criticalBatteryThreshold = getOrDefault<int>("autonomy.critical_battery_threshold", 15);
    config.onLowBattery = getOrDefault<std::string>("autonomy.on_low_battery", "RTL");
    config.onCriticalBattery = getOrDefault<std::string>("autonomy.on_critical_battery", "LAND");
    config.onTimeout = getOrDefault<std::string>("autonomy.on_timeout", "RTL");
    config.onComplete = getOrDefault<std::string>("autonomy.on_complete", "RTL");
    config.maxTelemetryBufferSize = getOrDefault<int>("autonomy.max_telemetry_buffer_size", 1000);
    return config;
}

ConfigurationManager::SwarmConfig ConfigurationManager::getSwarmConfig() const {
    SwarmConfig config;
    config.heartbeatIntervalMs = getOrDefault<int>("swarm.heartbeat_interval_ms", 1000);
    config.heartbeatTimeoutMs = getOrDefault<int>("swarm.heartbeat_timeout_ms", 5000);
    config.leaderElectionTimeoutMs = getOrDefault<int>("swarm.leader_election_timeout_ms", 15000);
    config.partitionDetectionIntervalMs = getOrDefault<int>("swarm.partition_detection_interval_ms", 5000);
    config.minSignalStrength = getOrDefault<int>("swarm.min_signal_strength", 20);
    config.enableAutoMerge = getOrDefault<bool>("swarm.enable_auto_merge", true);
    config.maxPartitionDurationMinutes = getOrDefault<int>("swarm.max_partition_duration_minutes", 20);
    return config;
}

ConfigurationManager::CommunicationConfig ConfigurationManager::getCommunicationConfig() const {
    CommunicationConfig config;
    config.gcsAddress = getOrDefault<std::string>("communication.gcs_address", "127.0.0.1");
    config.gcsPort = getOrDefault<int>("communication.gcs_port", 9000);
    config.reconnectIntervalMs = getOrDefault<int>("communication.reconnect_interval_ms", 1000);
    config.maxReconnectAttempts = getOrDefault<int>("communication.max_reconnect_attempts", 10);
    config.enableEncryption = getOrDefault<bool>("communication.enable_encryption", true);
    config.certificatePath = getOrDefault<std::string>("communication.certificate_path", "");
    return config;
}

std::string ConfigurationManager::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

} // namespace nodeagent
