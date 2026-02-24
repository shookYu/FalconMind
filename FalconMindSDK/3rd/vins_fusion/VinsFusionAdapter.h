// FalconMindSDK - VINS-Fusion Integration Adapter
// HKUST VINS-Fusion: https://github.com/HKUST-Aerial-Robotics/VINS-Fusion
//
// This adapter provides a C++ interface to integrate VINS-Fusion into FalconMindSDK
// VINS-Fusion supports: Stereo/Mono camera + IMU fusion
//
// Prerequisites:
//   1. Install VINS-Fusion: https://github.com/HKUST-Aerial-Robotics/VINS-Fusion
//   2. Install dependencies: OpenCV, Ceres Solver, Eigen3
//   3. Build VINS-Fusion and link against this adapter
//
// Build Instructions:
//   cd FalconMindSDK/3rd/vins_fusion
//   mkdir build && cd build
//   cmake ..
//   make -j4
//
// VINS-Fusion 核心功能：
//   - 视觉惯性里程计 (VIO)
//   - 多相机支持 (单目/双目)
//   - 回环检测
//   - 全局位姿图优化

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>

#include "falconmind/sdk/sensors/SensorTypes.h"

// VINS-Fusion forward declarations (avoid including full headers)
// Users need to link against actual VINS-Fusion library
namespace vins {
    class Estimator;
    class FeatureTracker;
    struct IMUData;
    struct ImageData;
}

namespace falconmind::sdk::perception {

// Camera configuration for VINS-Fusion
struct VinsCameraConfig {
    int cameraCount{1};              // 1=mono, 2=stereo
    int imageWidth{640};
    int imageHeight{480};
    double focalLength{460.0};       // pixels
    double cx{320.0};                // principal point x
    double cy{240.0};                // principal point y
    std::vector<double> distortion{0, 0, 0, 0}; // k1, k2, p1, p2
    
    // Stereo baseline (meters), only used if cameraCount==2
    double baseline{0.1};
    
    // Camera-IMU extrinsic (rotation matrix 3x3 + translation 3x1)
    std::vector<double> ric{1,0,0, 0,1,0, 0,0,1}; // Rotation IMU->Camera
    std::vector<double> tic{0, 0, 0};             // Translation IMU->Camera
};

// IMU configuration
struct VinsImuConfig {
    double accNoise{0.1};            // accelerometer measurement noise
    double gyroNoise{0.01};          // gyroscope measurement noise
    double accBiasNoise{0.001};      // accelerometer bias random work noise
    double gyroBiasNoise{0.0001};    // gyroscope bias random work noise
    double gravity{9.81};            // gravity magnitude
    
    // IMU-Camera time offset
    double td{0.0};                  // seconds
};

// Feature tracking configuration
struct VinsFeatureConfig {
    int maxFeatureCount{150};        // max feature number per image
    int minFeatureDistance{30};      // min feature distance (pixels)
    int featureType{0};              // 0=KLT, 1=ORB
    bool showTrack{false};           // publish tracking image
};

// Optimization configuration
struct VinsOptimizationConfig {
    bool enableLoopClosure{true};    // enable loop closure
    bool enableGlobalOptimization{true}; // enable global pose graph
    int windowSize{10};              // sliding window size
    int keyframeParallax{10};        // keyframe selection threshold (pixels)
};

// Complete VINS-Fusion configuration
struct VinsFusionConfig {
    VinsCameraConfig camera;
    VinsImuConfig imu;
    VinsFeatureConfig feature;
    VinsOptimizationConfig optimization;
    
    std::string configFile;          // Path to VINS-Fusion config file (YAML)
    std::string outputPath{"./vins_output"}; // Output directory for trajectory
};

// SLAM output: pose + velocity + bias
struct VinsOutput {
    double timestamp{0.0};           // seconds
    
    // Position (world frame, meters)
    double pw_x{0.0}, pw_y{0.0}, pw_z{0.0};
    
    // Orientation (world frame, quaternion w,x,y,z)
    double qw{1.0}, qx{0.0}, qy{0.0}, qz{0.0};
    
    // Velocity (world frame, m/s)
    double vw_x{0.0}, vw_y{0.0}, vw_z{0.0};
    
    // IMU bias
    double bias_acc_x{0.0}, bias_acc_y{0.0}, bias_acc_z{0.0};
    double bias_gyro_x{0.0}, bias_gyro_y{0.0}, bias_gyro_z{0.0};
    
    // Tracking status
    bool isTracking{false};
    int trackedFeatures{0};
};

// VINS-Fusion Integration Adapter
class VinsFusionAdapter {
public:
    explicit VinsFusionAdapter(const VinsFusionConfig& config);
    ~VinsFusionAdapter();
    
    // Initialize VINS-Fusion (load config, initialize estimator)
    bool initialize();
    
    // Shutdown
    void shutdown();
    
    // Input: Image data
    // timestamp: seconds
    // imageData: raw image buffer (GRAY8 or RGB)
    // stereoImageData: second camera image (for stereo)
    void inputImage(double timestamp, 
                    const uint8_t* imageData, 
                    const uint8_t* stereoImageData = nullptr);
    
    // Input: IMU data
    void inputImu(const sensors::ImuSample& imu);
    
    // Get latest output (non-blocking)
    bool getLatestOutput(VinsOutput& output);
    
    // Set callback for new output
    using OutputCallback = std::function<void(const VinsOutput&)>;
    void setOutputCallback(OutputCallback callback);
    
    // Save trajectory to file (TUM format)
    bool saveTrajectory(const std::string& filename);
    
    // Get system status
    bool isInitialized() const { return initialized_.load(); }
    bool isRunning() const { return running_.load(); }
    
    // Get statistics
    int getProcessedImageCount() const { return imageCount_.load(); }
    int getProcessedImuCount() const { return imuCount_.load(); }

private:
    void processingThreadFunc();
    void outputThreadFunc();
    
    VinsFusionConfig config_;
    
    // VINS-Fusion core components (forward declared to avoid header dependency)
    std::unique_ptr<vins::Estimator> estimator_;
    std::unique_ptr<vins::FeatureTracker> featureTracker_;
    
    // Threading
    std::thread processingThread_;
    std::thread outputThread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> initialized_{false};
    
    // Input queues
    struct ImageFrame {
        double timestamp;
        std::vector<uint8_t> data;
        std::vector<uint8_t> stereoData;
    };
    std::queue<ImageFrame> imageQueue_;
    std::queue<sensors::ImuSample> imuQueue_;
    std::mutex inputMutex_;
    
    // Output
    VinsOutput latestOutput_;
    std::mutex outputMutex_;
    OutputCallback outputCallback_;
    
    // Statistics
    std::atomic<int> imageCount_{0};
    std::atomic<int> imuCount_{0};
};

} // namespace falconmind::sdk::perception
