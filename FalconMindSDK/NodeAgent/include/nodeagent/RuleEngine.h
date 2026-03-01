#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>
#include <chrono>
#include <map>
#include <variant>
#include <optional>

namespace nodeagent {

enum class RuleType {
    TIME_BASED,
    BATTERY_BASED,
    POSITION_BASED,
    COMMUNICATION_BASED,
    TASK_BASED,
    EMERGENCY
};

enum class ComparisonOperator {
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_THAN_OR_EQUAL,
    GREATER_THAN,
    GREATER_THAN_OR_EQUAL,
    IN_RANGE,
    CONTAINS
};

enum class ActionType {
    RTL,                    // Return to Launch
    LAND,                   // Land immediately
    HOVER,                  // Hover in place
    CONTINUE,               // Continue mission
    ABORT,                  // Abort mission
    EMERGENCY_LAND,         // Emergency landing
    NOTIFY,                 // Send notification
    CHANGE_STATE            // Change autonomy state
};

struct RuleCondition {
    std::string parameter;              // e.g., "battery_level", "altitude"
    ComparisonOperator op;
    std::variant<int, double, std::string, bool> value;
    std::optional<std::variant<int, double>> rangeEnd;  // For IN_RANGE
    
    bool evaluate(const std::variant<int, double, std::string, bool>& actualValue) const;
};

struct RuleAction {
    ActionType type;
    nlohmann::json parameters;
    int priority;
    std::string description;
};

struct Rule {
    std::string id;
    std::string name;
    RuleType type;
    std::vector<RuleCondition> conditions;
    std::vector<RuleAction> actions;
    bool enabled;
    int priority;
    std::string createdAt;
    std::string lastTriggered;
    int triggerCount;
    bool oneShot;
};

struct RuleExecutionContext {
    std::string uavId;
    double batteryLevel;
    double altitude;
    std::chrono::milliseconds timeSinceDisconnect;
    std::chrono::milliseconds timeSincePartition;
    std::string currentTask;
    int taskProgress;
    bool gcsConnected;
    bool swarmConnected;
    nlohmann::json telemetry;
    nlohmann::json extendedData;
};

struct RuleExecutionResult {
    bool ruleTriggered;
    std::string ruleId;
    std::vector<RuleAction> executedActions;
    std::string executionTime;
    nlohmann::json context;
};

class RuleEngine {
public:
    using RuleTriggeredCallback = std::function<void(const Rule& rule, const RuleExecutionResult& result)>;
    using ActionExecutedCallback = std::function<void(const RuleAction& action, bool success)>;

    RuleEngine();
    ~RuleEngine();

    bool initialize();
    void shutdown();

    // Rule management
    std::string addRule(const Rule& rule);
    bool removeRule(const std::string& ruleId);
    bool updateRule(const std::string& ruleId, const Rule& rule);
    bool enableRule(const std::string& ruleId);
    bool disableRule(const std::string& ruleId);
    std::optional<Rule> getRule(const std::string& ruleId) const;
    std::vector<Rule> getAllRules() const;
    std::vector<Rule> getRulesByType(RuleType type) const;
    std::vector<Rule> getEnabledRules() const;
    
    // Rule execution
    RuleExecutionResult evaluateRules(const RuleExecutionContext& context);
    bool evaluateSingleRule(const Rule& rule, const RuleExecutionContext& context);
    bool executeAction(const RuleAction& action, const RuleExecutionContext& context);
    
    // Default rules for offline autonomy
    void loadDefaultGcsDisconnectedRules();
    void loadDefaultSwarmPartitionedRules();
    void loadDefaultFullyDisconnectedRules();
    void loadDefaultEmergencyRules();
    void clearAllRules();
    
    // Configuration
    void setRuleCheckInterval(std::chrono::milliseconds interval);
    void setMaxRulesPerEvaluation(int maxRules);
    void setActionTimeout(std::chrono::milliseconds timeout);
    
    // Statistics
    struct Statistics {
        int totalRules;
        int enabledRules;
        int totalEvaluations;
        int totalTriggers;
        int totalActionsExecuted;
        int totalActionsFailed;
        std::chrono::milliseconds averageEvaluationTime;
    };
    Statistics getStatistics() const;
    void resetStatistics();
    
    // Callbacks
    void setRuleTriggeredCallback(RuleTriggeredCallback callback);
    void setActionExecutedCallback(ActionExecutedCallback callback);
    
    // Import/Export
    bool exportRulesToFile(const std::string& filepath) const;
    bool importRulesFromFile(const std::string& filepath);
    nlohmann::json serializeRules() const;
    bool deserializeRules(const nlohmann::json& json);

private:
    bool evaluateCondition(const RuleCondition& condition, const RuleExecutionContext& context);
    bool executeRtlAction(const nlohmann::json& params);
    bool executeLandAction(const nlohmann::json& params);
    bool executeHoverAction(const nlohmann::json& params);
    bool executeContinueAction(const nlohmann::json& params);
    bool executeAbortAction(const nlohmann::json& params);
    bool executeEmergencyLandAction(const nlohmann::json& params);
    bool executeNotifyAction(const nlohmann::json& params);
    bool executeChangeStateAction(const nlohmann::json& params);
    
    std::variant<int, double, std::string, bool> getParameterValue(
        const std::string& parameter, 
        const RuleExecutionContext& context
    ) const;
    
    std::string generateRuleId();
    std::string getCurrentTimestamp() const;
    void sortRulesByPriority();

    std::map<std::string, Rule> rules_;
    std::chrono::milliseconds checkInterval_;
    int maxRulesPerEvaluation_;
    std::chrono::milliseconds actionTimeout_;
    
    RuleTriggeredCallback ruleTriggeredCallback_;
    ActionExecutedCallback actionExecutedCallback_;
    
    Statistics stats_;
    mutable std::mutex mutex_;
    bool initialized_;
    int nextRuleId_;
};

} // namespace nodeagent
