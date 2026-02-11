// NodeAgent - Main agent class for SDK ↔ Cluster Center communication
#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>

// 前向声明
namespace falconmind::sdk::flight {
    class FlightConnectionService;
}

namespace nodeagent {

class IUplinkClient;
class IDownlinkClient;
class CommandHandler;
class MissionHandler;
class FlowHandler;
class MessageAckManager;
class ReconnectManager;

// 下行消息处理器类型
struct DownlinkMessage;
using DownlinkMessageHandler = std::function<void(const DownlinkMessage&)>;

// NodeAgent 主类：订阅 SDK Telemetry 并上报到 Cluster Center，接收下行命令/任务
class NodeAgent {
public:
    enum class Protocol {
        TCP,   // TCP Socket + JSON
        MQTT   // MQTT
    };

    struct Config {
        std::string uavId{"uav0"};
        Protocol protocol{Protocol::TCP};  // 通信协议
        
        // TCP 配置
        std::string centerAddress{"127.0.0.1"};
        int centerPort{8888};
        
        // MQTT 配置
        std::string mqttBrokerAddress{"127.0.0.1"};
        int mqttBrokerPort{1883};
        std::string mqttClientId{"nodeagent"};
        std::string mqttTopicPrefix{"uav"};  // 主题前缀：uav/{uavId}/telemetry
        
        int telemetryIntervalMs{1000};  // Telemetry 上报间隔（毫秒）
        
        // 错误处理和重连配置
        bool enableAutoReconnect{true};  // 启用自动重连
        int maxReconnectRetries{5};  // 最大重连次数
        int reconnectInitialDelayMs{1000};  // 初始重连延迟（毫秒）
        
        // 日志配置（使用完整命名空间，避免循环依赖）
        int logLevel{1};  // 日志级别：0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=FATAL
    };

    NodeAgent(const Config& config);
    ~NodeAgent();

    // 启动 NodeAgent（开始订阅 SDK Telemetry 并上报，同时接收下行消息）
    bool start();

    // 停止 NodeAgent
    void stop();

    // 检查是否正在运行
    bool isRunning() const { return running_; }

    // 设置下行消息处理器（当收到 Cluster Center 的命令/任务时调用）
    void setDownlinkMessageHandler(DownlinkMessageHandler handler);

    // 设置 FlightConnectionService（用于执行命令和任务）
    void setFlightConnectionService(std::shared_ptr<falconmind::sdk::flight::FlightConnectionService> service);

private:
    Config config_;
    std::unique_ptr<IUplinkClient> uplinkClient_;
    std::unique_ptr<IDownlinkClient> downlinkClient_;
    std::unique_ptr<CommandHandler> commandHandler_;
    std::unique_ptr<MissionHandler> missionHandler_;
    std::unique_ptr<FlowHandler> flowHandler_;
    std::unique_ptr<MessageAckManager> ackManager_;
    std::unique_ptr<ReconnectManager> reconnectManager_;
    std::atomic<bool> running_{false};
    std::thread workerThread_;

    void workerLoop();
    void handleDownlinkMessage(const DownlinkMessage& msg);
    
    // 上报Flow状态到Cluster Center
    void reportFlowStatus(const std::string& flow_id, const std::string& status, const std::string& error_msg = "");
};

} // namespace nodeagent
