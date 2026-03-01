/**
 * @file test_async_logger.cpp
 * @brief Comprehensive unit tests for AsyncLogger
 * 
 * Tests cover:
 * - Logger initialization and shutdown
 * - Log level filtering
 * - Synchronous and asynchronous logging
 * - Log rotation (size-based and time-based)
 * - Component-level filtering
 * - JSON and human-readable format output
 * - Thread safety under concurrent logging
 * - Log callback functionality
 * - Statistics tracking
 * 
 * @note Zero-mock testing - uses real AsyncLogger with temp directories
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "nodeagent/AsyncLogger.h"
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>
#include <chrono>
#include <regex>

using namespace nodeagent;
using namespace testing;

class AsyncLoggerTest : public Test {
protected:
    void SetUp() override {
        // Create temp directory for test logs
        tempDir_ = std::filesystem::temp_directory_path() / "async_logger_test";
        std::filesystem::create_directories(tempDir_);
        
        // Clean any existing files
        for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
            std::filesystem::remove_all(entry.path());
        }
    }
    
    void TearDown() override {
        if (logger_ && logger_->isRunning()) {
            logger_->shutdown();
        }
        logger_.reset();
        
        // Clean up temp directory
        std::filesystem::remove_all(tempDir_);
    }
    
    std::string readLogFile(const std::string& filename) {
        std::ifstream file(tempDir_ / filename);
        if (!file.is_open()) return "";
        
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    int countLogFiles() {
        int count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".log") {
                ++count;
            }
        }
        return count;
    }
    
    std::unique_ptr<AsyncLogger> logger_;
    std::filesystem::path tempDir_;
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(AsyncLoggerTest, InitializeCreatesLogDirectory) {
    auto testDir = tempDir_ / "subdir" / "logs";
    
    LoggerConfig config;
    config.logDirectory = testDir.string();
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    EXPECT_TRUE(logger_->initialize(config));
    EXPECT_TRUE(std::filesystem::exists(testDir));
}

TEST_F(AsyncLoggerTest, InitializeWithFileOutput) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.outputToFile = true;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    EXPECT_TRUE(logger_->initialize(config));
    EXPECT_TRUE(logger_->isRunning());
    
    logger_->info("TestComponent", "Test message");
    logger_->flush();
    
    EXPECT_EQ(countLogFiles(), 1);
}

TEST_F(AsyncLoggerTest, InitializeIsIdempotent) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    EXPECT_TRUE(logger_->initialize(config));
    EXPECT_TRUE(logger_->initialize(config));  // Second call should succeed
}

TEST_F(AsyncLoggerTest, ShutdownFlushesPendingLogs) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.async = true;  // Use async mode
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->info("Test", "Message 1");
    logger_->info("Test", "Message 2");
    
    logger_->shutdown();
    
    // After shutdown, logs should be flushed to file
    EXPECT_EQ(countLogFiles(), 1);
}

// ============================================================================
// Log Level Tests
// ============================================================================

TEST_F(AsyncLoggerTest, LogLevelFiltering) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.minLevel = LogLevel::WARN;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->trace("Test", "Trace message");
    logger_->debug("Test", "Debug message");
    logger_->info("Test", "Info message");
    logger_->warn("Test", "Warning message");
    logger_->error("Test", "Error message");
    logger_->fatal("Test", "Fatal message");
    
    logger_->flush();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".log") {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    // Only WARN and above should be logged
    EXPECT_THAT(content, Not(HasSubstr("Trace message")));
    EXPECT_THAT(content, Not(HasSubstr("Debug message")));
    EXPECT_THAT(content, Not(HasSubstr("Info message")));
    EXPECT_THAT(content, HasSubstr("Warning message"));
    EXPECT_THAT(content, HasSubstr("Error message"));
    EXPECT_THAT(content, HasSubstr("Fatal message"));
}

TEST_F(AsyncLoggerTest, SetMinLevelAtRuntime) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.minLevel = LogLevel::INFO;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->debug("Test", "Before change");
    
    logger_->setMinLevel(LogLevel::DEBUG);
    
    logger_->debug("Test", "After change");
    logger_->flush();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    EXPECT_THAT(content, Not(HasSubstr("Before change")));
    EXPECT_THAT(content, HasSubstr("After change"));
}

// ============================================================================
// Component-Level Filtering Tests
// ============================================================================

TEST_F(AsyncLoggerTest, ComponentLevelFiltering) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.minLevel = LogLevel::DEBUG;
    config.componentLevels["VerboseComponent"] = LogLevel::WARN;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->info("VerboseComponent", "Verbose info");
    logger_->info("NormalComponent", "Normal info");
    logger_->warn("VerboseComponent", "Verbose warning");
    
    logger_->flush();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    EXPECT_THAT(content, Not(HasSubstr("Verbose info")));
    EXPECT_THAT(content, HasSubstr("Normal info"));
    EXPECT_THAT(content, HasSubstr("Verbose warning"));
}

TEST_F(AsyncLoggerTest, SetComponentLevelAtRuntime) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.minLevel = LogLevel::DEBUG;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->setComponentLevel("Restricted", LogLevel::ERROR);
    
    logger_->warn("Restricted", "Warning message");
    logger_->error("Restricted", "Error message");
    logger_->flush();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    EXPECT_THAT(content, Not(HasSubstr("Warning message")));
    EXPECT_THAT(content, HasSubstr("Error message"));
}

// ============================================================================
// JSON Format Tests
// ============================================================================

TEST_F(AsyncLoggerTest, JsonFormatOutput) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.useJsonFormat = true;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->info("TestComponent", "Test message");
    logger_->flush();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    // Should be valid JSON
    EXPECT_THAT(content, HasSubstr("\"timestamp\""));
    EXPECT_THAT(content, HasSubstr("\"level\""));
    EXPECT_THAT(content, HasSubstr("\"INFO\""));
    EXPECT_THAT(content, HasSubstr("\"component\""));
    EXPECT_THAT(content, HasSubstr("\"TestComponent\""));
    EXPECT_THAT(content, HasSubstr("\"message\""));
    EXPECT_THAT(content, HasSubstr("\"Test message\""));
}

TEST_F(AsyncLoggerTest, JsonFormatWithContext) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.useJsonFormat = true;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    nlohmann::json ctx;
    ctx["request_id"] = "12345";
    ctx["user_id"] = "user_123";
    
    logger_->log(LogLevel::INFO, "API", "Request processed", "", 0, "", ctx);
    logger_->flush();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    EXPECT_THAT(content, HasSubstr("\"context\""));
    EXPECT_THAT(content, HasSubstr("\"request_id\""));
    EXPECT_THAT(content, HasSubstr("\"12345\""));
}

// ============================================================================
// Human-Readable Format Tests
// ============================================================================

TEST_F(AsyncLoggerTest, HumanReadableFormatOutput) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.useJsonFormat = false;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->info("TestComponent", "Test message");
    logger_->flush();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    // Should contain timestamp, level, component, and message
    EXPECT_THAT(content, HasSubstr(" [INFO]"));
    EXPECT_THAT(content, HasSubstr("[TestComponent]"));
    EXPECT_THAT(content, HasSubstr("Test message"));
}

TEST_F(AsyncLoggerTest, HumanReadableFormatWithSourceLocation) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.useJsonFormat = false;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->info("Test", "Message", __FILE__, __LINE__, __func__);
    logger_->flush();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    EXPECT_THAT(content, HasSubstr("test_async_logger.cpp"));
    EXPECT_THAT(content, HasSubstr("HumanReadableFormatWithSourceLocation"));
}

// ============================================================================
// Log Rotation Tests
// ============================================================================

TEST_F(AsyncLoggerTest, SizeBasedRotation) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.maxFileSize = 500;  // 500 bytes - small for testing
    config.maxFiles = 3;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    // Write enough logs to trigger rotation
    for (int i = 0; i < 20; ++i) {
        logger_->info("Test", "This is a test log message that should cause rotation when repeated");
    }
    
    logger_->flush();
    
    // Should have multiple log files due to rotation
    EXPECT_GE(countLogFiles(), 2);
}

TEST_F(AsyncLoggerTest, ManualRotation) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->info("Test", "Message 1");
    logger_->rotateLogFile();
    logger_->info("Test", "Message 2");
    logger_->flush();
    
    EXPECT_EQ(countLogFiles(), 2);
}

TEST_F(AsyncLoggerTest, MaxFilesLimit) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.maxFileSize = 100;  // Very small
    config.maxFiles = 2;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    // Write many logs to create multiple rotations
    for (int i = 0; i < 50; ++i) {
        logger_->info("Test", "Message that will cause rotation very quickly indeed");
    }
    
    logger_->flush();
    
    // Should not exceed maxFiles
    EXPECT_LE(countLogFiles(), config.maxFiles);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(AsyncLoggerTest, StatisticsTracking) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.minLevel = LogLevel::DEBUG;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->debug("Test", "Debug 1");
    logger_->info("Test", "Info 1");
    logger_->info("Test", "Info 2");
    logger_->warn("Test", "Warning 1");
    
    auto stats = logger_->getStatistics();
    
    EXPECT_EQ(stats.totalLogsLogged, 4);
    EXPECT_GE(stats.uptime.count(), 0);
}

TEST_F(AsyncLoggerTest, StatisticsFilteredLogs) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.minLevel = LogLevel::WARN;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->debug("Test", "Debug");  // Filtered
    logger_->info("Test", "Info");    // Filtered
    logger_->warn("Test", "Warning"); // Logged
    
    auto stats = logger_->getStatistics();
    
    EXPECT_EQ(stats.totalLogsLogged, 1);
    EXPECT_EQ(stats.totalLogsFiltered, 2);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(AsyncLoggerTest, ConcurrentLogging) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.async = true;  // Use async mode
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    const int numThreads = 10;
    const int logsPerThread = 100;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < logsPerThread; ++j) {
                logger_->info("Thread", "Message from thread " + std::to_string(i) + 
                             " iteration " + std::to_string(j));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    logger_->shutdown();
    
    auto stats = logger_->getStatistics();
    EXPECT_EQ(stats.totalLogsLogged, numThreads * logsPerThread);
}

TEST_F(AsyncLoggerTest, ConcurrentMixedLevelLogging) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.async = true;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    const int numThreads = 5;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < 20; ++j) {
                switch (j % 6) {
                    case 0: logger_->trace("Test", "Trace"); break;
                    case 1: logger_->debug("Test", "Debug"); break;
                    case 2: logger_->info("Test", "Info"); break;
                    case 3: logger_->warn("Test", "Warn"); break;
                    case 4: logger_->error("Test", "Error"); break;
                    case 5: logger_->fatal("Test", "Fatal"); break;
                }
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    logger_->shutdown();
    
    auto stats = logger_->getStatistics();
    EXPECT_EQ(stats.totalLogsLogged, numThreads * 20);
}

// ============================================================================
// Log Callback Tests
// ============================================================================

TEST_F(AsyncLoggerTest, LogCallbackInvoked) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    int callbackCount = 0;
    std::string lastComponent;
    std::string lastMessage;
    
    logger_->setLogCallback([&](const LogEntry& entry) {
        ++callbackCount;
        lastComponent = entry.component;
        lastMessage = entry.message;
    });
    
    logger_->info("TestComponent", "Test message");
    logger_->flush();
    
    EXPECT_EQ(callbackCount, 1);
    EXPECT_EQ(lastComponent, "TestComponent");
    EXPECT_EQ(lastMessage, "Test message");
}

TEST_F(AsyncLoggerTest, LogCallbackWithMultipleLogs) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    int callbackCount = 0;
    logger_->setLogCallback([&](const LogEntry&) {
        ++callbackCount;
    });
    
    for (int i = 0; i < 10; ++i) {
        logger_->info("Test", "Message " + std::to_string(i));
    }
    logger_->flush();
    
    EXPECT_EQ(callbackCount, 10);
}

// ============================================================================
// Trace ID Tests
// ============================================================================

TEST_F(AsyncLoggerTest, TraceIdInJsonFormat) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.useJsonFormat = true;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    nlohmann::json ctx;
    logger_->log(LogLevel::INFO, "Test", "Message", "", 0, "", ctx, "trace-123-abc");
    logger_->flush();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    EXPECT_THAT(content, HasSubstr("\"traceId\""));
    EXPECT_THAT(content, HasSubstr("\"trace-123-abc\""));
}

TEST_F(AsyncLoggerTest, TraceIdInHumanFormat) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.useJsonFormat = false;
    config.async = false;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    nlohmann::json ctx;
    logger_->log(LogLevel::INFO, "Test", "Message", "", 0, "", ctx, "trace-123-abc");
    logger_->flush();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    EXPECT_THAT(content, HasSubstr("[trace=trace-123-abc]"));
}

// ============================================================================
// Synchronous Mode Tests
// ============================================================================

TEST_F(AsyncLoggerTest, SynchronousLoggingImmediateWrite) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.async = false;  // Synchronous mode
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    logger_->info("Test", "Immediate message");
    // No flush needed in sync mode - should be written immediately
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    EXPECT_THAT(content, HasSubstr("Immediate message"));
}

// ============================================================================
// Complex Scenario Tests
// ============================================================================

TEST_F(AsyncLoggerTest, ComplexScenarioMixedOperations) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.minLevel = LogLevel::DEBUG;
    config.useJsonFormat = false;
    config.async = true;
    
    logger_ = std::make_unique<AsyncLogger>();
    ASSERT_TRUE(logger_->initialize(config));
    
    // Log at different levels
    logger_->debug("ComponentA", "Debug message");
    logger_->info("ComponentA", "Info message");
    logger_->warn("ComponentB", "Warning message");
    logger_->error("ComponentB", "Error message");
    
    // Change log level at runtime
    logger_->setComponentLevel("ComponentA", LogLevel::WARN);
    
    // These should be filtered
    logger_->debug("ComponentA", "Filtered debug");
    logger_->info("ComponentA", "Filtered info");
    
    // These should pass
    logger_->warn("ComponentA", "Post-change warning");
    logger_->info("ComponentB", "ComponentB info still works");
    
    logger_->shutdown();
    
    std::string content;
    for (const auto& entry : std::filesystem::directory_iterator(tempDir_)) {
        if (entry.is_regular_file()) {
            content = readLogFile(entry.path().filename().string());
            break;
        }
    }
    
    EXPECT_THAT(content, HasSubstr("Debug message"));
    EXPECT_THAT(content, HasSubstr("Info message"));
    EXPECT_THAT(content, HasSubstr("Warning message"));
    EXPECT_THAT(content, HasSubstr("Error message"));
    EXPECT_THAT(content, Not(HasSubstr("Filtered debug")));
    EXPECT_THAT(content, Not(HasSubstr("Filtered info")));
    EXPECT_THAT(content, HasSubstr("Post-change warning"));
    EXPECT_THAT(content, HasSubstr("ComponentB info still works"));
}

// ============================================================================
// Helper Function Tests
// ============================================================================

TEST(LogLevelConversion, LogLevelToString) {
    EXPECT_EQ(logLevelToString(LogLevel::TRACE), "TRACE");
    EXPECT_EQ(logLevelToString(LogLevel::DEBUG), "DEBUG");
    EXPECT_EQ(logLevelToString(LogLevel::INFO), "INFO");
    EXPECT_EQ(logLevelToString(LogLevel::WARN), "WARN");
    EXPECT_EQ(logLevelToString(LogLevel::ERROR), "ERROR");
    EXPECT_EQ(logLevelToString(LogLevel::FATAL), "FATAL");
}

TEST(LogLevelConversion, LogLevelFromString) {
    EXPECT_EQ(logLevelFromString("TRACE"), LogLevel::TRACE);
    EXPECT_EQ(logLevelFromString("DEBUG"), LogLevel::DEBUG);
    EXPECT_EQ(logLevelFromString("INFO"), LogLevel::INFO);
    EXPECT_EQ(logLevelFromString("WARN"), LogLevel::WARN);
    EXPECT_EQ(logLevelFromString("WARNING"), LogLevel::WARN);
    EXPECT_EQ(logLevelFromString("ERROR"), LogLevel::ERROR);
    EXPECT_EQ(logLevelFromString("FATAL"), LogLevel::FATAL);
    EXPECT_EQ(logLevelFromString("UNKNOWN"), LogLevel::INFO);  // Default
    EXPECT_EQ(logLevelFromString("invalid"), LogLevel::INFO);  // Default
}

// ============================================================================
// Macro Tests
// ============================================================================

TEST_F(AsyncLoggerTest, LogMacros) {
    LoggerConfig config;
    config.logDirectory = tempDir_.string();
    config.async = false;
    
    initializeGlobalLogger(config);
    
    LOG_INFO("TestMacros", "Testing LOG_INFO macro");
    LOG_WARN("TestMacros", "Testing LOG_WARN macro");
    LOG_ERROR("TestMacros", "Testing LOG_ERROR macro");
    
    nlohmann::json ctx;
    ctx["key"] = "value";
    LOG_INFO_CTX("TestMacros", "Testing context macro", ctx);
    
    shutdownGlobalLogger();
    
    // Cleanup the global logger file
    std::filesystem::path logPath = std::filesystem::path(config.logDirectory) / "nodeagent_";
    for (const auto& entry : std::filesystem::directory_iterator(config.logDirectory)) {
        if (entry.is_regular_file() && entry.path().filename().string().find("nodeagent_") == 0) {
            std::filesystem::remove(entry.path());
        }
    }
}

} // namespace
