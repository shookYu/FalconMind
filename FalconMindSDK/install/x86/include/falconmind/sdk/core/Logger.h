/**
 * @file Logger.h
 * @brief 结构化日志系统
 * 
 * 提供分级别、分模块的日志记录，支持异步写入和多种输出目标
 * 
 * @example
 * @code
 * Logger::init({
 *     .level = LogLevel::Info,
 *     .outputFile = "mission.log",
 *     .enableConsole = true,
 *     .enableTelemetry = true
 * });
 * 
 * LOG_INFO("Mission") << "Starting mission: " << missionId;
 * LOG_WARN("Perception") << "Low confidence detection: " << confidence;
 * LOG_ERROR("Flight") << "Command failed: " << error.message();
 * @endcode
 */

#pragma once

#include "falconmind/sdk/core/ErrorCode.h"
#include <sstream>
#include <string>
#include <memory>
#include <chrono>
#include <fstream>
#include <queue>
#include <atomic>
#include <thread>
#include <mutex>
#include <sstream>
#include <string>
#include <memory>
#include <chrono>
#include <fstream>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>

namespace falconmind {
namespace sdk {
namespace core {

/**
 * @brief 日志级别
 */
enum class LogLevel {
    Trace = 0,    ///< 详细跟踪（开发调试用）
    Debug = 1,    ///< 调试信息
    Info = 2,     ///< 一般信息
    Warn = 3,     ///< 警告
    Error = 4,    ///< 错误
    Fatal = 5     ///< 致命错误
};

/**
 * @brief 日志条目
 */
struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string module;
    std::string message;
    std::string file;
    int line;
    std::string function;
    std::string threadId;
    
    // 额外上下文
    std::unordered_map<std::string, std::string> context;
};

/**
 * @brief 日志输出接口
 */
class LogOutput {
public:
    virtual ~LogOutput() = default;
    virtual void write(const LogEntry& entry) = 0;
    virtual void flush() = 0;
};

/**
 * @brief 控制台输出
 */
class ConsoleOutput : public LogOutput {
public:
    void write(const LogEntry& entry) override;
    void flush() override;
};

/**
 * @brief 文件输出
 */
class FileOutput : public LogOutput {
public:
    explicit FileOutput(const std::string& filepath, bool rotate = true);
    ~FileOutput();
    void write(const LogEntry& entry) override;
    void flush() override;
    
private:
    std::ofstream file_;
    std::string filepath_;
    bool rotate_;
    std::mutex mutex_;
    
    void rotateIfNeeded();
};

/**
 * @brief 遥测输出（实时上传到云端）
 */
class TelemetryOutput : public LogOutput {
public:
    using TelemetryCallback = std::function<void(const LogEntry&)>;
    
    explicit TelemetryOutput(TelemetryCallback callback);
    void write(const LogEntry& entry) override;
    void flush() override {}
    
private:
    TelemetryCallback callback_;
};

/**
 * @brief 日志配置
 */
struct LoggerConfig {
    LogLevel level = LogLevel::Info;
    std::string outputFile;
    bool enableConsole = true;
    bool enableTelemetry = false;
    bool async = true;
    size_t asyncQueueSize = 10000;
    bool colorizeConsole = true;
    std::string timeFormat = "%Y-%m-%d %H:%M:%S.%3f";
    std::vector<std::string> moduleFilters;  // 空表示不过滤
};

/**
 * @brief 日志系统
 */
class Logger {
public:
    /**
     * @brief 初始化日志系统
     */
    static void init(const LoggerConfig& config);
    
    /**
     * @brief 关闭日志系统
     */
    static void shutdown();
    
    /**
     * @brief 记录日志
     */
    static void log(LogLevel level, const std::string& module, 
                    const std::string& message,
                    const std::string& file = "", int line = 0, 
                    const std::string& function = "");
    
    /**
     * @brief 设置日志级别
     */
    static void setLevel(LogLevel level);
    
    /**
     * @brief 添加输出目标
     */
    static void addOutput(std::shared_ptr<LogOutput> output);
    
    /**
     * @brief 刷新所有输出
     */
    static void flush();
    
    /**
     * @brief 是否已初始化
     */
    static bool isInitialized();

private:
    Logger() = default;
    static Logger& instance();
    
    LoggerConfig config_;
    std::vector<std::shared_ptr<LogOutput>> outputs_;
    std::mutex mutex_;
    std::atomic<bool> initialized_{false};
    
    // 异步相关
    std::queue<LogEntry> queue_;
    std::mutex queueMutex_;
    std::condition_variable cv_;
    std::thread workerThread_;
    std::atomic<bool> running_{false};
    
    void asyncWorker();
    void processEntry(const LogEntry& entry);
    bool shouldLog(LogLevel level, const std::string& module) const;
};

/**
 * @brief 日志流类
 */
class LogStream {
public:
    LogStream(LogLevel level, const std::string& module,
              const std::string& file, int line, const std::string& function);
    ~LogStream();
    
    template<typename T>
    LogStream& operator<<(const T& value) {
        buffer_ << value;
        return *this;
    }
    
private:
    LogLevel level_;
    std::string module_;
    std::string file_;
    int line_;
    std::string function_;
    std::ostringstream buffer_;
};

// 宏定义简化日志记录
#define LOG_TRACE(module) \
    falconmind::sdk::core::LogStream(falconmind::sdk::core::LogLevel::Trace, module, __FILE__, __LINE__, __FUNCTION__)

#define LOG_DEBUG(module) \
    falconmind::sdk::core::LogStream(falconmind::sdk::core::LogLevel::Debug, module, __FILE__, __LINE__, __FUNCTION__)

#define LOG_INFO(module) \
    falconmind::sdk::core::LogStream(falconmind::sdk::core::LogLevel::Info, module, __FILE__, __LINE__, __FUNCTION__)

#define LOG_WARN(module) \
    falconmind::sdk::core::LogStream(falconmind::sdk::core::LogLevel::Warn, module, __FILE__, __LINE__, __FUNCTION__)

#define LOG_ERROR(module) \
    falconmind::sdk::core::LogStream(falconmind::sdk::core::LogLevel::Error, module, __FILE__, __LINE__, __FUNCTION__)

#define LOG_FATAL(module) \
    falconmind::sdk::core::LogStream(falconmind::sdk::core::LogLevel::Fatal, module, __FILE__, __LINE__, __FUNCTION__)

} // namespace core
} // namespace sdk
} // namespace falconmind
