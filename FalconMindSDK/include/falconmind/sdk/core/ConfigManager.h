/**
 * @file ConfigManager.h
 * @brief 配置管理系统 - 支持YAML/JSON配置文件
 * 
 * 提供统一的配置加载、验证和热更新功能
 * 
 * @example
 * @code
 * ConfigManager config;
 * config.loadFromFile("mission.yaml");
 * 
 * auto altitude = config.get<double>("flight.altitude", 50.0);
 * auto camera = config.get<std::string>("sensors.camera.type", "rgb");
 * 
 * // 监听配置变更
 * config.onChange("flight.altitude", [](double newVal) {
 *     std::cout << "Altitude changed to: " << newVal << std::endl;
 * });
 * @endcode
 */

#pragma once

#include "falconmind/sdk/core/ErrorCode.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <functional>
#include <mutex>
#include <atomic>
#include <thread>
#include <filesystem>
#include <filesystem>

namespace falconmind {
namespace sdk {
namespace core {

using json = nlohmann::json;

/**
 * @brief 配置变更回调类型
 */
template<typename T>
using ConfigChangeCallback = std::function<void(const T& oldVal, const T& newVal)>;

/**
 * @brief 配置管理器
 * 
 * 统一的配置管理系统，支持：
 * - YAML/JSON 配置文件加载
 * - 环境变量覆盖
 * - 配置验证和默认值
 * - 运行时热更新
 * - 变更通知
 */
class ConfigManager {
public:
    ConfigManager();
    ~ConfigManager();

    /**
     * @brief 从文件加载配置
     * @param filepath 配置文件路径 (.yaml, .yml, .json)
     * @return 是否加载成功
     */
    Result<void> loadFromFile(const std::filesystem::path& filepath);

    /**
     * @brief 从字符串加载配置
     * @param content 配置内容 (JSON/YAML格式)
     * @param format 格式类型 ("json" 或 "yaml")
     */
    Result<void> loadFromString(const std::string& content, const std::string& format = "json");

    /**
     * @brief 从环境变量加载（覆盖现有配置）
     * @param prefix 环境变量前缀，如 "FALCONMIND_"
     */
    void loadFromEnvironment(const std::string& prefix = "FALCONMIND_");

    /**
     * @brief 保存配置到文件
     */
    Result<void> saveToFile(const std::filesystem::path& filepath) const;

    /**
     * @brief 获取配置值（支持点号路径）
     * @param path 配置路径，如 "mission.altitude"
     * @param defaultVal 默认值（如果配置不存在）
     * @return 配置值或默认值
     */
    template<typename T>
    T get(const std::string& path, const T& defaultVal = T{}) const {
        std::lock_guard<std::mutex> lock(mutex_);
        try {
            return getNestedValue<T>(config_, path);
        } catch (...) {
            return defaultVal;
        }
    }

    /**
     * @brief 设置配置值
     * @param path 配置路径
     * @param value 新值
     * @param notify 是否触发变更通知
     */
    template<typename T>
    void set(const std::string& path, const T& value, bool notify = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        T oldVal = get<T>(path, T{});
        setNestedValue(config_, path, value);
        
        if (notify && oldVal != value) {
            notifyChange(path, oldVal, value);
        }
    }

    /**
     * @brief 检查配置是否存在
     */
    bool has(const std::string& path) const;

    /**
     * @brief 注册配置变更监听器
     */
    template<typename T>
    void onChange(const std::string& path, ConfigChangeCallback<T> callback) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        callbacks_[path].push_back([callback](const json& oldVal, const json& newVal) {
            callback(oldVal.get<T>(), newVal.get<T>());
        });
    }

    /**
     * @brief 启用配置文件热更新
     * @param intervalMs 检查间隔（毫秒）
     */
    void enableHotReload(int intervalMs = 5000);
    void disableHotReload();

    /**
     * @brief 验证配置结构
     * @param schema JSON Schema
     * @return 验证结果
     */
    Result<void> validate(const json& schema) const;

    /**
     * @brief 获取完整配置JSON
     */
    json toJson() const;

    /**
     * @brief 从其他配置合并（深度合并）
     */
    void merge(const ConfigManager& other);

    /**
     * @brief 清空所有配置
     */
    void clear();

    /**
     * @brief 打印配置内容（用于调试）
     */
    std::string dump(int indent = 2) const;

private:
    mutable std::mutex mutex_;
    mutable std::mutex callbackMutex_;
    json config_;
    std::filesystem::path sourceFile_;
    std::unordered_map<std::string, std::vector<std::function<void(const json&, const json&)>>> callbacks_;
    
    // 热更新相关
    std::atomic<bool> hotReloadEnabled_{false};
    std::thread hotReloadThread_;
    std::filesystem::file_time_type lastWriteTime_;

    template<typename T>
    T getNestedValue(const json& j, const std::string& path) const {
        size_t pos = path.find('.');
        if (pos == std::string::npos) {
            return j.at(path).get<T>();
        }
        std::string key = path.substr(0, pos);
        std::string rest = path.substr(pos + 1);
        return getNestedValue<T>(j.at(key), rest);
    }

    template<typename T>
    void setNestedValue(json& j, const std::string& path, const T& value) {
        size_t pos = path.find('.');
        if (pos == std::string::npos) {
            j[path] = value;
            return;
        }
        std::string key = path.substr(0, pos);
        std::string rest = path.substr(pos + 1);
        if (!j.contains(key) || !j[key].is_object()) {
            j[key] = json::object();
        }
        setNestedValue(j[key], rest, value);
    }

    void notifyChange(const std::string& path, const json& oldVal, const json& newVal);
    void hotReloadLoop(int intervalMs);
};

/**
 * @brief 全局配置实例
 */
ConfigManager& globalConfig();

} // namespace core
} // namespace sdk
} // namespace falconmind
