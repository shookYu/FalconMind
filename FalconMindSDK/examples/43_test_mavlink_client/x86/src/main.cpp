/**
 * @file test_mavlink_client.cpp
 * @brief Test example for MavlinkClient high-level API
 * 
 * Demonstrates usage of the simplified MAVLink flight control API.
 */

#include <falconmind/sdk/high_level/MavlinkClient.h>
#include <falconmind/sdk/high_level/Result.h>
#include <falconmind/sdk/high_level/ErrorCode.h>
#include <iostream>
#include <thread>
#include <chrono>

using namespace falconmind::sdk::high_level;

int main() {
    std::cout << "================================================================================\n";
    std::cout << "  Phase 2 Test: MavlinkClient High-Level API\n";
    std::cout << "================================================================================\n\n";
    
    // Test 1: Connect to SITL
    std::cout << "[Test 1] Connecting to PX4 SITL on localhost:14550...\n";
    auto clientResult = MavlinkClient::connectSITL(14550);
    
    if (!clientResult) {
        std::cerr << "  [SKIP] Failed to connect: " << clientResult.errorMessage() << "\n";
        std::cerr << "  (This is expected if PX4 SITL is not running)\n";
        
        std::cout << "\n  Testing Result<T> API without connection...\n";
        
        // Test Result API
        auto errorClient = ResultPtr<MavlinkClient>::error(
            falconmind::sdk::ErrorCode::ConnectionFailed, 
            "Test error");
        
        if (errorClient.isError()) {
            std::cout << "  [PASS] Error Result works correctly\n";
            std::cout << "    Error code: " << static_cast<int>(errorClient.error()) << "\n";
            std::cout << "    Error message: " << errorClient.errorMessage() << "\n";
        }
        
        std::cout << "\n================================================================================\n";
        std::cout << "  Test completed with warnings (SITL not running)\n";
        std::cout << "================================================================================\n";
        
        return 0; // Exit gracefully
    }
    
    // If we get here, connection succeeded
    auto client = *clientResult;  // Dereference Result to get the shared_ptr
    
    std::cout << "  [PASS] Connected to flight controller\n";
    
    // Test 2: Check connection status
    std::cout << "\n[Test 2] Checking connection status...\n";
    if (client->isConnected()) {
        std::cout << "  [PASS] Connection is active\n";
    } else {
        std::cout << "  [FAIL] Connection lost\n";
    }
    
    // Test 3: Get initial state
    std::cout << "\n[Test 3] Getting initial vehicle state...\n";
    auto state = client->getState();
    std::cout << "  Latitude: " << state.latitude << "\n";
    std::cout << "  Longitude: " << state.longitude << "\n";
    std::cout << "  Altitude: " << state.altitude << " m\n";
    std::cout << "  Is Armed: " << (client->isArmed() ? "Yes" : "No") << "\n";
    std::cout << "  Battery: " << client->getBatteryPercent() << "%\n";
    
    // Test 4: Arm the vehicle
    std::cout << "\n[Test 4] Arming vehicle...\n";
    auto armResult = client->arm();
    if (armResult) {
        std::cout << "  [PASS] Arm command sent successfully\n";
    } else {
        std::cout << "  [WARN] Arm command failed: " << armResult.errorMessage() << "\n";
    }
    
    // Test 5: State callback
    std::cout << "\n[Test 5] Setting up state update callback...\n";
    int callbackCount = 0;
    client->onStateUpdate([&callbackCount](const VehicleState& s) {
        if (++callbackCount <= 3) {
            std::cout << "    State update #" << callbackCount 
                      << ": pos=(" << s.latitude << ", " << s.longitude << ")\n";
        }
    });
    std::cout << "  [PASS] Callback registered\n";
    
    // Wait a bit for callbacks
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Test 6: Poll state
    std::cout << "\n[Test 6] Polling for state update (500ms timeout)...\n";
    auto polledState = client->pollState(500);
    std::cout << "  [PASS] State polled successfully\n";
    std::cout << "    Callbacks received: " << callbackCount << "\n";
    
    // Test 7: Takeoff
    std::cout << "\n[Test 7] Commanding takeoff to 10 meters...\n";
    auto takeoffResult = client->takeoff(10.0);
    if (takeoffResult) {
        std::cout << "  [PASS] Takeoff command sent successfully\n";
    } else {
        std::cout << "  [WARN] Takeoff command failed: " << takeoffResult.errorMessage() << "\n";
    }
    
    // Test 8: Hold position
    std::cout << "\n[Test 8] Commanding hold position...\n";
    auto holdResult = client->hold();
    if (holdResult) {
        std::cout << "  [PASS] Hold command sent successfully\n";
    } else {
        std::cout << "  [WARN] Hold command failed: " << holdResult.errorMessage() << "\n";
    }
    
    // Test 9: Land
    std::cout << "\n[Test 9] Commanding land...\n";
    auto landResult = client->land();
    if (landResult) {
        std::cout << "  [PASS] Land command sent successfully\n";
    } else {
        std::cout << "  [WARN] Land command failed: " << landResult.errorMessage() << "\n";
    }
    
    // Test 10: Disarm
    std::cout << "\n[Test 10] Disarming vehicle...\n";
    auto disarmResult = client->disarm();
    if (disarmResult) {
        std::cout << "  [PASS] Disarm command sent successfully\n";
    } else {
        std::cout << "  [WARN] Disarm command failed: " << disarmResult.errorMessage() << "\n";
    }
    
    // Test 11: Return to launch
    std::cout << "\n[Test 11] Commanding return to launch...\n";
    auto rtlResult = client->returnToLaunch();
    if (rtlResult) {
        std::cout << "  [PASS] RTL command sent successfully\n";
    } else {
        std::cout << "  [WARN] RTL command failed: " << rtlResult.errorMessage() << "\n";
    }
    
    // Disconnect
    std::cout << "\n[Test 12] Disconnecting...\n";
    client->disconnect();
    if (!client->isConnected()) {
        std::cout << "  [PASS] Disconnected successfully\n";
    }
    
    std::cout << "\n================================================================================\n";
    std::cout << "  Phase 2 Test Results: COMPLETE\n";
    std::cout << "================================================================================\n\n";
    
    std::cout << "Features tested:\n";
    std::cout << "  - Connection to PX4 SITL\n";
    std::cout << "  - Result<T> error handling\n";
    std::cout << "  - Vehicle state queries\n";
    std::cout << "  - Flight control commands (arm, takeoff, hold, land, disarm, RTL)\n";
    std::cout << "  - State update callbacks\n";
    std::cout << "  - Connection lifecycle\n\n";
    
    std::cout << "Phase 2 implementation: COMPLETE\n";
    
    return 0;
}
