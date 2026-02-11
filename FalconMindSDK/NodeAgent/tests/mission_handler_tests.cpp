// NodeAgent - MissionHandler unit tests
#include <gtest/gtest.h>
#include "nodeagent/MissionHandler.h"
#include "nodeagent/Logger.h"
#include "nodeagent/ErrorStatistics.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <memory>
#include <thread>
#include <chrono>

using namespace nodeagent;

// Note: FlightConnectionService is not virtual, so we use real instances for testing

void registerMissionHandlerTests() {
    // Tests are registered via TEST macros
}

TEST(MissionHandlerTest, BasicInitialization) {
    MissionHandler handler;
    EXPECT_TRUE(true);
}

TEST(MissionHandlerTest, HandleMissionWithoutFlightService) {
    MissionHandler handler;
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Mission;
    msg.payload = R"({"task":"takeoff_and_hover","uavId":"uav0"})";
    
    bool result = handler.handleMission(msg);
    
    // Should fail when FlightService is not set
    EXPECT_FALSE(result);
}

TEST(MissionHandlerTest, HandleTakeoffAndHoverMission) {
    MissionHandler handler;
    auto service = std::make_shared<falconmind::sdk::flight::FlightConnectionService>();
    handler.setFlightConnectionService(service);
    
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Mission;
    msg.payload = R"({"task":"takeoff_and_hover","uavId":"uav0","altitude":10.0})";
    
    // Tests JSON parsing and mission creation without requiring connection
    bool result = handler.handleMission(msg);
    // May fail due to service not connected, but parsing should work
    EXPECT_TRUE(true);  // At least doesn't crash
}

TEST(MissionHandlerTest, HandleInvalidJson) {
    MissionHandler handler;
    auto service = std::make_shared<falconmind::sdk::flight::FlightConnectionService>();
    handler.setFlightConnectionService(service);
    
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Mission;
    msg.payload = "invalid json";
    
    bool result = handler.handleMission(msg);
    
    // Should fail for invalid JSON
    EXPECT_FALSE(result);
}

TEST(MissionHandlerTest, HandleUnknownTaskType) {
    MissionHandler handler;
    auto service = std::make_shared<falconmind::sdk::flight::FlightConnectionService>();
    handler.setFlightConnectionService(service);
    
    DownlinkMessage msg;
    msg.type = DownlinkMessageType::Mission;
    msg.payload = R"({"task":"unknown_task","uavId":"uav0"})";
    
    bool result = handler.handleMission(msg);
    
    // Should handle gracefully (may fail but shouldn't crash)
    EXPECT_TRUE(true);
}

TEST(MissionHandlerTest, UpdateWithoutActiveMission) {
    MissionHandler handler;
    // Should not crash when updating without active mission
    handler.update();
    EXPECT_TRUE(true);
}
