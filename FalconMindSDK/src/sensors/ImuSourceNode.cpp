#include "falconmind/sdk/sensors/ImuSourceNode.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/flight/FlightTypes.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

namespace falconmind::sdk::sensors {

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::flight;

ImuSourceNode::ImuSourceNode() : Node("imu_source") {
    addPad(std::make_shared<Pad>("imu_out", PadType::Source));
}

bool ImuSourceNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("device");
    if (it != params.end()) deviceOrUri_ = it->second;
    auto itUri = params.find("uri");
    if (itUri != params.end()) deviceOrUri_ = itUri->second;
    
    // Check for MAVLink mode indicators
    if (deviceOrUri_ == "mavlink" || deviceOrUri_ == "px4") {
        mavlinkMode_ = true;
        mavlinkAddress_ = "127.0.0.1";
        mavlinkPort_ = 14540;
    } else if (deviceOrUri_.find(':') != std::string::npos) {
        // Parse IP:port format
        size_t colonPos = deviceOrUri_.find(':');
        if (colonPos != std::string::npos) {
            mavlinkAddress_ = deviceOrUri_.substr(0, colonPos);
            try {
                mavlinkPort_ = std::stoi(deviceOrUri_.substr(colonPos + 1));
                mavlinkMode_ = true;
            } catch (...) {
                mavlinkMode_ = false;
            }
        }
    }
    
    return true;
}

void ImuSourceNode::pushImu(const ImuSample& s) {
    auto outPad = getPad("imu_out");
    if (outPad) {
        static int pushCount = 0;
        if (++pushCount % 10 == 0) {
            std::cout << "[ImuSourceNode] pushImu called, pushing " << sizeof(s) << " bytes" << std::endl;
        }
        outPad->pushToConnections(&s, sizeof(s));
    } else {
        std::cout << "[ImuSourceNode] pushImu: outPad is null!" << std::endl;
    }
}

bool ImuSourceNode::initMavlink(const std::string& address, int port) {
    flightConn_ = std::make_shared<FlightConnectionService>();
    FlightConnectionConfig cfg;
    cfg.remoteAddress = address;
    cfg.remotePort = port;
    cfg.linkType = "UDP";
    cfg.mavlinkVersion = MavlinkVersion::V2;
    
    if (!flightConn_->connect(cfg)) {
        std::cerr << "[ImuSourceNode] Failed to connect to MAVLink at " 
                  << address << ":" << port << std::endl;
        flightConn_.reset();
        return false;
    }
    
    std::cout << "[ImuSourceNode] Connected to MAVLink at " 
              << address << ":" << port << std::endl;
    return true;
}

void ImuSourceNode::closeMavlink() {
    if (flightConn_) {
        flightConn_->disconnect();
        flightConn_.reset();
    }
}

bool ImuSourceNode::pollMavlinkImu(ImuSample& s) {
    if (!flightConn_ || !flightConn_->isConnected()) {
        return false;
    }
    
    // Poll for MAVLink messages and update last state
    bool gotData = false;
    for (int i = 0; i < 10; ++i) {  // Process up to 10 pending messages
        auto stateOpt = flightConn_->pollState();
        if (!stateOpt.has_value()) {
            break;
        }
        gotData = true;
        auto state = stateOpt.value();
        
        s.gx = state.gx;
        s.gy = state.gy;
        s.gz = state.gz;
        s.ax = state.ax;
        s.ay = state.ay;
        s.az = state.az;
        s.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    
    // Use last known state if no new data but we have cached values
    if (!gotData) {
        auto lastState = flightConn_->getLastState();
        // Accept data if we have gyro (from ATTITUDE) or accel (from HIGHRES_IMU)
        if (lastState.gx != 0.0 || lastState.gy != 0.0 || lastState.gz != 0.0 ||
            lastState.ax != 0.0 || lastState.ay != 0.0 || lastState.az != 0.0) {
            s.gx = lastState.gx;
            s.gy = lastState.gy;
            s.gz = lastState.gz;
            s.ax = lastState.ax;
            s.ay = lastState.ay;
            s.az = lastState.az;
            s.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            
            static int debugCount = 0;
            if (++debugCount % 10 == 0) {
                std::cout << "[ImuSourceNode] Using cached state: gx=" << s.gx 
                          << " gy=" << s.gy << " gz=" << s.gz << std::endl;
            }
            return true;
        }
    }
    
    return gotData;
}

bool ImuSourceNode::start() {
    started_ = true;
    replayMode_ = false;
    mavlinkMode_ = false;
    replayFile_.close();
    
    if (!deviceOrUri_.empty() && 
        (deviceOrUri_ == "mavlink" || deviceOrUri_ == "px4" || 
         deviceOrUri_.find(':') != std::string::npos)) {
        
        std::string address = "127.0.0.1";
        int port = 14540;
        
        if (deviceOrUri_ == "mavlink" || deviceOrUri_ == "px4") {
            address = "127.0.0.1";
            port = 14540;
        } else {
            size_t colonPos = deviceOrUri_.find(':');
            if (colonPos != std::string::npos) {
                address = deviceOrUri_.substr(0, colonPos);
                try {
                    port = std::stoi(deviceOrUri_.substr(colonPos + 1));
                } catch (...) {
                    std::cerr << "[ImuSourceNode] Invalid port in URI: " << deviceOrUri_ << std::endl;
                }
            }
        }
        
        if (initMavlink(address, port)) {
            mavlinkMode_ = true;
            std::cout << "[ImuSourceNode] Started MAVLink mode: " << address << ":" << port << std::endl;
        } else {
            std::cerr << "[ImuSourceNode] MAVLink init failed, falling back to sim" << std::endl;
            mavlinkMode_ = false;
        }
    }
    
    if (!mavlinkMode_ && !deviceOrUri_.empty() && deviceOrUri_ != "sim") {
        replayFile_.open(deviceOrUri_);
        if (replayFile_.is_open()) {
            replayMode_ = true;
            std::cout << "[ImuSourceNode] Start replay from file: " << deviceOrUri_ << std::endl;
        } else {
            std::cout << "[ImuSourceNode] Start: open failed, fallback to sim" << std::endl;
        }
    } else if (!mavlinkMode_) {
        std::cout << "[ImuSourceNode] Start sim mode" << std::endl;
    }
    
    simTimestampNs_ = 0;
    return true;
}

void ImuSourceNode::stop() {
    started_ = false;
    if (mavlinkMode_) {
        closeMavlink();
        mavlinkMode_ = false;
    }
    replayFile_.close();
}

void ImuSourceNode::process() {
    if (!started_) return;

    if (mavlinkMode_) {
        ImuSample s;
        if (pollMavlinkImu(s)) {
            pushImu(s);
        } else {
            static int debugCount = 0;
            if (++debugCount % 50 == 0) {
                std::cout << "[ImuSourceNode] pollMavlinkImu returned false" << std::endl;
            }
        }
        return;
    }

    if (replayMode_ && replayFile_.is_open()) {
        std::string line;
        if (std::getline(replayFile_, line) && !line.empty()) {
            std::istringstream ss(line);
            ImuSample s;
            if (ss >> s.timestampNs >> s.gx >> s.gy >> s.gz >> s.ax >> s.ay >> s.az) {
                pushImu(s);
                return;
            }
        }
        replayFile_.clear();
        replayFile_.seekg(0);
        return;
    }

    ImuSample s;
    s.gx = 0.01 * std::sin(simTimestampNs_ * 1e-9);
    s.gy = 0.02 * std::cos(simTimestampNs_ * 1e-9);
    s.gz = 0.0;
    s.ax = 0.0;
    s.ay = 0.0;
    s.az = 9.81;
    s.timestampNs = simTimestampNs_;
    simTimestampNs_ += 10'000'000;
    pushImu(s);
}

} // namespace falconmind::sdk::sensors
