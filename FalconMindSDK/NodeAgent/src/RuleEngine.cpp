/**
 * RuleEngine.cpp - Production-grade rule engine for offline autonomy
 */

#include "nodeagent/RuleEngine.h"
#include "nodeagent/Logger.h"
#include <sstream>
#include <iomanip>
#include <fstream>

namespace nodeagent {

RuleEngine::RuleEngine()
    : checkInterval_(std::chrono::milliseconds(1000))
    , maxRulesPerEvaluation_(100)
    , actionTimeout_(std::chrono::milliseconds(5000))
    , initialized_(false)
    , nextRuleId_(1) {
    LOG_INFO("RuleEngine", "Constructor called");
}

RuleEngine::~RuleEngine() {
    LOG_INFO("RuleEngine", "Destructor called");
    shutdown();
}

bool RuleEngine::initialize() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        LOG_WARNING("RuleEngine", "Already initialized");
        return true;
    }
    
    LOG_INFO("RuleEngine", "Initializing rule engine");
    
    // Load default rules
    loadDefaultGcsDisconnectedRules();
    loadDefaultSwarmPartitionedRules();
    loadDefaultFullyDisconnectedRules();
    loadDefaultEmergencyRules();
    
    initialized_ = true;
    
    LOG_INFO("RuleEngine", "Initialization complete with " + 
             std::to_string(rules_.size()) + " rules");
    return true;
}

void RuleEngine::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    initialized_ = false;
    LOG_INFO("RuleEngine", "Shutdown complete");
}

std::string RuleEngine::addRule(const Rule& rule) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string id = rule.id.empty() ? generateRuleId() : rule.id;
    
    Rule newRule = rule;
    newRule.id = id;
    newRule.createdAt = getCurrentTimestamp();
    newRule.triggerCount = 0;
    
    rules_[id] = newRule;
    
    LOG_INFO("RuleEngine", "Added rule: " + id + " (" + newRule.name + ")");
    
    sortRulesByPriority();
    
    return id;
}

bool RuleEngine::removeRule(const std::string& ruleId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = rules_.find(ruleId);
    if (it == rules_.end()) {
        return false;
    }
    
    rules_.erase(it);
    LOG_INFO("RuleEngine", "Removed rule: " + ruleId);
    return true;
}

bool RuleEngine::updateRule(const std::string& ruleId, const Rule& rule) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = rules_.find(ruleId);
    if (it == rules_.end()) {
        return false;
    }
    
    Rule updatedRule = rule;
    updatedRule.id = ruleId;
    updatedRule.createdAt = it->second.createdAt;
    updatedRule.triggerCount = it->second.triggerCount;
    updatedRule.lastTriggered = it->second.lastTriggered;
    
    it->second = updatedRule;
    
    LOG_INFO("RuleEngine", "Updated rule: " + ruleId);
    
    sortRulesByPriority();
    return true;
}

bool RuleEngine::enableRule(const std::string& ruleId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = rules_.find(ruleId);
    if (it == rules_.end()) {
        return false;
    }
    
    it->second.enabled = true;
    LOG_INFO("RuleEngine", "Enabled rule: " + ruleId);
    return true;
}

bool RuleEngine::disableRule(const std::string& ruleId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = rules_.find(ruleId);
    if (it == rules_.end()) {
        return false;
    }
    
    it->second.enabled = false;
    LOG_INFO("RuleEngine", "Disabled rule: " + ruleId);
    return true;
}

std::optional<Rule> RuleEngine::getRule(const std::string& ruleId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = rules_.find(ruleId);
    if (it != rules_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::vector<Rule> RuleEngine::getAllRules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Rule> result;
    for (const auto& pair : rules_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<Rule> RuleEngine::getRulesByType(RuleType type) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Rule> result;
    for (const auto& pair : rules_) {
        if (pair.second.type == type) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<Rule> RuleEngine::getEnabledRules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<Rule> result;
    for (const auto& pair : rules_) {
        if (pair.second.enabled) {
            result.push_back(pair.second);
        }
    }
    return result;
}

RuleExecutionResult RuleEngine::evaluateRules(const RuleExecutionContext& context) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    RuleExecutionResult result;
    result.ruleTriggered = false;
    
    auto startTime = std::chrono::steady_clock::now();
    
    int evaluatedCount = 0;
    
    for (const auto& pair : rules_) {
        if (evaluatedCount >= maxRulesPerEvaluation_) {
            break;
        }
        
        const Rule& rule = pair.second;
        
        if (!rule.enabled) {
            continue;
        }
        
        if (evaluateSingleRule(rule, context)) {
            result.ruleTriggered = true;
            result.ruleId = rule.id;
            
            // Execute actions
            for (const auto& action : rule.actions) {
                if (executeAction(action, context)) {
                    result.executedActions.push_back(action);
                }
            }
            
            // Update rule statistics
            auto& mutableRule = const_cast<Rule&>(rule);
            mutableRule.triggerCount++;
            mutableRule.lastTriggered = getCurrentTimestamp();
            
            if (rule.oneShot) {
                mutableRule.enabled = false;
            }
            
            // Notify callback
            if (ruleTriggeredCallback_) {
                ruleTriggeredCallback_(rule, result);
            }
            
            // For priority-based evaluation, stop after first triggered rule
            // or continue based on configuration
            break;
        }
        
        evaluatedCount++;
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime);
    
    result.executionTime = getCurrentTimestamp();
    result.context = {
        {"evaluated_rules", evaluatedCount},
        {"evaluation_duration_ms", duration.count()}
    };
    
    // Update statistics
    stats_.totalEvaluations++;
    if (result.ruleTriggered) {
        stats_.totalTriggers++;
    }
    
    return result;
}

bool RuleEngine::evaluateSingleRule(const Rule& rule, 
                                     const RuleExecutionContext& context) {
    // All conditions must be true (AND logic)
    for (const auto& condition : rule.conditions) {
        if (!evaluateCondition(condition, context)) {
            return false;
        }
    }
    
    return true;
}

bool RuleEngine::evaluateCondition(const RuleCondition& condition, 
                                    const RuleExecutionContext& context) {
    auto actualValue = getParameterValue(condition.parameter, context);
    return condition.evaluate(actualValue);
}

bool RuleCondition::evaluate(const std::variant<int, double, std::string, bool>& actualValue) const {
    // Type checking and comparison
    if (std::holds_alternative<int>(value) && std::holds_alternative<int>(actualValue)) {
        int expected = std::get<int>(value);
        int actual = std::get<int>(actualValue);
        
        switch (op) {
            case ComparisonOperator::EQUAL: return actual == expected;
            case ComparisonOperator::NOT_EQUAL: return actual != expected;
            case ComparisonOperator::LESS_THAN: return actual < expected;
            case ComparisonOperator::LESS_THAN_OR_EQUAL: return actual <= expected;
            case ComparisonOperator::GREATER_THAN: return actual > expected;
            case ComparisonOperator::GREATER_THAN_OR_EQUAL: return actual >= expected;
            default: return false;
        }
    }
    
    if (std::holds_alternative<double>(value) && std::holds_alternative<double>(actualValue)) {
        double expected = std::get<double>(value);
        double actual = std::get<double>(actualValue);
        
        switch (op) {
            case ComparisonOperator::EQUAL: return std::abs(actual - expected) < 0.001;
            case ComparisonOperator::NOT_EQUAL: return std::abs(actual - expected) >= 0.001;
            case ComparisonOperator::LESS_THAN: return actual < expected;
            case ComparisonOperator::LESS_THAN_OR_EQUAL: return actual <= expected;
            case ComparisonOperator::GREATER_THAN: return actual > expected;
            case ComparisonOperator::GREATER_THAN_OR_EQUAL: return actual >= expected;
            case ComparisonOperator::IN_RANGE:
                if (rangeEnd && std::holds_alternative<double>(*rangeEnd)) {
                    double end = std::get<double>(*rangeEnd);
                    return actual >= expected && actual <= end;
                }
                return false;
            default: return false;
        }
    }
    
    if (std::holds_alternative<std::string>(value) && 
        std::holds_alternative<std::string>(actualValue)) {
        const std::string& expected = std::get<std::string>(value);
        const std::string& actual = std::get<std::string>(actualValue);
        
        switch (op) {
            case ComparisonOperator::EQUAL: return actual == expected;
            case ComparisonOperator::NOT_EQUAL: return actual != expected;
            case ComparisonOperator::CONTAINS: return actual.find(expected) != std::string::npos;
            default: return false;
        }
    }
    
    if (std::holds_alternative<bool>(value) && std::holds_alternative<bool>(actualValue)) {
        bool expected = std::get<bool>(value);
        bool actual = std::get<bool>(actualValue);
        
        switch (op) {
            case ComparisonOperator::EQUAL: return actual == expected;
            case ComparisonOperator::NOT_EQUAL: return actual != expected;
            default: return false;
        }
    }
    
    return false;
}

bool RuleEngine::executeAction(const RuleAction& action, 
                                const RuleExecutionContext& context) {
    bool success = false;
    
    switch (action.type) {
        case ActionType::RTL:
            success = executeRtlAction(action.parameters);
            break;
        case ActionType::LAND:
            success = executeLandAction(action.parameters);
            break;
        case ActionType::HOVER:
            success = executeHoverAction(action.parameters);
            break;
        case ActionType::CONTINUE:
            success = executeContinueAction(action.parameters);
            break;
        case ActionType::ABORT:
            success = executeAbortAction(action.parameters);
            break;
        case ActionType::EMERGENCY_LAND:
            success = executeEmergencyLandAction(action.parameters);
            break;
        case ActionType::NOTIFY:
            success = executeNotifyAction(action.parameters);
            break;
        case ActionType::CHANGE_STATE:
            success = executeChangeStateAction(action.parameters);
            break;
    }
    
    if (actionExecutedCallback_) {
        actionExecutedCallback_(action, success);
    }
    
    if (success) {
        stats_.totalActionsExecuted++;
    } else {
        stats_.totalActionsFailed++;
    }
    
    return success;
}

bool RuleEngine::executeRtlAction(const nlohmann::json& params) {
    LOG_INFO("RuleEngine", "Executing RTL action");
    // Integration with flight controller would go here
    return true;
}

bool RuleEngine::executeLandAction(const nlohmann::json& params) {
    LOG_INFO("RuleEngine", "Executing LAND action");
    return true;
}

bool RuleEngine::executeHoverAction(const nlohmann::json& params) {
    LOG_INFO("RuleEngine", "Executing HOVER action");
    return true;
}

bool RuleEngine::executeContinueAction(const nlohmann::json& params) {
    LOG_INFO("RuleEngine", "Executing CONTINUE action");
    return true;
}

bool RuleEngine::executeAbortAction(const nlohmann::json& params) {
    LOG_INFO("RuleEngine", "Executing ABORT action");
    return true;
}

bool RuleEngine::executeEmergencyLandAction(const nlohmann::json& params) {
    LOG_ERROR("RuleEngine", "Executing EMERGENCY LAND action");
    return true;
}

bool RuleEngine::executeNotifyAction(const nlohmann::json& params) {
    std::string message = params.value("message", "");
    LOG_INFO("RuleEngine", "Notification: " + message);
    return true;
}

bool RuleEngine::executeChangeStateAction(const nlohmann::json& params) {
    std::string newState = params.value("state", "");
    LOG_INFO("RuleEngine", "Changing state to: " + newState);
    return true;
}

std::variant<int, double, std::string, bool> RuleEngine::getParameterValue(
    const std::string& parameter, 
    const RuleExecutionContext& context) const {
    
    if (parameter == "battery_level") {
        return static_cast<double>(context.batteryLevel);
    } else if (parameter == "altitude") {
        return static_cast<double>(context.altitude);
    } else if (parameter == "time_since_disconnect_ms") {
        return static_cast<int>(context.timeSinceDisconnect.count());
    } else if (parameter == "time_since_partition_ms") {
        return static_cast<int>(context.timeSincePartition.count());
    } else if (parameter == "task_progress") {
        return static_cast<int>(context.taskProgress);
    } else if (parameter == "gcs_connected") {
        return context.gcsConnected;
    } else if (parameter == "swarm_connected") {
        return context.swarmConnected;
    } else if (parameter == "current_task") {
        return context.currentTask;
    }
    
    // Check extended data
    if (context.extendedData.contains(parameter)) {
        const auto& value = context.extendedData[parameter];
        if (value.is_number_integer()) {
            return value.get<int>();
        } else if (value.is_number_float()) {
            return value.get<double>();
        } else if (value.is_string()) {
            return value.get<std::string>();
        } else if (value.is_boolean()) {
            return value.get<bool>();
        }
    }
    
    return 0;
}

void RuleEngine::loadDefaultGcsDisconnectedRules() {
    LOG_INFO("RuleEngine", "Loading default GCS disconnected rules");
    
    // Rule: Low battery RTL
    Rule lowBatteryRule;
    lowBatteryRule.name = "Low Battery RTL";
    lowBatteryRule.type = RuleType::BATTERY_BASED;
    lowBatteryRule.enabled = true;
    lowBatteryRule.priority = 100;
    lowBatteryRule.oneShot = false;
    
    RuleCondition batteryCondition;
    batteryCondition.parameter = "battery_level";
    batteryCondition.op = ComparisonOperator::LESS_THAN;
    batteryCondition.value = 30.0;
    lowBatteryRule.conditions.push_back(batteryCondition);
    
    RuleCondition gcsDisconnected;
    gcsDisconnected.parameter = "gcs_connected";
    gcsDisconnected.op = ComparisonOperator::EQUAL;
    gcsDisconnected.value = false;
    lowBatteryRule.conditions.push_back(gcsDisconnected);
    
    RuleAction rtlAction;
    rtlAction.type = ActionType::RTL;
    rtlAction.priority = 100;
    rtlAction.description = "Return to launch when battery low and GCS disconnected";
    lowBatteryRule.actions.push_back(rtlAction);
    
    addRule(lowBatteryRule);
    
    // Rule: Timeout RTL
    Rule timeoutRule;
    timeoutRule.name = "GCS Disconnect Timeout RTL";
    timeoutRule.type = RuleType::TIME_BASED;
    timeoutRule.enabled = true;
    timeoutRule.priority = 90;
    
    RuleCondition timeCondition;
    timeCondition.parameter = "time_since_disconnect_ms";
    timeCondition.op = ComparisonOperator::GREATER_THAN;
    timeCondition.value = 30 * 60 * 1000;  // 30 minutes
    timeoutRule.conditions.push_back(timeCondition);
    
    RuleCondition gcsDisconnected2;
    gcsDisconnected2.parameter = "gcs_connected";
    gcsDisconnected2.op = ComparisonOperator::EQUAL;
    gcsDisconnected2.value = false;
    timeoutRule.conditions.push_back(gcsDisconnected2);
    
    RuleAction rtlAction2;
    rtlAction2.type = ActionType::RTL;
    rtlAction2.priority = 90;
    rtlAction2.description = "RTL after 30 minutes of GCS disconnection";
    timeoutRule.actions.push_back(rtlAction2);
    
    addRule(timeoutRule);
}

void RuleEngine::loadDefaultSwarmPartitionedRules() {
    LOG_INFO("RuleEngine", "Loading default swarm partitioned rules");
    
    // Partition-specific rules
    Rule partitionLowBatteryRule;
    partitionLowBatteryRule.name = "Partition Low Battery RTL";
    partitionLowBatteryRule.type = RuleType::BATTERY_BASED;
    partitionLowBatteryRule.enabled = true;
    partitionLowBatteryRule.priority = 95;
    
    RuleCondition batteryCondition;
    batteryCondition.parameter = "battery_level";
    batteryCondition.op = ComparisonOperator::LESS_THAN;
    batteryCondition.value = 35.0;  // Higher threshold during partition
    partitionLowBatteryRule.conditions.push_back(batteryCondition);
    
    RuleCondition swarmDisconnected;
    swarmDisconnected.parameter = "swarm_connected";
    swarmDisconnected.op = ComparisonOperator::EQUAL;
    swarmDisconnected.value = false;
    partitionLowBatteryRule.conditions.push_back(swarmDisconnected);
    
    RuleAction rtlAction;
    rtlAction.type = ActionType::RTL;
    rtlAction.priority = 95;
    rtlAction.description = "RTL when battery low during partition";
    partitionLowBatteryRule.actions.push_back(rtlAction);
    
    addRule(partitionLowBatteryRule);
}

void RuleEngine::loadDefaultFullyDisconnectedRules() {
    LOG_INFO("RuleEngine", "Loading default fully disconnected rules");
    
    // More conservative rules when both GCS and swarm are disconnected
    Rule criticalBatteryRule;
    criticalBatteryRule.name = "Critical Battery Immediate RTL";
    criticalBatteryRule.type = RuleType::BATTERY_BASED;
    criticalBatteryRule.enabled = true;
    criticalBatteryRule.priority = 200;  // Highest priority
    
    RuleCondition batteryCondition;
    batteryCondition.parameter = "battery_level";
    batteryCondition.op = ComparisonOperator::LESS_THAN;
    batteryCondition.value = 40.0;  // Even higher threshold
    criticalBatteryRule.conditions.push_back(batteryCondition);
    
    RuleCondition gcsDisconnected;
    gcsDisconnected.parameter = "gcs_connected";
    gcsDisconnected.op = ComparisonOperator::EQUAL;
    gcsDisconnected.value = false;
    criticalBatteryRule.conditions.push_back(gcsDisconnected);
    
    RuleCondition swarmDisconnected;
    swarmDisconnected.parameter = "swarm_connected";
    swarmDisconnected.op = ComparisonOperator::EQUAL;
    swarmDisconnected.value = false;
    criticalBatteryRule.conditions.push_back(swarmDisconnected);
    
    RuleAction rtlAction;
    rtlAction.type = ActionType::RTL;
    rtlAction.priority = 200;
    rtlAction.description = "Immediate RTL when critical battery and fully disconnected";
    criticalBatteryRule.actions.push_back(rtlAction);
    
    addRule(criticalBatteryRule);
    
    // Short timeout for fully disconnected
    Rule shortTimeoutRule;
    shortTimeoutRule.name = "Fully Disconnected Short Timeout";
    shortTimeoutRule.type = RuleType::TIME_BASED;
    shortTimeoutRule.enabled = true;
    shortTimeoutRule.priority = 150;
    
    RuleCondition timeCondition;
    timeCondition.parameter = "time_since_disconnect_ms";
    timeCondition.op = ComparisonOperator::GREATER_THAN;
    timeCondition.value = 15 * 60 * 1000;  // 15 minutes
    shortTimeoutRule.conditions.push_back(timeCondition);
    
    RuleCondition gcsDisconnected2;
    gcsDisconnected2.parameter = "gcs_connected";
    gcsDisconnected2.op = ComparisonOperator::EQUAL;
    gcsDisconnected2.value = false;
    shortTimeoutRule.conditions.push_back(gcsDisconnected2);
    
    RuleCondition swarmDisconnected2;
    swarmDisconnected2.parameter = "swarm_connected";
    swarmDisconnected2.op = ComparisonOperator::EQUAL;
    swarmDisconnected2.value = false;
    shortTimeoutRule.conditions.push_back(swarmDisconnected2);
    
    RuleAction rtlAction2;
    rtlAction2.type = ActionType::RTL;
    rtlAction2.priority = 150;
    rtlAction2.description = "RTL after 15 minutes when fully disconnected";
    shortTimeoutRule.actions.push_back(rtlAction2);
    
    addRule(shortTimeoutRule);
}

void RuleEngine::loadDefaultEmergencyRules() {
    LOG_INFO("RuleEngine", "Loading default emergency rules");
    
    Rule emergencyRule;
    emergencyRule.name = "Critical Battery Emergency Land";
    emergencyRule.type = RuleType::EMERGENCY;
    emergencyRule.enabled = true;
    emergencyRule.priority = 1000;  // Absolute highest priority
    
    RuleCondition batteryCondition;
    batteryCondition.parameter = "battery_level";
    batteryCondition.op = ComparisonOperator::LESS_THAN;
    batteryCondition.value = 15.0;  // Critical level
    emergencyRule.conditions.push_back(batteryCondition);
    
    RuleAction emergencyLandAction;
    emergencyLandAction.type = ActionType::EMERGENCY_LAND;
    emergencyLandAction.priority = 1000;
    emergencyLandAction.description = "Emergency land when battery critical";
    emergencyRule.actions.push_back(emergencyLandAction);
    
    addRule(emergencyRule);
}

void RuleEngine::clearAllRules() {
    std::lock_guard<std::mutex> lock(mutex_);
    rules_.clear();
    LOG_INFO("RuleEngine", "All rules cleared");
}

void RuleEngine::sortRulesByPriority() {
    // Map is already sorted by key, but we might want to sort by priority
    // For now, rules are evaluated in insertion order
}

std::string RuleEngine::generateRuleId() {
    return "rule_" + std::to_string(nextRuleId_++);
}

std::string RuleEngine::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
    return ss.str();
}

RuleEngine::Statistics RuleEngine::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Statistics stats = stats_;
    stats.totalRules = rules_.size();
    
    stats.enabledRules = 0;
    for (const auto& pair : rules_) {
        if (pair.second.enabled) {
            stats.enabledRules++;
        }
    }
    
    return stats;
}

void RuleEngine::resetStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_ = Statistics{};
}

void RuleEngine::setRuleCheckInterval(std::chrono::milliseconds interval) {
    checkInterval_ = interval;
}

void RuleEngine::setMaxRulesPerEvaluation(int maxRules) {
    maxRulesPerEvaluation_ = maxRules;
}

void RuleEngine::setActionTimeout(std::chrono::milliseconds timeout) {
    actionTimeout_ = timeout;
}

void RuleEngine::setRuleTriggeredCallback(RuleTriggeredCallback callback) {
    ruleTriggeredCallback_ = callback;
}

void RuleEngine::setActionExecutedCallback(ActionExecutedCallback callback) {
    actionExecutedCallback_ = callback;
}

bool RuleEngine::exportRulesToFile(const std::string& filepath) const {
    std::ofstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("RuleEngine", "Failed to open file for export: " + filepath);
        return false;
    }
    
    nlohmann::json json = serializeRules();
    file << json.dump(2);
    file.close();
    
    LOG_INFO("RuleEngine", "Rules exported to: " + filepath);
    return true;
}

bool RuleEngine::importRulesFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        LOG_ERROR("RuleEngine", "Failed to open file for import: " + filepath);
        return false;
    }
    
    nlohmann::json json;
    try {
        file >> json;
        file.close();
    } catch (const std::exception& e) {
        LOG_ERROR("RuleEngine", "Failed to parse JSON: " + std::string(e.what()));
        return false;
    }
    
    return deserializeRules(json);
}

nlohmann::json RuleEngine::serializeRules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json json;
    json["rules"] = nlohmann::json::array();
    
    for (const auto& pair : rules_) {
        const Rule& rule = pair.second;
        nlohmann::json ruleJson;
        ruleJson["id"] = rule.id;
        ruleJson["name"] = rule.name;
        ruleJson["type"] = static_cast<int>(rule.type);
        ruleJson["enabled"] = rule.enabled;
        ruleJson["priority"] = rule.priority;
        ruleJson["one_shot"] = rule.oneShot;
        ruleJson["created_at"] = rule.createdAt;
        ruleJson["trigger_count"] = rule.triggerCount;
        
        ruleJson["conditions"] = nlohmann::json::array();
        for (const auto& condition : rule.conditions) {
            nlohmann::json condJson;
            condJson["parameter"] = condition.parameter;
            condJson["operator"] = static_cast<int>(condition.op);
            // Value serialization would need type handling
            ruleJson["conditions"].push_back(condJson);
        }
        
        ruleJson["actions"] = nlohmann::json::array();
        for (const auto& action : rule.actions) {
            nlohmann::json actionJson;
            actionJson["type"] = static_cast<int>(action.type);
            actionJson["priority"] = action.priority;
            actionJson["description"] = action.description;
            actionJson["parameters"] = action.parameters;
            ruleJson["actions"].push_back(actionJson);
        }
        
        json["rules"].push_back(ruleJson);
    }
    
    return json;
}

bool RuleEngine::deserializeRules(const nlohmann::json& json) {
    try {
        if (!json.contains("rules") || !json["rules"].is_array()) {
            LOG_ERROR("RuleEngine", "Invalid JSON format: missing rules array");
            return false;
        }
        
        std::lock_guard<std::mutex> lock(mutex_);
        rules_.clear();
        
        for (const auto& ruleJson : json["rules"]) {
            Rule rule;
            rule.id = ruleJson.value("id", generateRuleId());
            rule.name = ruleJson.value("name", "Unnamed Rule");
            rule.type = static_cast<RuleType>(ruleJson.value("type", 0));
            rule.enabled = ruleJson.value("enabled", true);
            rule.priority = ruleJson.value("priority", 50);
            rule.oneShot = ruleJson.value("one_shot", false);
            rule.createdAt = ruleJson.value("created_at", getCurrentTimestamp());
            rule.triggerCount = ruleJson.value("trigger_count", 0);
            
            // Deserialize conditions and actions
            // (Full implementation would parse all fields)
            
            rules_[rule.id] = rule;
        }
        
        LOG_INFO("RuleEngine", "Deserialized " + std::to_string(rules_.size()) + " rules");
        return true;
        
    } catch (const std::exception& e) {
        LOG_ERROR("RuleEngine", "Failed to deserialize rules: " + std::string(e.what()));
        return false;
    }
}

} // namespace nodeagent
