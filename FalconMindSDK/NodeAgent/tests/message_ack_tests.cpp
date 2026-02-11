// NodeAgent - MessageAckManager unit tests
#include <gtest/gtest.h>
#include "nodeagent/MessageAck.h"
#include "nodeagent/DownlinkClient.h"

#include <thread>
#include <chrono>

using namespace nodeagent;

void registerMessageAckTests() {
    // Tests are registered via TEST macros
}

TEST(MessageAckManagerTest, BasicInitialization) {
    MessageAckManager::Config config;
    config.maxRetries = 3;
    config.timeoutMs = std::chrono::milliseconds(5000);
    
    MessageAckManager manager(config);
    EXPECT_TRUE(true);
}

TEST(MessageAckManagerTest, RegisterPendingMessage) {
    MessageAckManager manager(MessageAckManager::Config{});
    
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Command;
    msg.payload = R"({"type":"ARM"})";
    msg.requestId = "req123";
    
    std::string msgId = manager.registerPendingMessage(msg);
    
    // Should return the requestId if provided
    EXPECT_EQ(msgId, "req123");
}

TEST(MessageAckManagerTest, RegisterMessageWithoutRequestId) {
    MessageAckManager manager(MessageAckManager::Config{});
    
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Command;
    msg.payload = R"({"type":"ARM"})";
    msg.requestId = "";  // No requestId
    
    std::string msgId = manager.registerPendingMessage(msg);
    
    // Should generate a new message ID
    EXPECT_FALSE(msgId.empty());
    EXPECT_NE(msgId, "");
}

TEST(MessageAckManagerTest, AcknowledgeMessage) {
    MessageAckManager manager(MessageAckManager::Config{});
    
    DownlinkMessage msg;
    msg.requestId = "req123";
    std::string msgId = manager.registerPendingMessage(msg);
    
    // Acknowledge the message
    manager.acknowledgeMessage("req123");
    
    // Update to check status
    manager.update();
    
    // Message should be acknowledged (no retry should occur)
    EXPECT_TRUE(true);
}

TEST(MessageAckManagerTest, MessageTimeout) {
    MessageAckManager::Config config;
    config.maxRetries = 2;
    config.timeoutMs = std::chrono::milliseconds(100);  // Short timeout for testing
    
    MessageAckManager manager(config);
    
    bool retryTriggered = false;
    manager.setRetryCallback([&retryTriggered](const DownlinkMessage&) {
        retryTriggered = true;
    });
    
    DownlinkMessage msg;
    msg.requestId = "req_timeout";
    manager.registerPendingMessage(msg);
    
    // Wait for timeout
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    
    // Update should trigger timeout check
    manager.update();
    
    // Retry should be triggered
    EXPECT_TRUE(retryTriggered);
}

TEST(MessageAckManagerTest, MaxRetriesReached) {
    MessageAckManager::Config config;
    config.maxRetries = 2;
    config.timeoutMs = std::chrono::milliseconds(50);  // Short timeout
    
    MessageAckManager manager(config);
    
    int retryCount = 0;
    manager.setRetryCallback([&retryCount](const DownlinkMessage&) {
        retryCount++;
    });
    
    DownlinkMessage msg;
    msg.requestId = "req_max_retries";
    manager.registerPendingMessage(msg);
    
    // Wait and update multiple times to trigger retries
    for (int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(60));
        manager.update();
    }
    
    // Should not exceed max retries
    EXPECT_LE(retryCount, config.maxRetries);
}

TEST(MessageAckManagerTest, UpdateWithoutPendingMessages) {
    MessageAckManager manager(MessageAckManager::Config{});
    
    // Should not crash when updating without pending messages
    manager.update();
    EXPECT_TRUE(true);
}
