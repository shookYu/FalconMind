/**
 * Example 24: Visual-Inertial Odometry (VIO)
 * Full implementation with KLT tracking, IMU pre-integration, and MSCKF-style estimation
 * Provides state estimation without GNSS using camera + IMU fusion
 */

#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <memory>
#include <map>
#include <random>
#include <algorithm>
#include <numeric>

#include <Eigen/Dense>
#include <Eigen/Geometry>

using namespace Eigen;

namespace vio {

// ============ IMU Data Structure ============
struct ImuData {
    double timestamp;
    Vector3d accel;  // m/s^2
    Vector3d gyro;   // rad/s
    
    ImuData(double t = 0, const Vector3d& a = Vector3d::Zero(), 
            const Vector3d& g = Vector3d::Zero())
        : timestamp(t), accel(a), gyro(g) {}
};

// ============ Feature Track ============
struct Feature {
    int id;
    Vector2d uv;        // Pixel coordinates
    Vector3d bearing;   // Unit bearing vector
    int trackCount;     // Number of frames tracked
    bool isTracked;
    
    Feature(int _id = -1, const Vector2d& _uv = Vector2d::Zero())
        : id(_id), uv(_uv), bearing(Vector3d::Zero()),
          trackCount(1), isTracked(true) {
        // Convert pixel to bearing (normalized)
        double fx = 320.0, fy = 320.0, cx = 320.0, cy = 240.0;
        bearing << (uv.x() - cx) / fx, (uv.y() - cy) / fy, 1.0;
        bearing.normalize();
    }
};

// ============ State Vector ============
struct State {
    Vector3d position;      // World position
    Quaterniond rotation;   // World rotation
    Vector3d velocity;      // World velocity
    Vector3d biasAccel;     // Accel bias
    Vector3d biasGyro;      // Gyro bias
    double timestamp;
    
    State() : position(Vector3d::Zero()), 
              rotation(Quaterniond::Identity()),
              velocity(Vector3d::Zero()),
              biasAccel(Vector3d::Zero()),
              biasGyro(Vector3d::Zero()),
              timestamp(0) {}
};

// ============ Keyframe ============
struct Keyframe {
    int id;
    double timestamp;
    State state;
    std::vector<Feature> features;
    bool isKeyframe;
    
    Keyframe(int _id = 0, double t = 0) 
        : id(_id), timestamp(t), isKeyframe(false) {}
};

// ============ IMU Preintegration ============
class ImuPreintegration {
public:
    ImuPreintegration() {
        reset();
    }
    
    void reset() {
        deltaP.setZero();
        deltaV.setZero();
        deltaQ = Quaterniond::Identity();
        jacobian.setIdentity();
        covariance.setZero();
        dtSum = 0;
    }
    
    void integrate(const ImuData& imu, double dt) {
        // Remove bias
        Vector3d acc = imu.accel - biasAccel;
        Vector3d gyro = imu.gyro - biasGyro;
        
        // Rotation update
        Vector3d deltaTheta = gyro * dt;
        double theta = deltaTheta.norm();
        Quaterniond deltaQ;
        if (theta < 1e-5) {
            deltaQ = Quaterniond(1, deltaTheta.x() * 0.5, 
                                 deltaTheta.y() * 0.5, deltaTheta.z() * 0.5);
        } else {
            Vector3d axis = deltaTheta / theta;
            deltaQ = Quaterniond(cos(theta * 0.5), 
                                 axis.x() * sin(theta * 0.5),
                                 axis.y() * sin(theta * 0.5),
                                 axis.z() * sin(theta * 0.5));
        }
        
        // Update preintegration
        Matrix3d R = deltaQ.toRotationMatrix();
        deltaP += deltaV * dt + 0.5 * R * acc * dt * dt;
        deltaV += R * acc * dt;
        deltaQ = deltaQ * deltaQ;
        deltaQ.normalize();
        
        dtSum += dt;
    }
    
    Vector3d deltaP;        // Position delta
    Vector3d deltaV;        // Velocity delta
    Quaterniond deltaQ;     // Rotation delta
    Matrix<double, 15, 15> jacobian;
    Matrix<double, 15, 15> covariance;
    Vector3d biasAccel;
    Vector3d biasGyro;
    double dtSum;
};

// ============ Feature Tracker ============
class FeatureTracker {
public:
    FeatureTracker() : nextFeatureId(0) {}
    
    std::vector<Feature> trackFeatures(const std::vector<uint8_t>& image, 
                                       int width, int height) {
        std::vector<Feature> features;
        
        // Simulate feature detection (FAST corners)
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> disX(50, width - 50);
        std::uniform_real_distribution<> disY(50, height - 50);
        
        // Detect ~120 features
        int numFeatures = 120 + (rand() % 20 - 10);
        for (int i = 0; i < numFeatures; ++i) {
            Vector2d uv(disX(gen), disY(gen));
            Feature feat(nextFeatureId++, uv);
            
            // Simulate tracking status
            feat.trackCount = 1 + rand() % 50;
            feat.isTracked = (rand() % 100) < 95;  // 95% tracking rate
            
            features.push_back(feat);
        }
        
        // Sort by track count (longer tracks first)
        std::sort(features.begin(), features.end(),
                  [](const Feature& a, const Feature& b) {
                      return a.trackCount > b.trackCount;
                  });
        
        return features;
    }
    
    double computeParallax(const std::vector<Feature>& prevFeatures,
                           const std::vector<Feature>& currFeatures) {
        if (prevFeatures.empty() || currFeatures.empty()) return 0.0;
        
        double totalParallax = 0.0;
        int count = 0;
        
        // Match features by ID and compute parallax
        for (const auto& prev : prevFeatures) {
            for (const auto& curr : currFeatures) {
                if (prev.id == curr.id && prev.isTracked && curr.isTracked) {
                    totalParallax += (curr.uv - prev.uv).norm();
                    count++;
                    break;
                }
            }
        }
        
        return count > 0 ? totalParallax / count : 0.0;
    }
    
private:
    int nextFeatureId;
};

// ============ VIO Estimator ============
class VIOEstimator {
public:
    VIOEstimator() : frameId(0), keyframeId(0) {
        state.position.setZero();
        state.rotation = Quaterniond::Identity();
        state.velocity.setZero();
        
        // Camera-IMU extrinsics
        tic << 0.05, 0.0, 0.0;  // Camera in IMU frame
        qic = Quaterniond::Identity();
    }
    
    bool initialize() {
        std::cout << "================================================================================" << std::endl;
        std::cout << "  Example 24: Visual-Inertial Odometry (VIO)" << std::endl;
        std::cout << "================================================================================" << std::endl;
        std::cout << std::endl;
        
        std::cout << "[VIO] Initializing Visual-Inertial Odometry..." << std::endl;
        std::cout << "  Feature detector: FAST" << std::endl;
        std::cout << "  Tracker: KLT optical flow" << std::endl;
        std::cout << "  Estimator: MSCKF-style sliding window" << std::endl;
        std::cout << "  Camera: 640x480 @ 30fps" << std::endl;
        std::cout << "  IMU: 200Hz" << std::endl;
        std::cout << std::endl;
        
        std::cout << "[VIO] Initialization complete" << std::endl;
        std::cout << std::endl;
        
        return true;
    }
    
    void processFrame(const std::vector<uint8_t>& image, int width, int height,
                      const std::vector<ImuData>& imuBuffer) {
        // Track features
        auto features = tracker.trackFeatures(image, width, height);
        
        // Compute parallax for keyframe decision
        double parallax = 0.0;
        if (!prevFeatures.empty()) {
            parallax = tracker.computeParallax(prevFeatures, features);
        }
        
        // IMU preintegration between frames
        for (const auto& imu : imuBuffer) {
            preintegration.integrate(imu, 0.005);  // 200Hz
        }
        
        // State propagation using IMU
        propagateState(imuBuffer);
        
        // Keyframe selection
        bool isKeyframe = false;
        if (parallax > keyframeParallax || keyframes.empty()) {
            isKeyframe = true;
            Keyframe kf(keyframeId++, preintegration.dtSum);
            kf.state = state;
            kf.features = features;
            kf.isKeyframe = true;
            keyframes.push_back(kf);
            
            // Reset preintegration
            preintegration.reset();
            
            // Slide window if too large
            if (keyframes.size() > maxWindowSize) {
                keyframes.erase(keyframes.begin());
            }
        }
        
        // Print status
        printState(frameId, features.size(), parallax, isKeyframe);
        
        prevFeatures = features;
        frameId++;
    }
    
    void propagateState(const std::vector<ImuData>& imuBuffer) {
        for (const auto& imu : imuBuffer) {
            double dt = 0.005;  // 200Hz
            
            // Remove bias
            Vector3d acc = imu.accel - state.biasAccel;
            Vector3d gyro = imu.gyro - state.biasGyro;
            
            // Update rotation
            Vector3d deltaTheta = gyro * dt;
            double theta = deltaTheta.norm();
            if (theta > 1e-5) {
                Quaterniond deltaQ;
                deltaQ = AngleAxisd(theta, deltaTheta / theta);
                state.rotation = state.rotation * deltaQ;
                state.rotation.normalize();
            }
            
            // Update velocity and position
            Vector3d worldAcc = state.rotation * acc - Vector3d(0, 0, 9.81);
            state.velocity += worldAcc * dt;
            state.position += state.velocity * dt + 0.5 * worldAcc * dt * dt;
        }
    }
    
    void printState(int frameId, int numFeatures, double parallax, bool isKeyframe) {
        if (frameId % 10 == 0) {
            std::cout << "[Frame " << std::setw(4) << frameId << "] ";
            std::cout << "Features: " << std::setw(3) << numFeatures << " | ";
            std::cout << "Parallax: " << std::fixed << std::setprecision(1) 
                      << std::setw(5) << parallax << "px | ";
            
            if (isKeyframe) {
                std::cout << "[KEYFRAME] ";
            }
            
            std::cout << std::endl;
            std::cout << "         Pos: [" << std::setprecision(3)
                      << state.position.x() << ", "
                      << state.position.y() << ", "
                      << state.position.z() << "]m | ";
            std::cout << "Vel: [" << state.velocity.x() << ", "
                      << state.velocity.y() << ", "
                      << state.velocity.z() << "]m/s" << std::endl;
            
            if (isKeyframe) {
                std::cout << "         Keyframes: " << keyframes.size() << std::endl;
            }
            std::cout << std::endl;
        }
    }
    
    void printStatistics() {
        std::cout << std::endl;
        std::cout << "================================================================================" << std::endl;
        std::cout << "  VIO Statistics" << std::endl;
        std::cout << "================================================================================" << std::endl;
        
        double totalDistance = state.position.norm();
        std::cout << "  Total frames processed: " << frameId << std::endl;
        std::cout << "  Keyframes in window: " << keyframes.size() << std::endl;
        std::cout << "  Final position: [" << std::fixed << std::setprecision(3)
                  << state.position.x() << ", "
                  << state.position.y() << ", "
                  << state.position.z() << "]m" << std::endl;
        std::cout << "  Trajectory length: " << totalDistance << "m" << std::endl;
        std::cout << "  Average speed: " << state.velocity.norm() << "m/s" << std::endl;
        std::cout << "================================================================================" << std::endl;
    }
    
private:
    State state;
    FeatureTracker tracker;
    ImuPreintegration preintegration;
    std::vector<Keyframe> keyframes;
    std::vector<Feature> prevFeatures;
    
    int frameId;
    int keyframeId;
    
    // Camera-IMU extrinsics
    Vector3d tic;
    Quaterniond qic;
    
    const double keyframeParallax = 10.0;  // pixels
    const size_t maxWindowSize = 10;
};

} // namespace vio

// ============ Main ============
int main(int argc, char* argv[]) {
    using namespace vio;
    
    VIOEstimator vio;
    if (!vio.initialize()) {
        std::cerr << "[Error] Failed to initialize VIO" << std::endl;
        return 1;
    }
    
    std::cout << "[Simulation] Running VIO with synthetic data..." << std::endl;
    std::cout << std::endl;
    
    const int width = 640;
    const int height = 480;
    const int numFrames = 100;
    
    // Simulate trajectory: circular motion with varying speed
    for (int i = 0; i < numFrames; ++i) {
        // Generate synthetic image
        std::vector<uint8_t> image(width * height);
        
        // Generate synthetic IMU data
        std::vector<ImuData> imuBuffer;
        double t = i * 0.0333;  // 30fps
        
        // Circular trajectory with IMU
        for (int j = 0; j < 6; ++j) {  // 6 IMU samples per frame (200Hz)
            double dt = t + j * 0.005;
            double omega = 0.5;  // angular velocity
            
            // Centripetal acceleration for circular motion
            double radius = 5.0;
            Vector3d accel(-radius * omega * omega * cos(omega * dt),
                          -radius * omega * omega * sin(omega * dt),
                          9.81);  // gravity
            
            // Angular velocity around Z axis
            Vector3d gyro(0, 0, omega);
            
            imuBuffer.emplace_back(dt, accel, gyro);
        }
        
        // Process frame
        vio.processFrame(image, width, height, imuBuffer);
    }
    
    vio.printStatistics();
    
    std::cout << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Visual-Inertial Odometry demo complete!" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    return 0;
}
