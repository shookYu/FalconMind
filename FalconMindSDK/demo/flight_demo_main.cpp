#include "falconmind/sdk/flight/FlightConnectionService.h"

#include <chrono>
#include <thread>
#include <iostream>

using namespace falconmind::sdk::flight;

int main() {
    FlightConnectionService svc;
    FlightConnectionConfig cfg;
    cfg.remoteAddress = "127.0.0.1";
    cfg.remotePort = 14540;

    if (!svc.connect(cfg)) {
        std::cerr << "[flight_demo] Failed to connect FlightConnectionService" << std::endl;
        return 1;
    }

    std::cout << "[flight_demo] Sending ARM" << std::endl;
    FlightCommand cmd{};
    cmd.type = FlightCommandType::Arm;
    svc.sendCommand(cmd);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    std::cout << "[flight_demo] Sending TAKEOFF" << std::endl;
    cmd.type = FlightCommandType::Takeoff;
    cmd.targetAlt = 10.0;
    svc.sendCommand(cmd);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "[flight_demo] Sending RTL" << std::endl;
    cmd.type = FlightCommandType::ReturnToLaunch;
    svc.sendCommand(cmd);

    svc.disconnect();
    std::cout << "[flight_demo] Done." << std::endl;
    return 0;
}

