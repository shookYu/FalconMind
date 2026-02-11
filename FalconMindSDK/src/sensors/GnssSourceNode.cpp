#include "falconmind/sdk/sensors/GnssSourceNode.h"
#include "falconmind/sdk/core/Pad.h"

#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>

namespace falconmind::sdk::sensors {

using namespace falconmind::sdk::core;

GnssSourceNode::GnssSourceNode() : Node("gnss_source") {
    addPad(std::make_shared<Pad>("gnss_out", PadType::Source));
}

bool GnssSourceNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto it = params.find("device");
    if (it != params.end()) deviceOrUri_ = it->second;
    auto itUri = params.find("uri");
    if (itUri != params.end()) deviceOrUri_ = itUri->second;
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

bool GnssSourceNode::start() {
    started_ = true;
    replayMode_ = false;
    nmeaFile_.close();
    if (!deviceOrUri_.empty() && deviceOrUri_ != "sim") {
        nmeaFile_.open(deviceOrUri_);
        if (nmeaFile_.is_open()) {
            replayMode_ = true;
            std::cout << "[GnssSourceNode] start replay from file: " << deviceOrUri_ << std::endl;
        } else {
            std::cout << "[GnssSourceNode] start: open failed, fallback to sim: " << deviceOrUri_ << std::endl;
        }
    } else {
        std::cout << "[GnssSourceNode] start sim mode lat=" << simLat_ << " lon=" << simLon_ << std::endl;
    }
    simTimestampNs_ = 0;
    return true;
}

void GnssSourceNode::process() {
    if (!started_) return;

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
