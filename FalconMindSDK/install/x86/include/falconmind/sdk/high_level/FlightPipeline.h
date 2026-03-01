/**
 * @file FlightPipeline.h
 * @brief 飞控连接与管理流水线
 * 
 * 简化无人机飞控连接、状态监控和基本控制命令。
 * 封装 MAVLink 协议细节，提供易用的 C++ API。
 * 
 * @example
 * @code
 * // 连接飞控
 * auto flight = FlightPipeline::create()
 *     .withConnection("/dev/ttyUSB0", 57600)
 *     .withHeartbeatInterval(1.0)
 *     .build();
 * 
 * if (!flight) {
 *     std::cerr << "连接失败: " << flight.errorMessage() << std::endl;
 *     return 1;
 * }
 * 
 * auto f = flight.value();
 * 
 * // 等待连接就绪
 * f->waitForConnection(std::chrono::seconds(10));
 * 
 * // 获取当前状态
 * auto status = f->getVehicleStatus();
 * std::cout << "电池: " << status.batteryPercent << "%" << std::endl;
 * 
 * // 解锁并起飞
 * f->arm();
 * f->takeoff(50.0f);
 * @endcode
 */

#pragma once

#include "Result.h"
#include "ErrorCode.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include <functional>
#include <memory>
#include <string>
#include <chrono>
#include <optional>

namespace falconmind {
namespace sdk {
namespace high_level {

// Import GeoPoint from mission namespace
using falconmind::sdk::mission::GeoPoint;

/**
 * @brief 飞行器状态
 */
struct VehicleStatus {
    bool isConnected = false;           ///< 是否已连接
    bool isArmed = false;               ///< 是否已解锁
    bool isFlying = false;              ///< 是否在飞行中
    bool isOffboard = false;            ///< 是否在外控模式
    
    float batteryPercent = 0.0f;        ///< 电池百分比
    float batteryVoltage = 0.0f;        ///< 电池电压
    
    double latitude = 0.0;              ///< 当前纬度
    double longitude = 0.0;             ///< 当前经度
    float relativeAltitude = 0.0f;      ///< 相对高度（米）
    float absoluteAltitude = 0.0f;      ///< 绝对高度（米）
    
    float groundSpeed = 0.0f;           ///< 地速（m/s）
    float airSpeed = 0.0f;              ///< 空速（m/s）
    float heading = 0.0f;               ///< 航向（度，0-360）
    
    float roll = 0.0f;                  ///< 横滚角（度）
    float pitch = 0.0f;                 ///< 俯仰角（度）
    float yaw = 0.0f;                   ///< 偏航角（度）
    
    int gpsSatellites = 0;              ///< GPS 卫星数
    float gpsHdop = 0.0f;               ///< GPS 水平精度因子
    
    std::string flightMode;             ///< 当前飞行模式
    std::string systemStatus;           ///< 系统状态
};

/**
 * @brief 飞行模式
 */
enum class FlightMode {
    STABILIZE,      ///< 自稳模式
    ALT_HOLD,       ///< 定高模式
    LOITER,         ///< 悬停模式
    RTL,            ///< 返航模式
    LAND,           ///< 降落模式
    AUTO,           ///< 自动模式
    GUIDED,         ///< 引导模式（外控）
    OFFBOARD        ///< 外控模式（PX4）
};

/**
 * @brief 遥测数据
 */
struct TelemetryData {
    std::chrono::steady_clock::time_point timestamp;
    VehicleStatus status;
    
    // 传感器数据
    struct {
        float x, y, z;
    } acceleration;     ///< 加速度（m/s²）
    
    struct {
        float x, y, z;
    } gyroscope;        ///< 陀螺仪（rad/s）
    
    struct {
        float x, y, z;
    } magneticField;    ///< 磁场（Gauss）
};

// 前置声明
class FlightPipeline;

/**
 * @brief 飞控连接构建器
 */
class FlightPipelineBuilder {
public:
    FlightPipelineBuilder() = default;
    
    /**
     * @brief 配置连接
     * @param connectionString 连接字符串
     *   - 串口: "/dev/ttyUSB0", "/dev/ttyACM0"
     *   - UDP: "udp://127.0.0.1:14550"
     *   - TCP: "tcp://127.0.0.1:5760"
     * @param baudRate 波特率（仅串口）
     */
    FlightPipelineBuilder& withConnection(
        const std::string& connectionString, 
        int baudRate = 57600);
    
    /**
     * @brief 配置系统 ID
     * @param systemId 本机系统 ID（默认 1）
     * @param componentId 组件 ID（默认 1）
     */
    FlightPipelineBuilder& withSystemId(uint8_t systemId, uint8_t componentId = 1);
    
    /**
     * @brief 配置心跳间隔
     * @param intervalSeconds 心跳发送间隔（秒）
     */
    FlightPipelineBuilder& withHeartbeatInterval(double intervalSeconds);
    
    /**
     * @brief 配置自动重连
     * @param enable 是否启用
     * @param maxRetries 最大重试次数（-1 表示无限）
     * @param retryInterval 重试间隔（秒）
     */
    FlightPipelineBuilder& withAutoReconnect(
        bool enable = true, 
        int maxRetries = -1,
        double retryInterval = 5.0);
    
    /**
     * @brief 配置超时时间
     * @param commandTimeout 命令超时（秒）
     * @param telemetryTimeout 遥测超时（秒）
     */
    FlightPipelineBuilder& withTimeouts(
        double commandTimeout = 30.0,
        double telemetryTimeout = 5.0);
    
    /**
     * @brief 配置遥测订阅
     * @param rateHz 遥测频率（Hz）
     */
    FlightPipelineBuilder& withTelemetryRate(double rateHz);
    
    /**
     * @brief 构建飞控连接
     */
    ResultPtr<FlightPipeline> build();
    
private:
    struct Config {
        std::string connectionString;
        int baudRate = 57600;
        bool useUdp = false;
        uint8_t systemId = 1;
        uint8_t componentId = 1;
        double heartbeatInterval = 1.0;
        bool autoReconnect = true;
        int maxRetries = -1;
        double retryInterval = 5.0;
        double commandTimeout = 30.0;
        double telemetryTimeout = 5.0;
        double telemetryRate = 10.0;
    };
    
    Config config_;
    friend class FlightPipeline;
};

/**
 * @brief 飞控连接与管理流水线
 */
class FlightPipeline {
public:
    ~FlightPipeline();
    
    /**
     * @brief 创建构建器
     */
    static FlightPipelineBuilder create();
    
    // ==================== 连接管理 ====================
    
    /**
     * @brief 连接飞控
     */
    Result<void> connect();
    
    /**
     * @brief 断开连接
     */
    Result<void> disconnect();
    
    /**
     * @brief 是否已连接
     */
    bool isConnected() const;
    
    /**
     * @brief 等待连接建立
     * @param timeout 超时时间
     * @return 是否成功连接
     */
    bool waitForConnection(std::chrono::seconds timeout);
    
    // ==================== 基本控制 ====================
    
    /**
     * @brief 解锁电机
     */
    Result<void> arm();
    
    /**
     * @brief 锁定电机
     */
    Result<void> disarm();
    
    /**
     * @brief 起飞到指定高度
     * @param altitude 目标高度（米，相对地面）
     */
    Result<void> takeoff(float altitude);
    
    /**
     * @brief 降落
     * @param waitUntilLanded 是否等待降落完成
     */
    Result<void> land(bool waitUntilLanded = true);
    
    /**
     * @brief 返航（RTL）
     */
    Result<void> returnToLaunch();
    
    /**
     * @brief 急停（Kill Switch）
     */
    Result<void> kill();
    
    // ==================== 模式切换 ====================
    
    /**
     * @brief 设置飞行模式
     */
    Result<void> setFlightMode(FlightMode mode);
    
    /**
     * @brief 设置外控模式（OFFBOARD/GUIDED）
     */
    Result<void> setOffboardMode(bool enabled);
    
    // ==================== 位置控制 ====================
    
    /**
     * @brief 飞向指定位置
     * @param lat 纬度
     * @param lon 经度
     * @param alt 高度（米，相对地面）
     * @param speed 速度（m/s，0 表示默认速度）
     */
    Result<void> goToPosition(double lat, double lon, float alt, float speed = 0.0f);
    
    /**
     * @brief 相对当前位置移动
     * @param north 北向距离（米）
     * @param east 东向距离（米）
     * @param down 向下距离（米，正数向下）
     * @param speed 速度（m/s）
     */
    Result<void> moveRelative(float north, float east, float down, float speed = 0.0f);
    
    /**
     * @brief 设置速度
     * @param vx X 方向速度（m/s，前向）
     * @param vy Y 方向速度（m/s，右向）
     * @param vz Z 方向速度（m/s，向下）
     * @param yawRate 偏航角速度（deg/s）
     */
    Result<void> setVelocity(float vx, float vy, float vz, float yawRate = 0.0f);
    
    /**
     * @brief 设置机体速度
     * @param forward 前向速度（m/s）
     * @param right 右向速度（m/s）
     * @param down 下向速度（m/s）
     * @param yawRate 偏航角速度（deg/s）
     */
    Result<void> setBodyVelocity(float forward, float right, float down, float yawRate = 0.0f);
    
    /**
     * @brief 设置目标航向
     * @param heading 航向角（度，0-360，0=北）
     */
    Result<void> setHeading(float heading);
    
    // ==================== 状态查询 ====================
    
    /**
     * @brief 获取飞行器状态
     */
    VehicleStatus getVehicleStatus() const;
    
    /**
     * @brief 获取当前位置
     */
    std::optional<GeoPoint> getCurrentPosition() const;
    
    /**
     * @brief 获取当前高度
     */
    float getCurrentAltitude() const;
    
    /**
     * @brief 获取当前速度
     */
    float getCurrentSpeed() const;
    
    /**
     * @brief 获取电池状态
     */
    struct BatteryStatus {
        float percent;
        float voltage;
        float current;
        float remainingCapacity;
    };
    BatteryStatus getBatteryStatus() const;
    
    // ==================== 参数操作 ====================
    
    /**
     * @brief 获取参数
     * @param name 参数名
     * @return 参数值
     */
    Result<float> getParameter(const std::string& name);
    
    /**
     * @brief 设置参数
     * @param name 参数名
     * @param value 参数值
     */
    Result<void> setParameter(const std::string& name, float value);
    
    // ==================== 回调设置 ====================
    
    /**
     * @brief 设置连接状态回调
     */
    void onConnectionChanged(std::function<void(bool connected)> callback);
    
    /**
     * @brief 设置状态更新回调
     */
    void onStatusUpdated(std::function<void(const VehicleStatus&)> callback);
    
    /**
     * @brief 设置遥测数据回调
     */
    void onTelemetry(std::function<void(const TelemetryData&)> callback);
    
    /**
     * @brief 设置命令完成回调
     */
    void onCommandCompleted(
        std::function<void(const std::string& command, bool success)> callback);
    
    /**
     * @brief 设置错误回调
     */
    void onError(std::function<void(const std::string& error)> callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    FlightPipeline() = default;
    friend class FlightPipelineBuilder;
};

/**
 * @brief 便捷函数：快速连接飞控
 */
inline ResultPtr<FlightPipeline> connectToFlightController(
    const std::string& connectionString,
    int baudRate = 57600) {
    
    return FlightPipeline::create()
        .withConnection(connectionString, baudRate)
        .build();
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
