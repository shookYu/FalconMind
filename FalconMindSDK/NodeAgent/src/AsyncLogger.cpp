/**
 * @file AsyncLogger.cpp
 * @brief Production-grade asynchronous logger implementation
 */

#include "nodeagent/AsyncLogger.h"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cstdio>

#ifdef __linux__
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace nodeagent {

namespace {
    // Format timestamp for human-readable logs
    std::string formatTimestamp(std::chrono::system_clock::time_point tp, 
                                const std::string& format) {
        auto time_t = std::chrono::system_clock::to_time_t(tp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            tp.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    // Get short filename (remove path)
    std::string getShortFilename(const char* file) {
        if (!file) return "";
        std::string path(file);
        size_t pos = path.find_last_of("/\\");
        if (pos != std::string::npos) {
            return path.substr(pos + 1);
        }
        return path;
    }
}

std::string logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

LogLevel logLevelFromString(const std::string& str) {
    std::string upper = str;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    if (upper == "TRACE") return LogLevel::TRACE;
    if (upper == "DEBUG") return LogLevel::DEBUG;
    if (upper == "INFO")  return LogLevel::INFO;
    if (upper == "WARN" || upper == "WARNING") return LogLevel::WARN;
    if (upper == "ERROR") return LogLevel::ERROR;
    if (upper == "FATAL") return LogLevel::FATAL;
    
    return LogLevel::INFO;  // Default
}

// Global logger instance
static std::unique_ptr<AsyncLogger> g_logger = nullptr;
static std::mutex g_loggerMutex;

AsyncLogger& getGlobalLogger() {
    std::lock_guard<std::mutex> lock(g_loggerMutex);
    if (!g_logger) {
        g_logger = std::make_unique<AsyncLogger>();
        // Auto-initialize with default config if not explicitly initialized
        g_logger->initialize(LoggerConfig{});
    }
    return *g_logger;
}

void initializeGlobalLogger(const LoggerConfig& config) {
    std::lock_guard<std::mutex> lock(g_loggerMutex);
    if (!g_logger) {
        g_logger = std::make_unique<AsyncLogger>();
    }
    g_logger->initialize(config);
}

void shutdownGlobalLogger() {
    std::lock_guard<std::mutex> lock(g_loggerMutex);
    if (g_logger) {
        g_logger->shutdown();
        g_logger.reset();
    }
}

AsyncLogger::AsyncLogger()
    : running_(false)
    , minLevel_(LogLevel::INFO)
    , flushRequested_(false)
    , currentFileSize_(0)
{
    stats_.startTime = std::chrono::steady_clock::now();
}

AsyncLogger::~AsyncLogger() {
    shutdown();
}

bool AsyncLogger::initialize(const LoggerConfig& config) {
    if (running_) {
        // Already initialized
        return true;
    }
    
    config_ = config;
    minLevel_ = config.minLevel;
    componentLevels_ = config.componentLevels;
    
    // Create log directory if it doesn't exist
    if (!std::filesystem::exists(config_.logDirectory)) {
        try {
            std::filesystem::create_directories(config_.logDirectory);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create log directory: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Open log file
    if (config_.outputToFile) {
        if (!openLogFile()) {
            std::cerr << "Failed to open log file" << std::endl;
            return false;
        }
    }
    
    running_ = true;
    lastRotationTime_ = std::chrono::system_clock::now();
    
    // Start writer thread for async mode
    if (config_.async) {
        writerThread_ = std::thread(&AsyncLogger::writerThreadFunc, this);
    }
    
    // Start flush thread for periodic flushing
    flushThread_ = std::thread(&AsyncLogger::flushThreadFunc, this);
    
    // Start forwarder thread if forwarder is set
    if (forwarder_) {
        forwarderThread_ = std::thread(&AsyncLogger::forwarderThreadFunc, this);
    }
    
    return true;
}

void AsyncLogger::shutdown() {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (!running_) {
            return;
        }
        running_ = false;
    }
    
    queueCv_.notify_all();
    
    // Wait for writer thread to finish
    if (writerThread_.joinable()) {
        writerThread_.join();
    }
    
    // Wait for flush thread to finish
    if (flushThread_.joinable()) {
        flushThread_.join();
    }
    
    // Wait for forwarder thread to finish
    if (forwarderThread_.joinable()) {
        forwarderThread_.join();
    }
    
    // Flush remaining logs synchronously
    std::queue<LogEntry> remaining;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        std::swap(remaining, logQueue_);
    }
    
    while (!remaining.empty()) {
        auto entry = remaining.front();
        remaining.pop();
        
        std::string formatted = formatLogEntry(entry);
        
        if (config_.outputToConsole) {
            std::cout << formatted << std::endl;
        }
        
        if (config_.outputToFile && logFile_.is_open()) {
            logFile_ << formatted << std::endl;
        }
        
        // Call log callback
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            if (logCallback_) {
                logCallback_(entry);
            }
        }
    }
    
    // Close log file
    closeLogFile();
}

void AsyncLogger::log(LogLevel level,
                      const std::string& component,
                      const std::string& message,
                      const char* file,
                      int line,
                      const char* function,
                      const nlohmann::json& context,
                      const std::string& traceId) {
    if (!shouldLog(level, component)) {
        std::lock_guard<std::mutex> lock(statsMutex_);
        ++stats_.totalLogsFiltered;
        return;
    }
    
    LogEntry entry;
    entry.timestamp = std::chrono::system_clock::now();
    entry.level = level;
    entry.component = component;
    entry.message = message;
    entry.file = file ? file : "";
    entry.line = line;
    entry.function = function ? function : "";
    entry.context = context;
    entry.threadId = std::this_thread::get_id();
    entry.traceId = traceId;
    
    if (config_.async) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        
        if (logQueue_.size() >= config_.queueSize) {
            // Queue is full, drop the log
            std::lock_guard<std::mutex> statsLock(statsMutex_);
            ++stats_.totalLogsDropped;
            return;
        }
        
        logQueue_.push(entry);
        queueCv_.notify_one();
    } else {
        // Synchronous logging
        std::string formatted = formatLogEntry(entry);
        
        if (config_.outputToConsole) {
            std::cout << formatted << std::endl;
        }
        
        if (config_.outputToFile && logFile_.is_open()) {
            std::lock_guard<std::mutex> lock(fileMutex_);
            logFile_ << formatted << std::endl;
            currentFileSize_ += formatted.length() + 1;
            
            if (shouldRotate()) {
                doRotation();
            }
        }
        
        // Call log callback
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            if (logCallback_) {
                logCallback_(entry);
            }
        }
        
        std::lock_guard<std::mutex> lock(statsMutex_);
        ++stats_.totalLogsLogged;
    }
}

void AsyncLogger::log(LogLevel level,
                      const std::string& component,
                      const std::string& message) {
    log(level, component, message, "", 0, "");
}

void AsyncLogger::trace(const std::string& component, const std::string& message,
                        const char* file, int line, const char* function) {
    log(LogLevel::TRACE, component, message, file, line, function);
}

void AsyncLogger::debug(const std::string& component, const std::string& message,
                        const char* file, int line, const char* function) {
    log(LogLevel::DEBUG, component, message, file, line, function);
}

void AsyncLogger::info(const std::string& component, const std::string& message,
                       const char* file, int line, const char* function) {
    log(LogLevel::INFO, component, message, file, line, function);
}

void AsyncLogger::warn(const std::string& component, const std::string& message,
                       const char* file, int line, const char* function) {
    log(LogLevel::WARN, component, message, file, line, function);
}

void AsyncLogger::error(const std::string& component, const std::string& message,
                        const char* file, int line, const char* function) {
    log(LogLevel::ERROR, component, message, file, line, function);
}

void AsyncLogger::fatal(const std::string& component, const std::string& message,
                        const char* file, int line, const char* function) {
    log(LogLevel::FATAL, component, message, file, line, function);
}

void AsyncLogger::flush() {
    flushRequested_ = true;
    queueCv_.notify_all();
    
    if (config_.outputToFile && logFile_.is_open()) {
        std::lock_guard<std::mutex> lock(fileMutex_);
        logFile_.flush();
    }
}

void AsyncLogger::setMinLevel(LogLevel level) {
    minLevel_ = level;
}

void AsyncLogger::setComponentLevel(const std::string& component, LogLevel level) {
    std::lock_guard<std::mutex> lock(componentMutex_);
    componentLevels_[component] = level;
}

bool AsyncLogger::rotateLogFile() {
    std::lock_guard<std::mutex> lock(fileMutex_);
    doRotation();
    return true;
}

std::string AsyncLogger::getCurrentLogFile() const {
    std::lock_guard<std::mutex> lock(fileMutex_);
    return currentLogFilePath_;
}

AsyncLogger::Statistics AsyncLogger::getStatistics() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    auto stats = stats_;
    {
        std::lock_guard<std::mutex> qlock(queueMutex_);
        stats.currentQueueSize = logQueue_.size();
    }
    {
        std::lock_guard<std::mutex> flock(fileMutex_);
        stats.currentFileSize = currentFileSize_;
    }
    return stats;
}

void AsyncLogger::setLogCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    logCallback_ = callback;
}

void AsyncLogger::setLogForwarder(LogForwarder forwarder, std::chrono::seconds batchInterval) {
    forwarder_ = forwarder;
    forwardInterval_ = batchInterval;
}

void AsyncLogger::writerThreadFunc() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        
        queueCv_.wait(lock, [this] {
            return !running_ || !logQueue_.empty() || flushRequested_;
        });
        
        std::queue<LogEntry> batch;
        std::swap(batch, logQueue_);
        flushRequested_ = false;
        lock.unlock();
        
        while (!batch.empty()) {
            auto entry = batch.front();
            batch.pop();
            
            std::string formatted = formatLogEntry(entry);
            
            // Console output
            if (config_.outputToConsole) {
                std::cout << formatted << std::endl;
            }
            
            // File output
            if (config_.outputToFile) {
                std::lock_guard<std::mutex> fileLock(fileMutex_);
                
                if (logFile_.is_open()) {
                    logFile_ << formatted << std::endl;
                    currentFileSize_ += formatted.length() + 1;
                    
                    if (shouldRotate()) {
                        doRotation();
                    }
                }
            }
            
            // Callback
            {
                std::lock_guard<std::mutex> cbLock(callbackMutex_);
                if (logCallback_) {
                    logCallback_(entry);
                }
            }
            
            // Forwarder buffer
            if (forwarder_) {
                std::lock_guard<std::mutex> fwdLock(forwardMutex_);
                forwardBuffer_.push_back(entry);
            }
            
            {
                std::lock_guard<std::mutex> statsLock(statsMutex_);
                ++stats_.totalLogsLogged;
            }
        }
    }
}

void AsyncLogger::flushThreadFunc() {
    while (running_) {
        std::this_thread::sleep_for(config_.flushInterval);
        
        if (!running_) break;
        
        flush();
        
        // Check for time-based rotation
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::hours>(
            now - lastRotationTime_);
        
        if (elapsed >= config_.rotationInterval) {
            rotateLogFile();
        }
    }
}

void AsyncLogger::forwarderThreadFunc() {
    while (running_) {
        std::this_thread::sleep_for(forwardInterval_);
        
        if (!running_) break;
        
        std::vector<LogEntry> batch;
        {
            std::lock_guard<std::mutex> lock(forwardMutex_);
            if (!forwardBuffer_.empty()) {
                std::swap(batch, forwardBuffer_);
            }
        }
        
        if (!batch.empty() && forwarder_) {
            forwarder_(batch);
        }
    }
}

bool AsyncLogger::openLogFile() {
    closeLogFile();
    
    std::string filePath = generateLogFileName();
    currentLogFilePath_ = filePath;
    
    logFile_.open(filePath, std::ios::app);
    if (!logFile_.is_open()) {
        return false;
    }
    
    // Get current file size
    logFile_.seekp(0, std::ios::end);
    currentFileSize_ = logFile_.tellp();
    
    return true;
}

void AsyncLogger::closeLogFile() {
    if (logFile_.is_open()) {
        logFile_.flush();
        logFile_.close();
    }
}

bool AsyncLogger::shouldRotate() const {
    return currentFileSize_ >= config_.maxFileSize;
}

void AsyncLogger::doRotation() {
    // Close current file
    closeLogFile();
    
    // Compress old file if enabled
    if (config_.compressOldFiles && !currentLogFilePath_.empty()) {
        compressFile(currentLogFilePath_);
    }
    
    // Open new file
    openLogFile();
    lastRotationTime_ = std::chrono::system_clock::now();
    currentFileSize_ = 0;
    
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        ++stats_.totalRotations;
    }
    
    // Cleanup old logs
    cleanupOldLogs();
}

std::string AsyncLogger::generateLogFileName() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << config_.logDirectory << "/" << config_.logFileName << "_";
    ss << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S");
    ss << ".log";
    
    return ss.str();
}

std::string AsyncLogger::formatLogEntry(const LogEntry& entry) const {
    if (config_.useJsonFormat) {
        return logEntryToJson(entry).dump();
    }
    
    // Human-readable format
    std::ostringstream oss;
    oss << formatTimestamp(entry.timestamp, config_.timeFormat);
    oss << " [" << std::setw(5) << logLevelToString(entry.level) << "]";
    oss << " [" << entry.component << "]";
    
    if (!entry.traceId.empty()) {
        oss << " [trace=" << entry.traceId << "]";
    }
    
    oss << " " << entry.message;
    
    if (!entry.file.empty()) {
        oss << " (" << getShortFilename(entry.file.c_str()) << ":" << entry.line;
        if (!entry.function.empty()) {
            oss << " " << entry.function;
        }
        oss << ")";
    }
    
    if (!entry.context.empty()) {
        oss << " context=" << entry.context.dump();
    }
    
    return oss.str();
}

nlohmann::json AsyncLogger::logEntryToJson(const LogEntry& entry) const {
    nlohmann::json j;
    j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(
        entry.timestamp.time_since_epoch()).count();
    j["level"] = logLevelToString(entry.level);
    j["component"] = entry.component;
    j["message"] = entry.message;
    
    if (!entry.file.empty()) {
        j["file"] = entry.file;
        j["line"] = entry.line;
    }
    
    if (!entry.function.empty()) {
        j["function"] = entry.function;
    }
    
    if (!entry.traceId.empty()) {
        j["traceId"] = entry.traceId;
    }
    
    // Thread ID as string
    std::ostringstream oss;
    oss << entry.threadId;
    j["threadId"] = oss.str();
    
    if (!entry.context.empty()) {
        j["context"] = entry.context;
    }
    
    return j;
}

bool AsyncLogger::shouldLog(LogLevel level, const std::string& component) const {
    // Check global level
    if (level < minLevel_) {
        return false;
    }
    
    // Check component-specific level
    std::lock_guard<std::mutex> lock(componentMutex_);
    auto it = componentLevels_.find(component);
    if (it != componentLevels_.end()) {
        return level >= it->second;
    }
    
    return true;
}

void AsyncLogger::compressFile(const std::string& filePath) {
#ifdef __linux__
    // Use gzip for compression
    std::string cmd = "gzip -f " + filePath;
    int ret = std::system(cmd.c_str());
    (void)ret;  // Suppress unused warning
#endif
}

void AsyncLogger::cleanupOldLogs() {
    if (config_.maxFiles == 0) return;
    
    try {
        std::vector<std::filesystem::directory_entry> logFiles;
        
        for (const auto& entry : std::filesystem::directory_iterator(config_.logDirectory)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find(config_.logFileName) == 0) {
                    logFiles.push_back(entry);
                }
            }
        }
        
        // Sort by modification time (oldest first)
        std::sort(logFiles.begin(), logFiles.end(),
                  [](const auto& a, const auto& b) {
                      return a.last_write_time() < b.last_write_time();
                  });
        
        // Remove old files beyond maxFiles
        while (logFiles.size() > config_.maxFiles) {
            std::filesystem::remove(logFiles.front().path());
            logFiles.erase(logFiles.begin());
        }
    } catch (const std::exception& e) {
        // Log cleanup errors to stderr since logger might be in bad state
        std::cerr << "Error cleaning up old logs: " << e.what() << std::endl;
    }
}

} // namespace nodeagent
