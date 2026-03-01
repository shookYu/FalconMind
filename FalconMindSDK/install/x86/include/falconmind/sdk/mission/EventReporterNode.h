// FalconMindSDK - Event Reporter Node
// 接收搜索事件和检测结果，通过MQTT遥测上报
#pragma once

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/mission/SearchTypes.h"

#include <memory>
#include <string>
#include <fstream>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

namespace falconmind::sdk::mission {

// 前向声明MQTT客户端
class MqttClient;

/**
 * @brief 事件上报节点
 * 
 * 功能：
 * - 接收搜索事件和进度
 * - MQTT遥测上报
 * - 本地日志持久化
 * - 批量上报优化
 * 
 * 配置参数：
 * - uav_id: UAV标识符
 * - mission_id: 任务标识符
 * - mqtt_host: MQTT broker地址
 * - mqtt_port: MQTT broker端口
 * - log_file: 日志文件路径
 * - batch_mode: 是否启用批量模式
 * - batch_size: 批量大小
 */
class EventReporterNode : public core::Node {
public:
    EventReporterNode();
    ~EventReporterNode() override;

    // 上报搜索事件
    void reportSearchEvent(const SearchEvent& event);
    
    // 上报搜索进度
    void reportSearchProgress(const SearchProgress& progress);
    
    // 上报检测结果（从 DetectionResult 转换）
    void reportDetection(const std::string& targetClass, double confidence,
                        double lat, double lon, double alt);

    // Node 接口实现
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;
    
    // 统计查询
    uint64_t getEventCount() const { return eventCount_.load(); }
    uint64_t getProgressCount() const { return progressCount_.load(); }
    bool isMqttConnected() const;

private:
    // 序列化
    std::string serializeEvent(const SearchEvent& event);
    std::string serializeProgress(const SearchProgress& progress);
    std::string escapeJson(const std::string& s);
    
    // 数据持久化和上报
    void writeToLog(const std::string& json);
    void publishEvent(const std::string& topic, const std::string& message);
    void flushBatch();
    void flushThreadFunc();

private:
    // 配置
    std::string uavId_;
    std::string missionId_;
    std::string mqttHost_;
    int mqttPort_;
    std::string logFilePath_;
    bool batchMode_;
    int maxBatchSize_;
    int flushIntervalMs_;
    
    // MQTT客户端
    std::unique_ptr<MqttClient> mqttClient_;
    
    // 日志文件
    std::ofstream logFile_;
    
    // 批量发送
    std::vector<std::string> eventBatch_;
    std::mutex batchMutex_;
    std::thread flushThread_;
    std::atomic<bool> stopThread_{false};
    
    // 统计
    std::atomic<uint64_t> eventCount_{0};
    std::atomic<uint64_t> progressCount_{0};
};

} // namespace falconmind::sdk::mission
