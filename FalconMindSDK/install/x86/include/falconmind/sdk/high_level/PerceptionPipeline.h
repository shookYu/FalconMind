/**
 * @file PerceptionPipeline.h
 * @brief High-level perception pipeline API
 * 
 * Simplified API for common perception tasks:
 * - Object detection
 * - Object tracking
 * - Visual SLAM
 * 
 * Usage:
 *   auto pipeline = PerceptionPipeline::create()
 *       .withCamera({640, 480, 30})
 *       .withDetector("yolov8n.rknn", DetectorBackend::RKNN)
 *       .withTracker(TrackerType::DeepSORT)
 *       .build();
 *   
 *   pipeline->onDetection([](const Detection& d) {
 *       std::cout << "Detected: " << d.className <> " at " << d.bbox << std::endl;
 *   });
 *   
 *   auto result = pipeline->start();
 *   if (!result) {
 *       std::cerr << "Failed to start: " << result.errorMessage() << std::endl;
 *   }
 */

#pragma once

#include "Result.h"
#include "ErrorCode.h"
#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/perception/DetectionTypes.h"
#include "falconmind/sdk/perception/TrackingTypes.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace falconmind {
namespace sdk {
namespace high_level {

// Forward declarations
class PerceptionPipeline;

/**
 * @brief Supported detector backends
 */
enum class DetectorBackend {
    RKNN,           // Rockchip NPU
    ONNX_RUNTIME,   // ONNX Runtime
    TENSORRT,       // NVIDIA TensorRT
    AUTO            // Auto-select best available
};

/**
 * @brief Supported tracker types
 */
enum class TrackerType {
    NONE,           // No tracking
    SIMPLE_IOU,     // Simple IoU-based tracking
    SORT,           // SORT algorithm
    DEEPSORT        // DeepSORT with appearance features
};

/**
 * @brief Camera configuration
 */
struct CameraConfig {
    int width = 640;
    int height = 480;
    int fps = 30;
    std::string device = "/dev/video0";
    bool useHardwareEncoding = false;
    
    CameraConfig() = default;
    CameraConfig(int w, int h, int f) : width(w), height(h), fps(f) {}
};

/**
 * @brief Detector configuration
 */
struct DetectorConfig {
    std::string modelPath;
    DetectorBackend backend = DetectorBackend::AUTO;
    float confidenceThreshold = 0.5f;
    float nmsThreshold = 0.45f;
    std::vector<int> inputSize = {640, 640};
    
    DetectorConfig() = default;
    DetectorConfig(const std::string& path, DetectorBackend be = DetectorBackend::AUTO)
        : modelPath(path), backend(be) {}
};

/**
 * @brief Tracker configuration
 */
struct TrackerConfig {
    TrackerType type = TrackerType::DEEPSORT;
    int maxAge = 30;              // Max frames to keep lost tracks
    int minHits = 3;              // Min hits to confirm track
    float iouThreshold = 0.3f;    // IoU threshold for matching
    
    TrackerConfig() = default;
    explicit TrackerConfig(TrackerType t) : type(t) {}
};

/**
 * @brief Simplified detection result for callbacks
 */
struct Detection {
    int trackId = -1;
    int classId;
    std::string className;
    float confidence;
    struct { float x, y, width, height; } bbox;  // Normalized 0-1
    struct { float x, y; } center;                // Normalized center point
    float area;
    
    bool isTracked() const { return trackId >= 0; }
};

/**
 * @brief Perception pipeline builder
 */
class PerceptionPipelineBuilder {
public:
    PerceptionPipelineBuilder() = default;
    
    /**
     * @brief Configure camera input
     */
    PerceptionPipelineBuilder& withCamera(const CameraConfig& config);
    PerceptionPipelineBuilder& withCamera(int width, int height, int fps);
    PerceptionPipelineBuilder& withCameraDevice(const std::string& device);
    
    /**
     * @brief Configure object detector
     */
    PerceptionPipelineBuilder& withDetector(const DetectorConfig& config);
    PerceptionPipelineBuilder& withDetector(const std::string& modelPath, 
                                              DetectorBackend backend = DetectorBackend::AUTO);
    
    /**
     * @brief Configure object tracker
     */
    PerceptionPipelineBuilder& withTracker(const TrackerConfig& config);
    PerceptionPipelineBuilder& withTracker(TrackerType type);
    
    /**
     * @brief Enable/disable low-light enhancement
     */
    PerceptionPipelineBuilder& withLowLightEnhancement(bool enable = true);
    
    /**
     * @brief Build the pipeline
     */
    ResultPtr<PerceptionPipeline> build();
    
private:
    CameraConfig cameraConfig_;
    DetectorConfig detectorConfig_;
    TrackerConfig trackerConfig_;
    bool lowLightEnhancement_ = false;
    bool hasCamera_ = false;
    bool hasDetector_ = false;
};

/**
 * @brief High-level perception pipeline
 * 
 * Encapsulates a complete perception pipeline:
 * Camera -> [Enhancement] -> Detector -> Tracker -> Output
 */
class PerceptionPipeline {
public:
    ~PerceptionPipeline();
    
    /**
     * @brief Create a builder for fluent API
     */
    static PerceptionPipelineBuilder create();
    
    /**
     * @brief Start the pipeline
     */
    Result<void> start();
    
    /**
     * @brief Stop the pipeline
     */
    Result<void> stop();
    
    /**
     * @brief Check if pipeline is running
     */
    bool isRunning() const;
    
    /**
     * @brief Set callback for detection events
     */
    void onDetection(std::function<void(const std::vector<Detection>&)> callback);
    void onDetection(std::function<void(const Detection&)> callback);  // Per-object
    
    /**
     * @brief Set callback for specific object class
     */
    void onObjectDetected(const std::string& className, 
                         std::function<void(const Detection&)> callback);
    
    /**
     * @brief Get latest detections (non-blocking)
     */
    std::vector<Detection> getLatestDetections() const;
    
    /**
     * @brief Get specific object by class name
     */
    std::optional<Detection> getObject(const std::string& className) const;
    
    /**
     * @brief Get tracked object by ID
     */
    std::optional<Detection> getTrackedObject(int trackId) const;
    
    /**
     * @brief Get detection count
     */
    size_t getDetectionCount() const;
    
    /**
     * @brief Get tracking count (only tracked objects)
     */
    size_t getTrackingCount() const;
    
    /**
     * @brief Get pipeline statistics
     */
    struct Statistics {
        uint64_t framesProcessed = 0;
        uint64_t detectionsTotal = 0;
        float averageFps = 0.0f;
        float averageInferenceTimeMs = 0.0f;
        uint64_t trackingSwitches = 0;  // ID switches
    };
    Statistics getStatistics() const;
    
    /**
     * @brief Wait for pipeline to finish (blocking)
     */
    void wait();
    
    /**
     * @brief Enable/disable specific object class
     */
    void setClassEnabled(const std::string& className, bool enabled);
    
    /**
     * @brief Set confidence threshold at runtime
     */
    void setConfidenceThreshold(float threshold);

private:
    // Implementation class defined here for std::make_unique
    class Impl {
    public:
        CameraConfig cameraConfig_;
        DetectorConfig detectorConfig_;
        TrackerConfig trackerConfig_;
        bool lowLightEnhancement_ = false;
        bool isRunning_ = false;
        
        // Callbacks
        std::function<void(const std::vector<Detection>&)> onDetectionBatch_;
        std::function<void(const Detection&)> onDetectionSingle_;
        std::function<void(const Detection&)> onObjectDetected_;
        std::string targetClassName_;
        
        // Statistics
        Statistics stats_;
    };
    
    PerceptionPipeline() = default;
    friend class PerceptionPipelineBuilder;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Convenience function to create a standard perception pipeline
 */
inline ResultPtr<PerceptionPipeline> createPerceptionPipeline(
    const CameraConfig& camera,
    const DetectorConfig& detector,
    const TrackerConfig& tracker = TrackerConfig()) {
    return PerceptionPipeline::create()
        .withCamera(camera)
        .withDetector(detector)
        .withTracker(tracker)
        .build();
}

/**
 * @brief Create a minimal pipeline with auto-configured defaults
 */
inline ResultPtr<PerceptionPipeline> createMinimalPipeline(
    const std::string& modelPath,
    DetectorBackend backend = DetectorBackend::AUTO) {
    return PerceptionPipeline::create()
        .withCamera(640, 480, 30)
        .withDetector(modelPath, backend)
        .withTracker(TrackerType::DEEPSORT)
        .build();
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
