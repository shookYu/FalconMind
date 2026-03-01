#pragma once

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <variant>
#include <nlohmann/json.hpp>
#include <mutex>
#include <functional>
#include <chrono>
#include <fstream>

namespace nodeagent {

// Configuration value types
using ConfigValue = std::variant<
    std::string,
    int,
    double,
    bool,
    std::vector<std::string>,
    nlohmann::json
>;

struct ConfigEntry {
    std::string key;
    ConfigValue value;
    std::string description;
    bool required;
    std::optional<ConfigValue> defaultValue;
    std::vector<std::string> validators;
    std::string lastModified;
    std::string source;
};

enum class ConfigSource {
    DEFAULT,
    FILE,
    ENVIRONMENT,
    COMMAND_LINE,
    REMOTE,
    RUNTIME
};

class ConfigurationManager {
public:
    using ConfigChangeCallback = std::function<void(const std::string& key, const ConfigValue& oldValue, const ConfigValue& newValue)>;
    using ConfigValidationCallback = std::function<bool(const std::string& key, const ConfigValue& value)>;

    ConfigurationManager();
    ~ConfigurationManager();

    // Initialization
    bool initialize(const std::string& configFilePath = "");
    void shutdown();
    bool isInitialized() const;

    // Schema definition
    void defineConfig(const std::string& key, 
                      const std::string& description,
                      bool required,
                      const std::optional<ConfigValue&>& defaultValue = std::nullopt,
                      const std::vector<std::string>& validators = {});
    
    // Load configuration
    bool loadFromFile(const std::string& filepath);
    bool loadFromEnvironment();
    bool loadFromCommandLine(int argc, char* argv[]);
    bool loadFromJson(const nlohmann::json& json);
    bool loadFromString(const std::string& configString);
    
    // Save configuration
    bool saveToFile(const std::string& filepath) const;
    nlohmann::json exportToJson() const;
    std::string exportToString() const;
    
    // Value access
    template<typename T>
    T get(const std::string& key) const;
    
    template<typename T>
    T getOrDefault(const std::string& key, const T& defaultValue) const;
    
    template<typename T>
    bool set(const std::string& key, const T& value, ConfigSource source = ConfigSource::RUNTIME);
    
    bool has(const std::string& key) const;
    bool isSet(const std::string& key) const;
    bool remove(const std::string& key);
    
    std::optional<ConfigValue> getValue(const std::string& key) const;
    std::vector<std::string> getAllKeys() const;
    std::vector<std::string> getModifiedKeys() const;
    
    // Validation
    bool validate() const;
    bool validateKey(const std::string& key) const;
    std::vector<std::string> getValidationErrors() const;
    void setValidationCallback(const std::string& key, ConfigValidationCallback callback);
    
    // Hot reload
    bool enableHotReload(std::chrono::milliseconds interval);
    void disableHotReload();
    bool isHotReloadEnabled() const;
    
    // Callbacks
    void setConfigChangeCallback(ConfigChangeCallback callback);
    
    // Configuration sections
    nlohmann::json getSection(const std::string& section) const;
    bool setSection(const std::string& section, const nlohmann::json& values);
    std::vector<std::string> getSections() const;
    
    // Reset
    void resetToDefaults();
    void resetKey(const std::string& key);
    
    // Metadata
    std::string getConfigSource(const std::string& key) const;
    std::string getLastModified(const std::string& key) const;
    std::string getDescription(const std::string& key) const;
    
    // Specific configuration sections
    struct OfflineAutonomyConfig {
        int heartbeatTimeoutSeconds = 10;
        int maxOfflineDurationMinutes = 30;
        int lowBatteryThreshold = 30;
        int criticalBatteryThreshold = 15;
        std::string onLowBattery = "RTL";
        std::string onCriticalBattery = "LAND";
        std::string onTimeout = "RTL";
        std::string onComplete = "RTL";
        int maxTelemetryBufferSize = 1000;
    };
    
    struct SwarmConfig {
        int heartbeatIntervalMs = 1000;
        int heartbeatTimeoutMs = 5000;
        int leaderElectionTimeoutMs = 15000;
        int partitionDetectionIntervalMs = 5000;
        int minSignalStrength = 20;
        bool enableAutoMerge = true;
        int maxPartitionDurationMinutes = 20;
    };
    
    struct CommunicationConfig {
        std::string gcsAddress = "127.0.0.1";
        int gcsPort = 9000;
        int reconnectIntervalMs = 1000;
        int maxReconnectAttempts = 10;
        bool enableEncryption = true;
        std::string certificatePath = "";
    };
    
    OfflineAutonomyConfig getOfflineAutonomyConfig() const;
    SwarmConfig getSwarmConfig() const;
    CommunicationConfig getCommunicationConfig() const;
    
    void setOfflineAutonomyConfig(const OfflineAutonomyConfig& config);
    void setSwarmConfig(const SwarmConfig& config);
    void setCommunicationConfig(const CommunicationConfig& config);

private:
    void loadDefaults();
    bool parseCommandLineArg(const std::string& arg);
    std::optional<ConfigValue> parseValue(const std::string& valueStr) const;
    std::string valueToString(const ConfigValue& value) const;
    std::string getCurrentTimestamp() const;
    void notifyConfigChange(const std::string& key, 
                           const ConfigValue& oldValue, 
                           const ConfigValue& newValue);
    void hotReloadThreadFunc();
    
    std::map<std::string, ConfigEntry> configSchema_;
    std::map<std::string, ConfigValue> configValues_;
    std::map<std::string, ConfigSource> configSources_;
    std::map<std::string, ConfigValidationCallback> validationCallbacks_;
    
    std::string configFilePath_;
    std::chrono::milliseconds hotReloadInterval_;
    std::atomic<bool> hotReloadEnabled_;
    std::thread hotReloadThread_;
    std::atomic<bool> running_;
    
    ConfigChangeCallback changeCallback_;
    mutable std::mutex mutex_;
    bool initialized_;
};

// Template implementations
template<typename T>
T ConfigurationManager::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = configValues_.find(key);
    if (it != configValues_.end()) {
        return std::get<T>(it->second);
    }
    
    // Check for default value
    auto schemaIt = configSchema_.find(key);
    if (schemaIt != configSchema_.end() && schemaIt->second.defaultValue) {
        return std::get<T>(*schemaIt->second.defaultValue);
    }
    
    throw std::runtime_error("Configuration key not found: " + key);
}

template<typename T>
T ConfigurationManager::getOrDefault(const std::string& key, const T& defaultValue) const {
    try {
        return get<T>(key);
    } catch (...) {
        return defaultValue;
    }
}

template<typename T>
bool ConfigurationManager::set(const std::string& key, const T& value, ConfigSource source) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Validate
    auto callbackIt = validationCallbacks_.find(key);
    if (callbackIt != validationCallbacks_.end()) {
        ConfigValue configValue = value;
        if (!callbackIt->second(key, configValue)) {
            return false;
        }
    }
    
    ConfigValue oldValue;
    auto it = configValues_.find(key);
    if (it != configValues_.end()) {
        oldValue = it->second;
    }
    
    ConfigValue newValue = value;
    configValues_[key] = newValue;
    configSources_[key] = source;
    
    // Update schema if exists
    auto schemaIt = configSchema_.find(key);
    if (schemaIt != configSchema_.end()) {
        schemaIt->second.lastModified = getCurrentTimestamp();
    }
    
    lock.unlock();
    notifyConfigChange(key, oldValue, newValue);
    
    return true;
}

} // namespace nodeagent
