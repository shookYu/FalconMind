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

    // ==================== 任务上传功能 ====================
    
    /**
     * @brief 上传航点任务到飞控
     * @param waypoints 航点列表 {lat, lon, alt} (度, 度, 米)
     * @return true成功，false失败
     */
    bool uploadMission(const std::vector<std::tuple<double, double, double>>& waypoints);
    
    /**
     * @brief 清除当前任务
     * @return true成功，false失败
     */
    bool clearMission();
    
    /**
     * @brief 等待并接收单个MAVLink消息
     * @param timeoutMs 超时时间(毫秒)
     * @return 成功解析的消息ID，失败返回-1
     */
    int receiveMessage(int timeoutMs = 1000);
    
    /**
     * @brief 获取最近一次接收的消息ID
     */
    uint8_t getLastReceivedMsgId() const { return lastReceivedMsgId_; }

private:
    // 生成 MAVLink v1 COMMAND_LONG 帧（二进制），后续可替换为完整 MAVLink 库实现
    bool encodeMavlinkCommand(const FlightCommand& cmd, std::string& out);

    // 生成 MAVLink MISSION_ITEM_INT 帧
    bool encodeMissionItemInt(uint16_t seq, uint16_t total, double lat, double lon, float alt, std::string& out);

    // 生成 MAVLink MISSION_COUNT 帧
    bool encodeMissionCount(uint16_t count, std::string& out);

    // 生成 MAVLink MISSION_CLEAR_ALL 帧
    bool encodeMissionClearAll(std::string& out);

    // CRC-16/MCRF4XX 算法（MAVLink 使用）
    std::uint16_t mavlinkCrcAccumulate(std::uint8_t data, std::uint16_t crc) const;
    std::uint16_t mavlinkCrcCalculate(const std::uint8_t* buf, std::size_t len, std::uint8_t crcExtra) const;

    int sock_{-1};
    FlightConnectionConfig cfg_{};
    std::atomic<bool> connected_{false};
    mutable std::mutex stateMutex_;  // mutable 允许在 const 方法中锁定
    FlightState lastState_{}; // 预留缓存，当前未真实填充
    std::uint8_t seq_{0};     // MAVLink 序号
    uint8_t lastReceivedMsgId_{0};  // 最近接收的消息ID
    mutable std::mutex receiveMutex_;  // 接收缓冲区互斥锁
    std::vector<uint8_t> receiveBuffer_;  // 接收缓冲区

};

} // namespace falconmind::sdk::flight

