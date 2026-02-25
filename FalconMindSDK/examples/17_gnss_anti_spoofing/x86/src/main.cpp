/**
 * Example 17: GNSS Anti-Spoofing with RAIM
 * Full implementation with Receiver Autonomous Integrity Monitoring,
 * signal quality analysis, and multi-layered spoofing detection
 */

#include <iostream>
#include <iomanip>
#include <cmath>
#include <thread>
#include <vector>
#include <array>
#include <cmath>
#include <thread>
#include <string>
#include <chrono>
#include <random>
#include <algorithm>
#include <map>
#include <sstream>

#include <Eigen/Dense>

#include "falconmind/sdk/sensors/GnssSourceNode.h"

using namespace falconmind::sdk::sensors;

namespace gnss {

// ============ Constants ============
constexpr double SPEED_OF_LIGHT = 299792458.0;      // m/s
constexpr double GPS_L1_FREQ = 1575.42e6;           // Hz
constexpr double EARTH_RADIUS = 6371000.0;          // meters
constexpr int MAX_SATELLITES = 32;
constexpr int MIN_SATS_FOR_RAIM = 5;

// ============ Satellite Structure ============
struct Satellite {
    int prn;                    // Pseudo-random number (satellite ID)
    double pseudorange;         // Measured pseudo-range (meters)
    double carrierPhase;        // Carrier phase measurement
    double doppler;             // Doppler shift (Hz)
    double cn0;                 // Carrier-to-noise density ratio (dB-Hz)
    double elevation;           // Elevation angle (degrees)
    double azimuth;             // Azimuth angle (degrees)
    double posX, posY, posZ;    // Satellite ECEF position (meters)
    bool healthy;               // Satellite health status
    double transmitTime;        // Signal transmission time
};

// ============ GNSS Solution ============
struct GnssSolution {
    double latitude;            // Degrees
    double longitude;           // Degrees
    double altitude;            // Meters
    double clockBias;           // Receiver clock bias (meters)
    double gdop, pdop, hdop, vdop;  // Dilution of precision
    int numSatellites;
    double raimStat;
    bool integrityAvailable;
    std::chrono::steady_clock::time_point timestamp;
};

// ============ Detection Results ============
enum class AlertType {
    NONE,
    LOW_SAT_COUNT,
    HIGH_CN0,
    LOW_CN0,
    BAD_GEOMETRY,
    RAIM_FAULT,
    DOPPLER_INCONSISTENCY,
    PSEUDORANGE_JUMP,
    MULTIPATH,
    SPOOFING_DETECTED
};

struct DetectionAlert {
    AlertType type;
    std::string description;
    double severity;            // 0.0 to 1.0
    int affectedSatellite;
    std::chrono::steady_clock::time_point timestamp;
};

// ============ RAIM Algorithm ============
class RaimMonitor {
public:
    struct RaimResult {
        bool solutionValid;
        double horizontalError;
        double verticalError;
        double protectionLevel;
        int faultedSatellite;
        double residual;
        std::vector<double> residuals;
    };

    RaimResult computeSolution(const std::vector<Satellite>& sats,
                                const GnssSolution& lastPos);
    
    bool checkIntegrity(const RaimResult& result);
    int identifyFaultySatellite(const std::vector<Satellite>& sats,
                                 const std::vector<double>& residuals);

private:
    Eigen::MatrixXd buildGeometryMatrix(const std::vector<Satellite>& sats,
                                         double lat, double lon, double alt);
    Eigen::VectorXd computePredictedRanges(const std::vector<Satellite>& sats,
                                            double lat, double lon, double alt,
                                            double clockBias);
};

Eigen::MatrixXd RaimMonitor::buildGeometryMatrix(const std::vector<Satellite>& sats,
                                                  double lat, double lon, double alt) {
    int n = static_cast<int>(sats.size());
    Eigen::MatrixXd H(n, 4);
    
    // Convert receiver position to ECEF
    double latRad = lat * M_PI / 180.0;
    double lonRad = lon * M_PI / 180.0;
    double sinLat = sin(latRad);
    double cosLat = cos(latRad);
    double sinLon = sin(lonRad);
    double cosLon = cos(lonRad);
    
    // WGS84 ellipsoid
    double a = 6378137.0;  // Semi-major axis
    double e2 = 0.00669437999013;  // Eccentricity squared
    
    double N = a / sqrt(1 - e2 * sinLat * sinLat);
    double recX = (N + alt) * cosLat * cosLon;
    double recY = (N + alt) * cosLat * sinLon;
    double recZ = (N * (1 - e2) + alt) * sinLat;
    
    for (int i = 0; i < n; i++) {
        double dx = sats[i].posX - recX;
        double dy = sats[i].posY - recY;
        double dz = sats[i].posZ - recZ;
        double range = sqrt(dx*dx + dy*dy + dz*dz);
        
        // Unit vector from receiver to satellite
        H(i, 0) = dx / range;
        H(i, 1) = dy / range;
        H(i, 2) = dz / range;
        H(i, 3) = 1.0;  // Clock bias component
    }
    
    return H;
}

Eigen::VectorXd RaimMonitor::computePredictedRanges(const std::vector<Satellite>& sats,
                                                     double lat, double lon, double alt,
                                                     double clockBias) {
    int n = static_cast<int>(sats.size());
    Eigen::VectorXd ranges(n);
    
    // Convert receiver position to ECEF (simplified)
    double latRad = lat * M_PI / 180.0;
    double lonRad = lon * M_PI / 180.0;
    double a = 6378137.0;
    double e2 = 0.00669437999013;
    double sinLat = sin(latRad);
    double cosLat = cos(latRad);
    double sinLon = sin(lonRad);
    double cosLon = cos(lonRad);
    
    double N = a / sqrt(1 - e2 * sinLat * sinLat);
    double recX = (N + alt) * cosLat * cosLon;
    double recY = (N + alt) * cosLat * sinLon;
    double recZ = (N * (1 - e2) + alt) * sinLat;
    
    for (int i = 0; i < n; i++) {
        double dx = sats[i].posX - recX;
        double dy = sats[i].posY - recY;
        double dz = sats[i].posZ - recZ;
        ranges(i) = sqrt(dx*dx + dy*dy + dz*dz) + clockBias;
    }
    
    return ranges;
}

RaimMonitor::RaimResult RaimMonitor::computeSolution(const std::vector<Satellite>& sats,
                                                      const GnssSolution& lastPos) {
    RaimResult result;
    result.solutionValid = false;
    result.faultedSatellite = -1;
    
    int n = static_cast<int>(sats.size());
    if (n < MIN_SATS_FOR_RAIM) {
        return result;
    }
    
    // Build measurement vector
    Eigen::VectorXd pr(n);
    for (int i = 0; i < n; i++) {
        pr(i) = sats[i].pseudorange;
    }
    
    // Initial position guess
    double lat = lastPos.latitude;
    double lon = lastPos.longitude;
    double alt = lastPos.altitude;
    double clockBias = lastPos.clockBias;
    
    // Least squares iteration
    for (int iter = 0; iter < 5; iter++) {
        Eigen::MatrixXd H = buildGeometryMatrix(sats, lat, lon, alt);
        Eigen::VectorXd predicted = computePredictedRanges(sats, lat, lon, alt, clockBias);
        
        Eigen::VectorXd deltaPr = pr - predicted;
        
        // Weighted least squares (simplified - equal weights)
        Eigen::MatrixXd HtH = H.transpose() * H;
        Eigen::VectorXd deltaX = HtH.inverse() * H.transpose() * deltaPr;
        
        // Update position
        double dLat = deltaX(0) / EARTH_RADIUS * 180.0 / M_PI;
        double dLon = deltaX(1) / (EARTH_RADIUS * cos(lat * M_PI / 180.0)) * 180.0 / M_PI;
        
        lat += dLat;
        lon += dLon;
        alt += deltaX(2);
        clockBias += deltaX(3);
        
        // Check convergence
        if (deltaX.head(3).norm() < 0.1) break;
    }
    
    // Compute residuals
    Eigen::MatrixXd H = buildGeometryMatrix(sats, lat, lon, alt);
    Eigen::VectorXd predicted = computePredictedRanges(sats, lat, lon, alt, clockBias);
    Eigen::VectorXd residuals = pr - predicted;
    
    result.residuals.resize(n);
    for (int i = 0; i < n; i++) {
        result.residuals[i] = residuals(i);
    }
    
    result.residual = residuals.squaredNorm();
    result.solutionValid = true;
    result.horizontalError = sqrt(residuals.head(n).squaredNorm() / n);
    
    // Compute protection level (simplified)
    Eigen::MatrixXd cov = (H.transpose() * H).inverse();
    result.protectionLevel = 3.0 * sqrt(cov(0,0) + cov(1,1));  // 3-sigma horizontal
    
    return result;
}

bool RaimMonitor::checkIntegrity(const RaimResult& result) {
    // HPL (Horizontal Protection Level) check
    constexpr double ALERT_LIMIT = 50.0;  // meters
    return result.protectionLevel < ALERT_LIMIT && result.residual < 100.0;
}

int RaimMonitor::identifyFaultySatellite(const std::vector<Satellite>& sats,
                                          const std::vector<double>& residuals) {
    // Find satellite with largest residual
    auto maxIt = std::max_element(residuals.begin(), residuals.end(),
        [](double a, double b) { return fabs(a) < fabs(b); });
    
    int idx = static_cast<int>(std::distance(residuals.begin(), maxIt));
    
    // Check if residual is significant enough
    if (fabs(residuals[idx]) > 100.0) {
        return sats[idx].prn;
    }
    return -1;
}

// ============ Signal Quality Monitor ============
class SignalQualityMonitor {
public:
    struct QualityMetrics {
        double avgCn0;
        double cn0StdDev;
        double cn0Range;
        int highCn0Count;
        int lowCn0Count;
        double elevationWeightedCn0;
        bool isNormal;
    };

    QualityMetrics analyze(const std::vector<Satellite>& sats);
    
    bool detectCn0Anomaly(const QualityMetrics& metrics);
    bool detectMultipath(const std::vector<Satellite>& sats);
    
private:
    constexpr static double CN0_NORMAL_MIN = 30.0;
    constexpr static double CN0_NORMAL_MAX = 50.0;
    constexpr static double CN0_SPOOFING_THRESHOLD = 55.0;
};

SignalQualityMonitor::QualityMetrics SignalQualityMonitor::analyze(
    const std::vector<Satellite>& sats) {
    
    QualityMetrics metrics = {};
    
    if (sats.empty()) return metrics;
    
    // Calculate statistics
    double sum = 0.0;
    double minCn0 = 100.0;
    double maxCn0 = 0.0;
    metrics.highCn0Count = 0;
    metrics.lowCn0Count = 0;
    
    for (const auto& sat : sats) {
        sum += sat.cn0;
        minCn0 = std::min(minCn0, sat.cn0);
        maxCn0 = std::max(maxCn0, sat.cn0);
        
        if (sat.cn0 > CN0_SPOOFING_THRESHOLD) metrics.highCn0Count++;
        if (sat.cn0 < CN0_NORMAL_MIN) metrics.lowCn0Count++;
    }
    
    metrics.avgCn0 = sum / sats.size();
    metrics.cn0Range = maxCn0 - minCn0;
    
    // Calculate standard deviation
    double variance = 0.0;
    for (const auto& sat : sats) {
        variance += pow(sat.cn0 - metrics.avgCn0, 2);
    }
    metrics.cn0StdDev = sqrt(variance / sats.size());
    
    // Elevation weighted CN0
    double weightSum = 0.0;
    for (const auto& sat : sats) {
        double weight = sin(sat.elevation * M_PI / 180.0);
        metrics.elevationWeightedCn0 += sat.cn0 * weight;
        weightSum += weight;
    }
    if (weightSum > 0) {
        metrics.elevationWeightedCn0 /= weightSum;
    }
    
    // Determine if signal quality is normal
    metrics.isNormal = (metrics.avgCn0 >= CN0_NORMAL_MIN &&
                       metrics.avgCn0 <= CN0_NORMAL_MAX &&
                       metrics.highCn0Count == 0);
    
    return metrics;
}

bool SignalQualityMonitor::detectCn0Anomaly(const QualityMetrics& metrics) {
    // Check for spoofing indicators in CN0
    if (metrics.highCn0Count > 0) return true;
    if (metrics.avgCn0 > CN0_NORMAL_MAX + 10.0) return true;
    if (metrics.cn0Range < 3.0 && metrics.avgCn0 > 45.0) return true;  // Suspiciously uniform
    return false;
}

bool SignalQualityMonitor::detectMultipath(const std::vector<Satellite>& sats) {
    // Check for multipath: high CN0 but large pseudorange residuals
    int multipathCount = 0;
    
    for (const auto& sat : sats) {
        if (sat.elevation < 15.0 && sat.cn0 > 45.0) {
            // Low elevation but very strong signal - possible multipath
            multipathCount++;
        }
    }
    
    return multipathCount >= 2;
}

// ============ Anti-Spoofing Detector ============
class AntiSpoofingDetector {
public:
    AntiSpoofingDetector();
    
    std::vector<DetectionAlert> analyze(const std::vector<Satellite>& sats,
                                          const GnssSolution& lastSolution);
    
    void setCn0Thresholds(double min, double max) {
        cn0Min_ = min;
        cn0Max_ = max;
    }
    
    void setRaimThreshold(double threshold) {
        raimThreshold_ = threshold;
    }
    
    bool isSpoofingDetected() const { return spoofingDetected_; }
    int getConfidence() const { return confidence_; }  // 0-100%
    
private:
    bool checkSatelliteCount(const std::vector<Satellite>& sats);
    bool checkSignalStrength(const std::vector<Satellite>& sats);
    bool checkGeometry(const std::vector<Satellite>& sats);
    bool checkConsistency(const std::vector<Satellite>& sats, const GnssSolution& last);
    bool checkDopplerConsistency(const std::vector<Satellite>& sats);
    
    RaimMonitor raim_;
    SignalQualityMonitor signalMonitor_;
    
    double cn0Min_;
    double cn0Max_;
    double raimThreshold_;
    bool spoofingDetected_;
    int confidence_;
    
    GnssSolution lastValidSolution_;
    std::chrono::steady_clock::time_point lastValidTime_;
};

AntiSpoofingDetector::AntiSpoofingDetector()
    : cn0Min_(30.0), cn0Max_(50.0), raimThreshold_(50.0),
      spoofingDetected_(false), confidence_(0) {}

std::vector<DetectionAlert> AntiSpoofingDetector::analyze(
    const std::vector<Satellite>& sats,
    const GnssSolution& lastSolution) {
    
    std::vector<DetectionAlert> alerts;
    spoofingDetected_ = false;
    confidence_ = 0;
    
    // Check 1: Satellite count
    if (checkSatelliteCount(sats)) {
        DetectionAlert alert;
        alert.type = AlertType::LOW_SAT_COUNT;
        alert.description = "Insufficient satellites for reliable positioning";
        alert.severity = 0.7;
        alert.affectedSatellite = -1;
        alert.timestamp = std::chrono::steady_clock::now();
        alerts.push_back(alert);
        confidence_ += 15;
    }
    
    // Check 2: Signal strength (CN0)
    if (checkSignalStrength(sats)) {
        DetectionAlert alert;
        alert.type = AlertType::HIGH_CN0;
        alert.description = "Abnormally high signal strength detected (possible meaconing)";
        alert.severity = 0.9;
        alert.affectedSatellite = -1;
        alert.timestamp = std::chrono::steady_clock::now();
        alerts.push_back(alert);
        confidence_ += 30;
        spoofingDetected_ = true;
    }
    
    // Check 3: Geometry (DOP)
    if (checkGeometry(sats)) {
        DetectionAlert alert;
        alert.type = AlertType::BAD_GEOMETRY;
        alert.description = "Poor satellite geometry (high DOP)";
        alert.severity = 0.5;
        alert.affectedSatellite = -1;
        alert.timestamp = std::chrono::steady_clock::now();
        alerts.push_back(alert);
        confidence_ += 10;
    }
    
    // Check 4: RAIM
    if (sats.size() >= MIN_SATS_FOR_RAIM) {
        RaimMonitor::RaimResult raimResult = raim_.computeSolution(sats, lastSolution);
        
        if (!raim_.checkIntegrity(raimResult)) {
            DetectionAlert alert;
            alert.type = AlertType::RAIM_FAULT;
            alert.description = "RAIM integrity check failed";
            alert.severity = 0.8;
            alert.affectedSatellite = raimResult.faultedSatellite;
            alert.timestamp = std::chrono::steady_clock::now();
            alerts.push_back(alert);
            confidence_ += 25;
            spoofingDetected_ = true;
        }
    }
    
    // Check 5: Position consistency
    if (checkConsistency(sats, lastSolution)) {
        DetectionAlert alert;
        alert.type = AlertType::PSEUDORANGE_JUMP;
        alert.description = "Sudden position jump detected";
        alert.severity = 0.85;
        alert.affectedSatellite = -1;
        alert.timestamp = std::chrono::steady_clock::now();
        alerts.push_back(alert);
        confidence_ += 20;
        spoofingDetected_ = true;
    }
    
    // Check 6: Signal quality analysis
    auto quality = signalMonitor_.analyze(sats);
    if (signalMonitor_.detectCn0Anomaly(quality)) {
        DetectionAlert alert;
        alert.type = AlertType::SPOOFING_DETECTED;
        alert.description = "CN0 anomaly pattern detected";
        alert.severity = 0.95;
        alert.affectedSatellite = -1;
        alert.timestamp = std::chrono::steady_clock::now();
        alerts.push_back(alert);
        confidence_ += 35;
        spoofingDetected_ = true;
    }
    
    // Cap confidence at 100
    confidence_ = std::min(confidence_, 100);
    
    return alerts;
}

bool AntiSpoofingDetector::checkSatelliteCount(const std::vector<Satellite>& sats) {
    return sats.size() < 4;
}

bool AntiSpoofingDetector::checkSignalStrength(const std::vector<Satellite>& sats) {
    int abnormalCount = 0;
    
    for (const auto& sat : sats) {
        if (sat.cn0 > cn0Max_ + 10.0 || sat.cn0 > 55.0) {
            abnormalCount++;
        }
    }
    
    return abnormalCount >= 2;  // Multiple satellites with abnormal signal
}

bool AntiSpoofingDetector::checkGeometry(const std::vector<Satellite>& sats) {
    if (sats.size() < 4) return true;
    
    // Calculate approximate HDOP based on satellite distribution
    // Simplified: check elevation spread
    double sumSin = 0.0;
    double sumCos = 0.0;
    
    for (const auto& sat : sats) {
        double eleRad = sat.elevation * M_PI / 180.0;
        sumSin += sin(eleRad);
        sumCos += cos(eleRad);
    }
    
    double avgElevation = atan2(sumSin, sumCos) * 180.0 / M_PI;
    
    // If all satellites at similar elevation, geometry is poor
    return avgElevation > 70.0 || avgElevation < 20.0;
}

bool AntiSpoofingDetector::checkConsistency(const std::vector<Satellite>& sats,
                                             const GnssSolution& last) {
    // Check for sudden jumps in position
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - lastValidTime_).count();
    
    if (elapsed > 5) {  // 5 second window
        // Calculate distance from last known position
        double dLat = last.latitude - lastValidSolution_.latitude;
        double dLon = last.longitude - lastValidSolution_.longitude;
        
        // Convert to meters (approximate)
        double dy = dLat * 111000.0;  // meters per degree latitude
        double dx = dLon * 111000.0 * cos(last.latitude * M_PI / 180.0);
        double distance = sqrt(dx*dx + dy*dy);
        
        // If moved more than 100m in 5 seconds, suspicious
        if (distance > 100.0 && elapsed < 10) {
            return true;
        }
    }
    
    // Update last valid position
    if (!spoofingDetected_) {
        lastValidSolution_ = last;
        lastValidTime_ = now;
    }
    
    return false;
}

// ============ GNSS Simulator ============
class GnssSimulator {
public:
    GnssSimulator();
    
    std::vector<Satellite> generateNormalMeasurements();
    std::vector<Satellite> generateSpoofedMeasurements();
    std::vector<Satellite> generateJammingScenario();
    std::vector<Satellite> generateMultipathScenario();
    
    void setScenario(const std::string& scenario) { currentScenario_ = scenario; }
    
private:
    std::mt19937 rng_;
    std::string currentScenario_;
    int frameCount_;
    
    Satellite generateSatellite(int prn, bool healthy = true);
};

GnssSimulator::GnssSimulator() : rng_(std::random_device{}()), frameCount_(0) {}

Satellite GnssSimulator::generateSatellite(int prn, bool healthy) {
    Satellite sat;
    sat.prn = prn;
    sat.healthy = healthy;
    
    // Generate random satellite position (simplified)
    double angle = (prn * 11.25) * M_PI / 180.0;
    sat.azimuth = prn * 11.25;
    sat.elevation = 20.0 + (rng_() % 60);  // 20-80 degrees
    
    // CN0 based on elevation
    double baseCn0 = 35.0 + (sat.elevation - 20.0) * 0.3;
    std::normal_distribution<double> cn0Dist(baseCn0, 2.0);
    sat.cn0 = cn0Dist(rng_);
    
    // Pseudorange
    sat.pseudorange = 20000000.0 + (rng_() % 1000000);
    
    // Doppler
    std::normal_distribution<double> dopplerDist(0.0, 500.0);
    sat.doppler = dopplerDist(rng_);
    
    // Satellite ECEF position (circular orbit approximation)
    double orbitRadius = 26560000.0;  // GPS orbit radius
    double eleRad = sat.elevation * M_PI / 180.0;
    double aziRad = sat.azimuth * M_PI / 180.0;
    
    sat.posX = orbitRadius * cos(eleRad) * cos(aziRad);
    sat.posY = orbitRadius * cos(eleRad) * sin(aziRad);
    sat.posZ = orbitRadius * sin(eleRad);
    
    return sat;
}

std::vector<Satellite> GnssSimulator::generateNormalMeasurements() {
    std::vector<Satellite> sats;
    int numSats = 8 + (rng_() % 5);  // 8-12 satellites
    
    for (int i = 0; i < numSats; i++) {
        sats.push_back(generateSatellite(i + 1));
    }
    
    frameCount_++;
    return sats;
}

std::vector<Satellite> GnssSimulator::generateSpoofedMeasurements() {
    std::vector<Satellite> sats;
    
    // Simulate spoofing: all satellites have abnormally high CN0
    int numSats = 10 + (rng_() % 4);
    
    for (int i = 0; i < numSats; i++) {
        Satellite sat = generateSatellite(i + 1);
        // Spoofed signals are stronger and more uniform
        sat.cn0 = 60.0 + (rng_() % 10);  // 60-70 dB-Hz (abnormally high)
        sats.push_back(sat);
    }
    
    frameCount_++;
    return sats;
}

std::vector<Satellite> GnssSimulator::generateJammingScenario() {
    std::vector<Satellite> sats;
    int numSats = 4 + (rng_() % 3);  // Fewer satellites due to jamming
    
    for (int i = 0; i < numSats; i++) {
        Satellite sat = generateSatellite(i + 1);
        // Jammed signals have low CN0
        sat.cn0 = 20.0 + (rng_() % 15);  // 20-35 dB-Hz (weak)
        sats.push_back(sat);
    }
    
    return sats;
}

std::vector<Satellite> GnssSimulator::generateMultipathScenario() {
    std::vector<Satellite> sats = generateNormalMeasurements();
    
    // Add multipath effects to low elevation satellites
    for (auto& sat : sats) {
        if (sat.elevation < 20.0) {
            // Multipath: high CN0 but erratic pseudorange
            sat.cn0 = 48.0 + (rng_() % 8);
            sat.pseudorange += (rng_() % 100) - 50;  // ±50m error
        }
    }
    
    return sats;
}

} // namespace gnss

// ============ Main ============
int main(int argc, char* argv[]) {
    using namespace gnss;
    
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Example 17: GNSS Anti-Spoofing with RAIM" << std::endl;
    std::cout << "  Full Implementation: Multi-layered spoofing detection" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    // Parse arguments
    int numFrames = 100;
    std::string scenario = "mixed";
    
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            numFrames = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--scenario") == 0 && i + 1 < argc) {
            scenario = argv[++i];
        }
    }
    
    // Initialize components
    AntiSpoofingDetector detector;
    GnssSimulator simulator;
    GnssSolution lastSolution;
    lastSolution.latitude = 39.9042;
    lastSolution.longitude = 116.4074;
    lastSolution.altitude = 50.0;
    lastSolution.clockBias = 0.0;
    lastSolution.timestamp = std::chrono::steady_clock::now();
    
    std::cout << "[Configuration]" << std::endl;
    std::cout << "  Frames to process: " << numFrames << std::endl;
    std::cout << "  Scenario: " << scenario << std::endl;
    std::cout << "  CN0 thresholds: 30-50 dB-Hz" << std::endl;
    std::cout << "  RAIM enabled: Yes" << std::endl;
    std::cout << std::endl;
    
    std::cout << "[Processing GNSS frames...]" << std::endl;
    std::cout << std::endl;
    
    int normalFrames = 0;
    int alertFrames = 0;
    int spoofingFrames = 0;
    
    for (int frame = 0; frame < numFrames; frame++) {
        std::vector<Satellite> sats;
        
        // Determine scenario for this frame
        if (scenario == "normal") {
            sats = simulator.generateNormalMeasurements();
        } else if (scenario == "spoofing") {
            sats = simulator.generateSpoofedMeasurements();
        } else if (scenario == "jamming") {
            sats = simulator.generateJammingScenario();
        } else if (scenario == "multipath") {
            sats = simulator.generateMultipathScenario();
        } else {  // mixed
            if (frame < 30) {
                sats = simulator.generateNormalMeasurements();
            } else if (frame < 50) {
                sats = simulator.generateSpoofedMeasurements();
            } else if (frame < 70) {
                sats = simulator.generateNormalMeasurements();
            } else {
                sats = simulator.generateJammingScenario();
            }
        }
        
        // Analyze measurements
        auto alerts = detector.analyze(sats, lastSolution);
        
        // Calculate average CN0
        double avgCn0 = 0.0;
        for (const auto& sat : sats) avgCn0 += sat.cn0;
        avgCn0 /= sats.size();
        
        // Print frame status
        std::cout << "[Frame " << std::setw(3) << (frame + 1) << "] ";
        std::cout << "Sats: " << std::setw(2) << sats.size() << " | ";
        std::cout << "CN0: " << std::fixed << std::setprecision(1) << avgCn0 << " dB-Hz | ";
        
        if (alerts.empty()) {
        std::cout << "Status: ✓ HEALTHY";
            normalFrames++;
        } else {
            std::cout << "Status: ⚠ ALERTS(" << alerts.size() << ")";
            alertFrames++;
            
            if (detector.isSpoofingDetected()) {
                spoofingFrames++;
            }
        }
        std::cout << std::endl;
        
        // Print detailed alerts periodically
        if (!alerts.empty() && (frame % 10 == 0 || detector.isSpoofingDetected())) {
            for (const auto& alert : alerts) {
                std::cout << "           → " << alert.description << std::endl;
            }
        }
        
        // Simulate delay between frames
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    std::cout << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Results Summary" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "Total frames processed: " << numFrames << std::endl;
    std::cout << "Normal frames: " << normalFrames << " (" << (100.0 * normalFrames / numFrames) << "%)" << std::endl;
    std::cout << "Alert frames: " << alertFrames << " (" << (100.0 * alertFrames / numFrames) << "%)" << std::endl;
    std::cout << "Spoofing detected: " << spoofingFrames << " (" << (100.0 * spoofingFrames / numFrames) << "%)" << std::endl;
    std::cout << std::endl;
    
    std::cout << "Detection Capabilities:" << std::endl;
    std::cout << "  ✓ Satellite count monitoring" << std::endl;
    std::cout << "  ✓ CN0 anomaly detection" << std::endl;
    std::cout << "  ✓ RAIM (Receiver Autonomous Integrity Monitoring)" << std::endl;
    std::cout << "  ✓ Signal geometry analysis" << std::endl;
    std::cout << "  ✓ Position consistency check" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    return 0;
}
