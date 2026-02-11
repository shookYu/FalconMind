// NodeAgent - Logging system with levels
#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <mutex>
#include <memory>
#include <chrono>
#include <iomanip>

namespace nodeagent {

// 日志级别
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    FATAL = 4
};

// 日志系统（单例）
class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    // 设置日志级别
    void setLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(mutex_);
        level_ = level;
    }

    LogLevel getLevel() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return level_;
    }

    // 设置是否输出到控制台
    void setConsoleOutput(bool enable) {
        std::lock_guard<std::mutex> lock(mutex_);
        consoleOutput_ = enable;
    }

    // 日志输出
    void log(LogLevel level, const std::string& component, const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (level < level_) {
            return;  // 低于当前日志级别，不输出
        }

        if (!consoleOutput_) {
            return;  // 控制台输出已禁用
        }

        // 获取当前时间
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        // 格式化时间戳
        std::tm tm_buf;
#ifdef _WIN32
        localtime_s(&tm_buf, &time);
#else
        localtime_r(&time, &tm_buf);
#endif
        
        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();

        // 选择输出流
        std::ostream& stream = (level >= LogLevel::ERROR) ? std::cerr : std::cout;

        // 输出日志
        stream << "[" << oss.str() << "] "
               << "[" << levelToString(level) << "] "
               << "[" << component << "] "
               << message << std::endl;
    }

    // 便捷方法
    void debug(const std::string& component, const std::string& message) {
        log(LogLevel::DEBUG, component, message);
    }

    void info(const std::string& component, const std::string& message) {
        log(LogLevel::INFO, component, message);
    }

    void warn(const std::string& component, const std::string& message) {
        log(LogLevel::WARN, component, message);
    }

    void error(const std::string& component, const std::string& message) {
        log(LogLevel::ERROR, component, message);
    }

    void fatal(const std::string& component, const std::string& message) {
        log(LogLevel::FATAL, component, message);
    }

private:
    Logger() : level_(LogLevel::INFO), consoleOutput_(true) {}

    static const char* levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO ";
            case LogLevel::WARN: return "WARN ";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }

    LogLevel level_;
    bool consoleOutput_;
    mutable std::mutex mutex_;
};

// 便捷宏定义
#define LOG_DEBUG(component, msg) nodeagent::Logger::instance().debug(component, msg)
#define LOG_INFO(component, msg) nodeagent::Logger::instance().info(component, msg)
#define LOG_WARN(component, msg) nodeagent::Logger::instance().warn(component, msg)
#define LOG_ERROR(component, msg) nodeagent::Logger::instance().error(component, msg)
#define LOG_FATAL(component, msg) nodeagent::Logger::instance().fatal(component, msg)

} // namespace nodeagent
