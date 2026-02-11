// NodeAgent - Multi-UAV management support
#pragma once

#include "nodeagent/NodeAgent.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <string>
#include <map>
#include <memory>
#include <mutex>

namespace nodeagent {

// 多 UAV 管理器：管理多个 UAV 的 NodeAgent 实例
class MultiUavManager {
public:
    struct UavConfig {
        std::string uavId;
        std::string centerAddress{"127.0.0.1"};
        int centerPort{8888};
        std::shared_ptr<falconmind::sdk::flight::FlightConnectionService> flightService;
    };

    MultiUavManager();
    ~MultiUavManager();

    // 添加 UAV
    bool addUav(const UavConfig& config);

    // 移除 UAV
    bool removeUav(const std::string& uavId);

    // 启动所有 UAV
    bool startAll();

    // 停止所有 UAV
    void stopAll();

    // 启动指定 UAV
    bool startUav(const std::string& uavId);

    // 停止指定 UAV
    void stopUav(const std::string& uavId);

    // 获取 UAV 列表
    std::vector<std::string> getUavList() const;

    // 检查 UAV 是否运行
    bool isUavRunning(const std::string& uavId) const;

private:
    struct UavEntry {
        UavConfig config;
        std::unique_ptr<NodeAgent> agent;
        bool running{false};
    };

    std::map<std::string, UavEntry> uavs_;
    mutable std::mutex mutex_;
};

} // namespace nodeagent
