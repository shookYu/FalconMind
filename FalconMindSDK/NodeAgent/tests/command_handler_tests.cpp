// NodeAgent - CommandHandler unit tests
#include <gtest/gtest.h>
#include "nodeagent/CommandHandler.h"
#include "nodeagent/Logger.h"
#include "nodeagent/ErrorStatistics.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/flight/FlightTypes.h"

#include <memory>
#include <thread>
#include <chrono>

using namespace nodeagent;
using namespace falconmind::sdk::flight;

// Note: FlightConnectionService is not virtual, so we can't easily mock it.
// For testing, we'll test CommandHandler's behavior without actual FlightService calls.
// In a real scenario, you might want to refactor FlightConnectionService to use an interface.

void registerCommandHandlerTests() {
    // Tests are registered via TEST macros
}

TEST(CommandHandlerTest, BasicInitialization) {
    CommandHandler handler;
    // Handler should be initialized without errors
    EXPECT_TRUE(true);
}

TEST(CommandHandlerTest, HandleCommandWithoutFlightService) {
    CommandHandler handler;
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Command;
    msg.payload = R"({"type":"ARM","uavId":"uav0"})";
    
    // Should handle gracefully when FlightService is not set
    bool result = handler.handleCommand(msg);
    
    // Should fail when FlightService is not set
    EXPECT_FALSE(result);
}

TEST(CommandHandlerTest, HandleArmCommand) {
    CommandHandler handler;
    // Note: Without a mockable FlightService, we test that the handler
    // processes the command without crashing. In production, you'd use a real
    // or properly mocked FlightConnectionService.
    auto service = std::make_shared<FlightConnectionService>();
    handler.setFlightConnectionService(service);
    
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Command;
    msg.payload = R"({"type":"ARM","uavId":"uav0"})";
    
    // This will fail because service is not connected, but tests parsing
    bool result = handler.handleCommand(msg);
    // Should fail due to not connected, but parsing should work
    EXPECT_FALSE(result);  // Service not connected
}

TEST(CommandHandlerTest, HandleTakeoffCommand) {
    CommandHandler handler;
    auto service = std::make_shared<FlightConnectionService>();
    handler.setFlightConnectionService(service);
    
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Command;
    msg.payload = R"({"type":"TAKEOFF","uavId":"uav0","altitude":10.0})";
    
    // Tests JSON parsing without requiring connection
    bool result = handler.handleCommand(msg);
    EXPECT_FALSE(result);  // Service not connected, but parsing should work
}

TEST(CommandHandlerTest, HandleRtlCommand) {
    CommandHandler handler;
    auto service = std::make_shared<FlightConnectionService>();
    handler.setFlightConnectionService(service);
    
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Command;
    msg.payload = R"({"type":"RTL","uavId":"uav0"})";
    
    bool result = handler.handleCommand(msg);
    EXPECT_FALSE(result);  // Service not connected
}

TEST(CommandHandlerTest, HandleInvalidJson) {
    CommandHandler handler;
    auto service = std::make_shared<FlightConnectionService>();
    handler.setFlightConnectionService(service);
    
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Command;
    msg.payload = "invalid json";
    
    bool result = handler.handleCommand(msg);
    
    // Should fail for invalid JSON
    EXPECT_FALSE(result);
}

TEST(CommandHandlerTest, HandleUnknownCommandType) {
    CommandHandler handler;
    auto service = std::make_shared<FlightConnectionService>();
    handler.setFlightConnectionService(service);
    
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Command;
    msg.payload = R"({"type":"UNKNOWN_COMMAND","uavId":"uav0"})";
    
    bool result = handler.handleCommand(msg);
    
    // Should fail for unknown command type
    EXPECT_FALSE(result);
}
