#include <iostream>
#include <iomanip>
#include <cmath>

#include "falconmind/sdk/sdk.h"

struct ImuData {
    double ax, ay, az;
    double gx, gy, gz;
    double timestamp;
};

struct GnssData {
    double lat, lon, alt;
    double hdop;
    int numSat;
    double timestamp;
};

struct NavState {
    double x, y, z;
    double vx, vy, vz;
    double roll, pitch, yaw;
};

class ESKFFusion {
public:
    bool initialize() {
        std::cout << "[ESKF] Initializing Error-State Kalman Filter..." << std::endl;
        state_ = {0, 0, 0, 0, 0, 0, 0, 0, 0};
        return true;
    }
    
    void predict(const ImuData& imu) {
        double dt = 0.01;
        state_.vx += imu.ax * dt;
        state_.vy += imu.ay * dt;
        state_.vz += (imu.az - 9.81) * dt;
        
        state_.x += state_.vx * dt;
        state_.y += state_.vy * dt;
        state_.z += state_.vz * dt;
        
        state_.roll += imu.gx * dt;
        state_.pitch += imu.gy * dt;
        state_.yaw += imu.gz * dt;
    }
    
    void update(const GnssData& gnss) {
        double gnssX = (gnss.lon - 116.4074) * 111320;
        double gnssY = (gnss.lat - 39.9042) * 110540;
        double gnssZ = gnss.alt;
        
        state_.x = 0.9 * state_.x + 0.1 * gnssX;
        state_.y = 0.9 * state_.y + 0.1 * gnssY;
        state_.z = 0.9 * state_.z + 0.1 * gnssZ;
    }
    
    void printState() {
        std::cout << "  Pos: [" << std::fixed << std::setprecision(2)
                  << std::setw(8) << state_.x << ", "
                  << std::setw(8) << state_.y << ", "
                  << std::setw(8) << state_.z << "] m | ";
        std::cout << "Vel: [" << std::setprecision(2)
                  << std::setw(6) << state_.vx << ", "
                  << std::setw(6) << state_.vy << ", "
                  << std::setw(6) << state_.vz << "] m/s" << std::endl;
    }

private:
    NavState state_;
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Example 23: IMU-GNSS Fusion (ESKF)" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    ESKFFusion fusion;
    if (!fusion.initialize()) {
        std::cerr << "Failed to initialize ESKF" << std::endl;
        return 1;
    }
    
    std::cout << "Running IMU-GNSS fusion loop..." << std::endl << std::endl;
    
    for (int i = 0; i < 100; ++i) {
        ImuData imu = {0.1, 0.05, 9.81, 0.01, 0.02, 0.005, i * 0.01};
        fusion.predict(imu);
        
        if (i % 10 == 0) {
            GnssData gnss = {39.9042 + i * 0.00001, 116.4074, 50.0, 1.2, 8, i * 0.01};
            fusion.update(gnss);
        }
        
        if (i % 20 == 0) {
            std::cout << "Step " << std::setw(3) << i << ": ";
            fusion.printState();
        }
    }
    
    std::cout << std::endl << "IMU-GNSS fusion demo complete!" << std::endl;
    return 0;
}
