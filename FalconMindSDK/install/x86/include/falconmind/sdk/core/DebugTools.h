/**
 * @file DebugTools.h
 * @brief 开发调试工具集
 * 
 * 提供Pipeline可视化、实时数据查看、模拟模式等开发工具
 * 
 * @example
 * @code
 * // 启动调试服务器
 * DebugServer server(8080);
 * server.start();
 * 
 * // 添加Pipeline监控
 * server.monitorPipeline(pipeline);
 * 
 * // 浏览器访问 http://localhost:8080 查看可视化界面
 * @endcode
 */

#pragma once

#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Telemetry.h"
#include <string>
#include <map>
#include <functional>

namespace falconmind {
namespace sdk {
namespace core {

/**
 * @brief Pipeline可视化数据
 */
struct PipelineVisualization {
    struct NodeInfo {
        std::string id;
        std::string type;
        std::string state;
        float processTimeMs;
        int inputQueueSize;
        int outputQueueSize;
        bool isActive;
        std::vector<std::string> inputPads;
        std::vector<std::string> outputPads;
    };
    
    struct ConnectionInfo {
        std::string fromNode;
        std::string fromPad;
        std::string toNode;
        std::string toPad;
        bool isActive;
        double dataRateBps;
    };
    
    std::string pipelineId;
    std::string state;
    std::vector<NodeInfo> nodes;
    std::vector<ConnectionInfo> connections;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief 调试服务器
 * 
 * 提供Web界面用于：
 * - Pipeline拓扑可视化
 * - 节点数据流监控
 * - 实时日志查看
 * - 参数动态调整
 */
class DebugServer {
public:
    explicit DebugServer(int port = 8080);
    ~DebugServer();
    
    /**
     * @brief 启动/停止服务器
     */
    void start();
    void stop();
    
    /**
     * @brief 监控Pipeline
     */
    void monitorPipeline(std::shared_ptr<Pipeline> pipeline);
    void unmonitorPipeline(const std::string& pipelineId);
    
    /**
     * @brief 注册自定义数据提供者
     */
    using DataProvider = std::function<std::string()>;
    void registerDataProvider(const std::string& name, DataProvider provider);
    
    /**
     * @brief 注册控制命令处理器
     */
    using CommandHandler = std::function<std::string(const std::map<std::string, std::string>&)>;
    void registerCommand(const std::string& command, CommandHandler handler);
    
    /**
     * @brief 获取服务器地址
     */
    std::string getAddress() const;
    
    /**
     * @brief 是否正在运行
     */
    bool isRunning() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief 模拟数据生成器
 * 
 * 用于无真机环境下的开发和测试
 */
class Simulator {
public:
    Simulator();
    ~Simulator();
    
    /**
     * @brief 配置模拟场景
     */
    struct Scenario {
        std::string name;
        sensors::GeoPoint startPosition;
        std::vector<sensors::GeoPoint> waypoints;
        std::vector<perception::Detection> simulatedDetections;
        double flightDurationMinutes;
        bool simulateWind;
        double windSpeed;
        double windDirection;
    };
    
    void loadScenario(const Scenario& scenario);
    void loadScenarioFromFile(const std::string& filepath);
    
    /**
     * @brief 模拟数据源
     */
    std::shared_ptr<sensors::CameraSourceNode> createCameraSource();
    std::shared_ptr<sensors::GnssSourceNode> createGnssSource();
    std::shared_ptr<sensors::ImuSourceNode> createImuSource();
    
    /**
     * @brief 模拟MAVLink连接
     */
    std::shared_ptr<flight::FlightConnectionService> createFlightConnection();
    
    /**
     * @brief 控制模拟
     */
    void start();
    void pause();
    void resume();
    void stop();
    void setSpeed(float speedMultiplier);  // 0.1x - 10x
    
    /**
     * @brief 注入事件
     */
    void injectDetection(const perception::Detection& detection);
    void injectError(const std::string& component, const std::string& error);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief 数据记录和回放
 */
class DataRecorder {
public:
    /**
     * @brief 开始记录
     */
    void startRecording(const std::string& sessionName);
    void stopRecording();
    
    /**
     * @brief 回放记录
     */
    void loadRecording(const std::string& filepath);
    void play();
    void pause();
    void seek(double timestamp);
    void setPlaybackSpeed(float speed);
    
    /**
     * @brief 回放回调
     */
    using FrameCallback = std::function<void(double timestamp, const std::string& dataType, const void* data, size_t size)>;
    void onFrame(FrameCallback callback);
    
    /**
     * @brief 获取记录信息
     */
    struct RecordingInfo {
        std::string name;
        std::chrono::system_clock::time_point startTime;
        std::chrono::seconds duration;
        size_t totalFrames;
        size_t fileSize;
        std::vector<std::string> dataTypes;
    };
    RecordingInfo getInfo() const;

private:
    bool recording_ = false;
    std::string currentSession_;
};

/**
 * @brief 性能分析器
 */
class Profiler {
public:
    /**
     * @brief 开始/结束分析
     */
    void start(const std::string& name);
    void end(const std::string& name);
    
    /**
     * @brief 自动作用域分析
     */
    class Scope {
    public:
        Scope(Profiler& profiler, const std::string& name);
        ~Scope();
    private:
        Profiler& profiler_;
        std::string name_;
    };
    
    /**
     * @brief 获取统计信息
     */
    struct Stats {
        std::string name;
        int count;
        double totalMs;
        double avgMs;
        double minMs;
        double maxMs;
    };
    std::vector<Stats> getStats() const;
    
    /**
     * @brief 打印报告
     */
    void printReport() const;
    
    /**
     * @brief 重置统计
     */
    void reset();

private:
    struct Entry {
        std::chrono::high_resolution_clock::time_point start;
        int count = 0;
        double totalMs = 0;
        double minMs = std::numeric_limits<double>::max();
        double maxMs = 0;
    };
    std::map<std::string, Entry> entries_;
    mutable std::mutex mutex_;
};

// 便利宏
#define PROFILE_SCOPE(profiler, name) \
    falconmind::sdk::core::Profiler::Scope _profile_scope_##__LINE__(profiler, name)

} // namespace core
} // namespace sdk
} // namespace falconmind
