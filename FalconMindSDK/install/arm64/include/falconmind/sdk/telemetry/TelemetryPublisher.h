// FalconMindSDK - TelemetryPublisher for SDK internal telemetry distribution
#pragma once

#include "falconmind/sdk/telemetry/TelemetryTypes.h"

#include <functional>
#include <vector>
#include <mutex>

namespace falconmind::sdk::telemetry {

// SDK 内部 Telemetry 发布器
// 用于节点（如 FlightStateSourceNode）发布遥测数据，供 NodeAgent 订阅
class TelemetryPublisher {
public:
    using Handler = std::function<void(const TelemetryMessage&)>;

    // 订阅 Telemetry 消息
    // 返回订阅 ID，可用于后续取消订阅
    int subscribe(const Handler& handler);

    // 取消订阅
    void unsubscribe(int id);

    // 发布一条 Telemetry 消息（通知所有订阅者）
    void publish(const TelemetryMessage& msg);

    // 获取全局单例（可选，也可以由上层显式创建实例）
    static TelemetryPublisher& instance();

private:
    TelemetryPublisher() = default;
    ~TelemetryPublisher() = default;
    TelemetryPublisher(const TelemetryPublisher&) = delete;
    TelemetryPublisher& operator=(const TelemetryPublisher&) = delete;

    int nextId_{1};
    std::vector<std::pair<int, Handler>> handlers_;
    std::mutex mutex_;
};

} // namespace falconmind::sdk::telemetry
