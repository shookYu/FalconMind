#include "falconmind/sdk/sensors/GnssSourceNode.h"
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

GnssSourceNode::GnssSourceNode() : Node("gnss_source") {
    addPad(std::make_shared<Pad>("gnss_out", PadType::Source));
}

bool GnssSourceNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("device");
    if (it != params.end()) deviceOrUri_ = it->second;
    auto itUri = params.find("uri");
    if (itUri != params.end()) deviceOrUri_ = itUri->second;
    
    if (deviceOrUri_ == "mavlink" || deviceOrUri_ == "px4") {
        mavlinkMode_ = true;
        mavlinkAddress_ = "127.0.0.1";
        mavlinkPort_ = 14540;
    } else if (deviceOrUri_.find(':') != std::string::npos) {
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

bool GnssSourceNode::parseNmeaGga(const std::string& line, GnssSample& out) {
    if (line.size() < 7 || line.substr(0, 7) != "$GPGGA,") return false;
    std::istringstream ss(line);
    std::string tok;
    std::getline(ss, tok, ','); // $GPGGA
    if (!std::getline(ss, tok, ',')) return false; // time
    if (!std::getline(ss, tok, ',')) return false; // lat
    double latDeg = 0;
    try { latDeg = std::stod(tok); } catch (...) { return false; }
    if (!std::getline(ss, tok, ',')) return false; // N/S
    if (tok == "S") latDeg = -latDeg;
    int latDegInt = static_cast<int>(latDeg / 100);
    double latMin = latDeg - latDegInt * 100;
    out.latitude = latDegInt + latMin / 60.0;
    if (!std::getline(ss, tok, ',')) return false; // lon
    double lonDeg = 0;
    try { lonDeg = std::stod(tok); } catch (...) { return false; }
    if (!std::getline(ss, tok, ',')) return false; // E/W
    if (tok == "W") lonDeg = -lonDeg;
    int lonDegInt = static_cast<int>(lonDeg / 100);
    double lonMin = lonDeg - lonDegInt * 100;
    out.longitude = lonDegInt + lonMin / 60.0;
    if (!std::getline(ss, tok, ',')) return false; // quality
    if (!std::getline(ss, tok, ',')) return false; // num sats
    try { out.numSatellites = std::stoi(tok); } catch (...) { out.numSatellites = 0; }
    if (!std::getline(ss, tok, ',')) return false; // hdop
    try { out.hdop = std::stof(tok); } catch (...) { out.hdop = 99.f; }
    if (!std::getline(ss, tok, ',')) return false; // alt
    try { out.altitude = std::stod(tok); } catch (...) { out.altitude = 0; }
    out.timestampNs = simTimestampNs_++;
    return true;
}

void GnssSourceNode::pushGnss(const GnssSample& s) {
    auto outPad = getPad("gnss_out");
    if (outPad)
        outPad->pushToConnections(&s, sizeof(s));
}

bool GnssSourceNode::initMavlink(const std::string& address, int port) {
    flightConn_ = std::make_shared<FlightConnectionService>();
    FlightConnectionConfig cfg;
    cfg.remoteAddress = address;
    cfg.remotePort = port;
    cfg.linkType = "UDP";
    cfg.mavlinkVersion = MavlinkVersion::V2;
    
    if (!flightConn_->connect(cfg)) {
        std::cerr << "[GnssSourceNode] Failed to connect to MAVLink at " 
                  << address << ":" << port << std::endl;
        flightConn_.reset();
        return false;
    }
    
    std::cout << "[GnssSourceNode] Connected to MAVLink at " 
              << address << ":" << port << std::endl;
    return true;
}

void GnssSourceNode::closeMavlink() {
    if (flightConn_) {
        flightConn_->disconnect();
        flightConn_.reset();
    }
}

bool GnssSourceNode::pollMavlinkGnss(GnssSample& s) {
    if (!flightConn_ || !flightConn_->isConnected()) {
        return false;
    }
    
    bool gotData = false;
    for (int i = 0; i < 10; ++i) {
        auto stateOpt = flightConn_->pollState();
        if (!stateOpt.has_value()) {
            break;
        }
        gotData = true;
        auto state = stateOpt.value();
        
        s.latitude = state.lat;
        s.longitude = state.lon;
        s.altitude = state.alt;
        s.hdop = static_cast<float>(state.hdop);
        s.numSatellites = state.numSat;
        s.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
    
    if (!gotData) {
        auto lastState = flightConn_->getLastState();
        if (lastState.lat != 0.0 || lastState.lon != 0.0) {
            s.latitude = lastState.lat;
            s.longitude = lastState.lon;
            s.altitude = lastState.alt;
            s.hdop = static_cast<float>(lastState.hdop);
            s.numSatellites = lastState.numSat;
            s.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            return true;
        }
    }
    
    return gotData;
}

bool GnssSourceNode::start() {
    started_ = true;
    replayMode_ = false;
    mavlinkMode_ = false;
    nmeaFile_.close();
    
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
                    std::cerr << "[GnssSourceNode] Invalid port in URI: " << deviceOrUri_ << std::endl;
                }
            }
        }
        
        if (initMavlink(address, port)) {
            mavlinkMode_ = true;
            std::cout << "[GnssSourceNode] Started MAVLink mode: " << address << ":" << port << std::endl;
        } else {
            std::cerr << "[GnssSourceNode] MAVLink init failed, falling back to sim" << std::endl;
            mavlinkMode_ = false;
        }
    }
    
    if (!mavlinkMode_ && !deviceOrUri_.empty() && deviceOrUri_ != "sim") {
        nmeaFile_.open(deviceOrUri_);
        if (nmeaFile_.is_open()) {
            replayMode_ = true;
            std::cout << "[GnssSourceNode] Start replay from file: " << deviceOrUri_ << std::endl;
        } else {
            std::cout << "[GnssSourceNode] Start: open failed, fallback to sim: " << deviceOrUri_ << std::endl;
        }
    } else if (!mavlinkMode_) {
        std::cout << "[GnssSourceNode] Start sim mode lat=" << simLat_ << " lon=" << simLon_ << std::endl;
    }
    
    simTimestampNs_ = 0;
    return true;
}

void GnssSourceNode::stop() {
    started_ = false;
    if (mavlinkMode_) {
        closeMavlink();
        mavlinkMode_ = false;
    }
    nmeaFile_.close();
}

void GnssSourceNode::process() {
    if (!started_) return;

    if (mavlinkMode_) {
        GnssSample s;
        if (pollMavlinkGnss(s)) {
            pushGnss(s);
        }
        return;
    }

    if (replayMode_ && nmeaFile_.is_open()) {
        std::string line;
        while (std::getline(nmeaFile_, line)) {
            if (line.empty()) continue;
            GnssSample s;
            if (parseNmeaGga(line, s)) {
                pushGnss(s);
                return;
            }
        }
        nmeaFile_.clear();
        nmeaFile_.seekg(0);
        return;
    }

    GnssSample s;
    s.latitude = simLat_;
    s.longitude = simLon_;
    s.altitude = simAlt_;
    s.hdop = 0.8f;
    s.numSatellites = 12;
    s.timestampNs = simTimestampNs_++;
    pushGnss(s);
}

} // namespace falconmind::sdk::sensors
