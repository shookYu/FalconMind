// NodeAgent - ErrorStatistics unit tests
#include <gtest/gtest.h>
#include "nodeagent/ErrorStatistics.h"
#include "nodeagent/ErrorCodes.h"

#include <thread>
#include <chrono>
#include <vector>

using namespace nodeagent;

void registerErrorStatisticsTests() {
    // Tests are registered via TEST macros
}

TEST(ErrorStatisticsTest, BasicInitialization) {
    ErrorStatistics& stats = ErrorStatistics::instance();
    EXPECT_TRUE(true);
}

TEST(ErrorStatisticsTest, RecordError) {
    ErrorStatistics& stats = ErrorStatistics::instance();
    stats.reset();  // Clear previous stats
    
    stats.recordError(ErrorCode::ConnectionFailed, "Test connection failed");
    
    int64_t count = stats.getErrorCount(ErrorCode::ConnectionFailed);
    EXPECT_EQ(count, 1);
}

TEST(ErrorStatisticsTest, RecordMultipleErrors) {
    ErrorStatistics& stats = ErrorStatistics::instance();
    stats.reset();
    
    stats.recordError(ErrorCode::ConnectionFailed);
    stats.recordError(ErrorCode::ConnectionFailed);
    stats.recordError(ErrorCode::ConnectionFailed);
    
    int64_t count = stats.getErrorCount(ErrorCode::ConnectionFailed);
    EXPECT_EQ(count, 3);
}

TEST(ErrorStatisticsTest, RecordDifferentErrorCodes) {
    ErrorStatistics& stats = ErrorStatistics::instance();
    stats.reset();
    
    stats.recordError(ErrorCode::ConnectionFailed);
    stats.recordError(ErrorCode::SendFailed);
    stats.recordError(ErrorCode::MessageParseError);
    
    EXPECT_EQ(stats.getErrorCount(ErrorCode::ConnectionFailed), 1);
    EXPECT_EQ(stats.getErrorCount(ErrorCode::SendFailed), 1);
    EXPECT_EQ(stats.getErrorCount(ErrorCode::MessageParseError), 1);
}

TEST(ErrorStatisticsTest, GetTotalErrorCount) {
    ErrorStatistics& stats = ErrorStatistics::instance();
    stats.reset();
    
    stats.recordError(ErrorCode::ConnectionFailed);
    stats.recordError(ErrorCode::SendFailed);
    stats.recordError(ErrorCode::ConnectionFailed);
    
    int64_t total = stats.getTotalErrorCount();
    EXPECT_EQ(total, 3);
}

TEST(ErrorStatisticsTest, GetAllStats) {
    ErrorStatistics& stats = ErrorStatistics::instance();
    stats.reset();
    
    stats.recordError(ErrorCode::ConnectionFailed, "Connection failed message");
    stats.recordError(ErrorCode::SendFailed, "Send failed message");
    
    auto allStats = stats.getAllStats();
    EXPECT_EQ(allStats.size(), 2);
    
    EXPECT_GT(allStats[ErrorCode::ConnectionFailed].count, 0);
    EXPECT_GT(allStats[ErrorCode::SendFailed].count, 0);
}

TEST(ErrorStatisticsTest, ResetStatistics) {
    ErrorStatistics& stats = ErrorStatistics::instance();
    
    stats.recordError(ErrorCode::ConnectionFailed);
    EXPECT_GT(stats.getErrorCount(ErrorCode::ConnectionFailed), 0);
    
    stats.reset();
    EXPECT_EQ(stats.getErrorCount(ErrorCode::ConnectionFailed), 0);
    EXPECT_EQ(stats.getTotalErrorCount(), 0);
}

TEST(ErrorStatisticsTest, ThreadSafety) {
    ErrorStatistics& stats = ErrorStatistics::instance();
    stats.reset();
    
    // Test concurrent error recording
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&stats, i]() {
            for (int j = 0; j < 10; j++) {
                stats.recordError(ErrorCode::ConnectionFailed, 
                                 "Thread " + std::to_string(i) + " error " + std::to_string(j));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should have recorded all errors
    int64_t count = stats.getErrorCount(ErrorCode::ConnectionFailed);
    EXPECT_EQ(count, 100);  // 10 threads * 10 errors
}
