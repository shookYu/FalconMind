// FalconMindSDK - FlightConnectionService (week2 skeleton, UDP-only)
#pragma once

#include "falconmind/sdk/flight/FlightTypes.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <cstdint>

namespace falconmind::sdk::flight {

class FlightConnectionService {
public:
    FlightConnectionService();
    ~FlightConnectionService();

    bool connect(const FlightConnectionConfig& cfg);
    void disconnect();

    bool isConnected() const noexcept { return connected_; }

    // 发送高层飞控命令（内部先简单打印/占位，后续接入 MAVLink）
    bool sendCommand(const FlightCommand& cmd);

    // 从链路中轮询最新状态（当前为占位，后续接入 MAVLink 解析）
    std::optional<FlightState> pollState();
    
    // 获取最后缓存的飞行状态
    FlightState getLastState() const;

private:
    // 生成 MAVLink v1 COMMAND_LONG 帧（二进制），后续可替换为完整 MAVLink 库实现
    bool encodeMavlinkCommand(const FlightCommand& cmd, std::string& out);

    // CRC-16/MCRF4XX 算法（MAVLink 使用）
    std::uint16_t mavlinkCrcAccumulate(std::uint8_t data, std::uint16_t crc) const;
    std::uint16_t mavlinkCrcCalculate(const std::uint8_t* buf, std::size_t len, std::uint8_t crcExtra) const;

    int sock_{-1};
    FlightConnectionConfig cfg_{};
    std::atomic<bool> connected_{false};
    mutable std::mutex stateMutex_;  // mutable 允许在 const 方法中锁定
    FlightState lastState_{}; // 预留缓存，当前未真实填充
    std::uint8_t seq_{0};     // MAVLink 序号
};

} // namespace falconmind::sdk::flight

