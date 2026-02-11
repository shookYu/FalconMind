// NodeAgent - Error statistics and monitoring
#pragma once

#include "nodeagent/ErrorCodes.h"

#include <map>
#include <mutex>
#include <chrono>
#include <atomic>
#include <string>

namespace nodeagent {

// 错误统计信息（简化版，用于返回，不包含 mutex）
struct ErrorStatsSummary {
    int64_t count;
    int64_t lastOccurrence;
    std::string lastMessage;
};

// 错误统计信息（内部使用，包含 mutex）
struct ErrorStats {
    std::atomic<int64_t> count{0};  // 错误计数
    std::atomic<int64_t> lastOccurrence{0};  // 最后发生时间（Unix 时间戳，秒）
    std::string lastMessage;  // 最后一条错误消息
    std::mutex messageMutex;  // 保护 lastMessage
};

// 错误统计管理器
class ErrorStatistics {
public:
    static ErrorStatistics& instance() {
        static ErrorStatistics stats;
        return stats;
    }

    // 记录错误
    void recordError(ErrorCode code, const std::string& message = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& stats = errorStats_[code];
        stats.count++;
        stats.lastOccurrence = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        if (!message.empty()) {
            std::lock_guard<std::mutex> msgLock(stats.messageMutex);
            stats.lastMessage = message;
        }
    }

    // 获取错误统计
    int64_t getErrorCount(ErrorCode code) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = errorStats_.find(code);
        if (it != errorStats_.end()) {
            return it->second.count.load();
        }
        return 0;
    }

    // 获取所有错误统计（返回简化版本，避免复制 mutex）
    std::map<ErrorCode, ErrorStatsSummary> getAllStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::map<ErrorCode, ErrorStatsSummary> result;
        for (const auto& [code, stats] : errorStats_) {
            ErrorStatsSummary summary;
            summary.count = stats.count.load();
            summary.lastOccurrence = stats.lastOccurrence.load();
            {
                // 注意：const_cast 是安全的，因为我们只是读取 lastMessage
                std::lock_guard<std::mutex> msgLock(
                    const_cast<std::mutex&>(stats.messageMutex));
                summary.lastMessage = stats.lastMessage;
            }
            result[code] = summary;
        }
        return result;
    }

    // 重置统计
    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        errorStats_.clear();
    }

    // 获取总错误数
    int64_t getTotalErrorCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        int64_t total = 0;
        for (const auto& [code, stats] : errorStats_) {
            total += stats.count.load();
        }
        return total;
    }

private:
    ErrorStatistics() = default;
    mutable std::mutex mutex_;
    std::map<ErrorCode, ErrorStats> errorStats_;
};

} // namespace nodeagent
