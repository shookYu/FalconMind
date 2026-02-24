#include <iostream>
#include <iomanip>
#include <cmath>
#include <vector>

// SDK includes
#include "falconmind/sdk/sensors/GnssSourceNode.h"

using namespace falconmind::sdk::sensors;

struct GnssMeasurement {
    double latitude;
    double longitude;
    double altitude;
    int numSatellites;
    double hdop;
    double cn0[12];
};

class AntiSpoofingDetector {
public:
    bool detectSpoofing(const GnssMeasurement& meas) {
        checks_.clear();
        
        bool spoofed = false;
        
        if (checkSatelliteCount(meas)) {
            checks_.push_back("LOW_SAT_COUNT");
            spoofed = true;
        }
        
        if (checkSignalStrength(meas)) {
            checks_.push_back("ABNORMAL_CN0");
            spoofed = true;
        }
        
        if (checkGeometry(meas)) {
            checks_.push_back("BAD_GEOMETRY");
            spoofed = true;
        }
        
        if (checkConsistency(meas)) {
            checks_.push_back("INCONSISTENT");
            spoofed = true;
        }
        
        return spoofed;
    }
    
    void printChecks() {
        for (const auto& check : checks_) {
            std::cout << "  [ALERT] " << check << std::endl;
        }
    }

private:
    std::vector<std::string> checks_;
    
    bool checkSatelliteCount(const GnssMeasurement& m) {
        return m.numSatellites < 4;
    }
    
    bool checkSignalStrength(const GnssMeasurement& m) {
        int abnormal = 0;
        for (int i = 0; i < m.numSatellites && i < 12; ++i) {
            if (m.cn0[i] > 55.0) abnormal++;
        }
        return abnormal > 3;
    }
    
    bool checkGeometry(const GnssMeasurement& m) {
        return m.hdop > 5.0;
    }
    
    bool checkConsistency(const GnssMeasurement& m) {
        return false;
    }
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Example 17: GNSS Anti-Spoofing Detection" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    AntiSpoofingDetector detector;
    
    GnssMeasurement normal = {39.9042, 116.4074, 50.0, 8, 1.2, {35, 38, 40, 42, 36, 39, 41, 37}};
    GnssMeasurement spoofed = {39.9042, 116.4074, 50.0, 12, 0.8, {65, 67, 68, 66, 64, 69, 70, 68, 65, 67, 66, 64}};
    
    std::cout << "[Test 1] Normal GNSS signal" << std::endl;
    if (!detector.detectSpoofing(normal)) {
        std::cout << "  Result: PASS - No spoofing detected" << std::endl;
    }
    
    std::cout << std::endl << "[Test 2] Potential spoofing attack" << std::endl;
    if (detector.detectSpoofing(spoofed)) {
        std::cout << "  Result: ALERT - Spoofing detected!" << std::endl;
        detector.printChecks();
    }
    
    std::cout << std::endl << "Anti-spoofing demo complete!" << std::endl;
    return 0;
}
