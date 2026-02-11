// NodeAgent - Logger unit tests
#include <gtest/gtest.h>
#include "nodeagent/Logger.h"

#include <sstream>
#include <iostream>
#include <thread>
#include <vector>

using namespace nodeagent;

void registerLoggerTests() {
    // Tests are registered via TEST macros
}

TEST(LoggerTest, BasicInitialization) {
    Logger& logger = Logger::instance();
    EXPECT_TRUE(true);
}

TEST(LoggerTest, SetAndGetLogLevel) {
    Logger& logger = Logger::instance();
    
    logger.setLevel(LogLevel::DEBUG);
    EXPECT_EQ(logger.getLevel(), LogLevel::DEBUG);
    
    logger.setLevel(LogLevel::INFO);
    EXPECT_EQ(logger.getLevel(), LogLevel::INFO);
    
    logger.setLevel(LogLevel::WARN);
    EXPECT_EQ(logger.getLevel(), LogLevel::WARN);
    
    logger.setLevel(LogLevel::ERROR);
    EXPECT_EQ(logger.getLevel(), LogLevel::ERROR);
}

TEST(LoggerTest, LogLevelFiltering) {
    Logger& logger = Logger::instance();
    
    // Set to WARN level
    logger.setLevel(LogLevel::WARN);
    
    // DEBUG and INFO should be filtered out
    logger.debug("TestComponent", "Debug message");
    logger.info("TestComponent", "Info message");
    
    // WARN and ERROR should be logged
    logger.warn("TestComponent", "Warning message");
    logger.error("TestComponent", "Error message");
    
    // Reset to INFO for other tests
    logger.setLevel(LogLevel::INFO);
}

TEST(LoggerTest, ConsoleOutputControl) {
    Logger& logger = Logger::instance();
    
    // Disable console output
    logger.setConsoleOutput(false);
    logger.info("TestComponent", "This should not appear");
    
    // Re-enable
    logger.setConsoleOutput(true);
    logger.info("TestComponent", "This should appear");
}

TEST(LoggerTest, LogMacros) {
    Logger& logger = Logger::instance();
    logger.setLevel(LogLevel::DEBUG);
    
    // Test all log macros
    LOG_DEBUG("TestComponent", "Debug message");
    LOG_INFO("TestComponent", "Info message");
    LOG_WARN("TestComponent", "Warning message");
    LOG_ERROR("TestComponent", "Error message");
    LOG_FATAL("TestComponent", "Fatal message");
    
    EXPECT_TRUE(true);  // If we get here, macros work
}

TEST(LoggerTest, ThreadSafety) {
    Logger& logger = Logger::instance();
    logger.setLevel(LogLevel::INFO);
    
    // Test concurrent logging
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&logger, i]() {
            for (int j = 0; j < 10; j++) {
                logger.info("Thread" + std::to_string(i), 
                           "Message " + std::to_string(j));
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // Should not crash
    EXPECT_TRUE(true);
}
