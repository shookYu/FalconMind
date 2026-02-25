/**
 * @file PerceptionPipeline.cpp
 * @brief Implementation of high-level perception pipeline API
 */

#include "falconmind/sdk/high_level/PerceptionPipeline.h"
#include <iostream>
#include <unistd.h>

namespace falconmind {
namespace sdk {
namespace high_level {

// Builder implementation
PerceptionPipelineBuilder& PerceptionPipelineBuilder::withCamera(const CameraConfig& config) {
    cameraConfig_ = config;
    hasCamera_ = true;
    return *this;
}

PerceptionPipelineBuilder& PerceptionPipelineBuilder::withCamera(int width, int height, int fps) {
    cameraConfig_.width = width;
    cameraConfig_.height = height;
    cameraConfig_.fps = fps;
    hasCamera_ = true;
    return *this;
}

PerceptionPipelineBuilder& PerceptionPipelineBuilder::withCameraDevice(const std::string& device) {
    cameraConfig_.device = device;
    hasCamera_ = true;
    return *this;
}

PerceptionPipelineBuilder& PerceptionPipelineBuilder::withDetector(const DetectorConfig& config) {
    detectorConfig_ = config;
    hasDetector_ = true;
    return *this;
}

PerceptionPipelineBuilder& PerceptionPipelineBuilder::withDetector(const std::string& modelPath, 
                                                                     DetectorBackend backend) {
    detectorConfig_.modelPath = modelPath;
    detectorConfig_.backend = backend;
    hasDetector_ = true;
    return *this;
}

PerceptionPipelineBuilder& PerceptionPipelineBuilder::withTracker(const TrackerConfig& config) {
    trackerConfig_ = config;
    return *this;
}

PerceptionPipelineBuilder& PerceptionPipelineBuilder::withTracker(TrackerType type) {
    trackerConfig_.type = type;
    return *this;
}

PerceptionPipelineBuilder& PerceptionPipelineBuilder::withLowLightEnhancement(bool enable) {
    lowLightEnhancement_ = enable;
    return *this;
}

ResultPtr<PerceptionPipeline> PerceptionPipelineBuilder::build() {
    if (!hasCamera_) {
        return ResultPtr<PerceptionPipeline>::error(
            ErrorCode::MissingRequiredParameter, 
            "Camera configuration is required");
    }
    
    if (!hasDetector_) {
        return ResultPtr<PerceptionPipeline>::error(
            ErrorCode::MissingRequiredParameter,
            "Detector configuration is required");
    }
    
    auto pipeline = std::shared_ptr<PerceptionPipeline>(new PerceptionPipeline());
    pipeline->impl_ = std::make_unique<PerceptionPipeline::Impl>();
    pipeline->impl_->cameraConfig_ = cameraConfig_;
    pipeline->impl_->detectorConfig_ = detectorConfig_;
    pipeline->impl_->trackerConfig_ = trackerConfig_;
    pipeline->impl_->lowLightEnhancement_ = lowLightEnhancement_;
    
    return ResultPtr<PerceptionPipeline>::success(pipeline);
}

// PerceptionPipeline implementation
PerceptionPipeline::~PerceptionPipeline() = default;

PerceptionPipelineBuilder PerceptionPipeline::create() {
    return PerceptionPipelineBuilder();
}

Result<void> PerceptionPipeline::start() {
    if (!impl_) {
        return Result<void>::error(ErrorCode::InternalError, "Implementation not initialized");
    }
    
    if (impl_->isRunning_) {
        return Result<void>::error(ErrorCode::PipelineAlreadyRunning, "Pipeline already running");
    }
    
    std::cout << "[PerceptionPipeline] Starting with camera: " 
              << impl_->cameraConfig_.width << "x" << impl_->cameraConfig_.height 
              << " @ " << impl_->cameraConfig_.fps << "fps" << std::endl;
    std::cout << "[PerceptionPipeline] Detector: " << impl_->detectorConfig_.modelPath << std::endl;
    
    impl_->isRunning_ = true;
    return Result<void>::success();
}

Result<void> PerceptionPipeline::stop() {
    if (!impl_) {
        return Result<void>::error(ErrorCode::InternalError, "Implementation not initialized");
    }
    
    if (!impl_->isRunning_) {
        return Result<void>::success();
    }
    
    std::cout << "[PerceptionPipeline] Stopping" << std::endl;
    impl_->isRunning_ = false;
    
    return Result<void>::success();
}

bool PerceptionPipeline::isRunning() const {
    return impl_ && impl_->isRunning_;
}

void PerceptionPipeline::onDetection(std::function<void(const std::vector<Detection>&)> callback) {
    if (impl_) {
        impl_->onDetectionBatch_ = callback;
    }
}

void PerceptionPipeline::onDetection(std::function<void(const Detection&)> callback) {
    if (impl_) {
        impl_->onDetectionSingle_ = callback;
    }
}

void PerceptionPipeline::onObjectDetected(const std::string& className, 
                                          std::function<void(const Detection&)> callback) {
    if (impl_) {
        impl_->targetClassName_ = className;
        impl_->onObjectDetected_ = callback;
    }
}

std::vector<Detection> PerceptionPipeline::getLatestDetections() const {
    return {};
}

std::optional<Detection> PerceptionPipeline::getObject(const std::string& className) const {
    auto detections = getLatestDetections();
    for (const auto& det : detections) {
        if (det.className == className) {
            return det;
        }
    }
    return std::nullopt;
}

std::optional<Detection> PerceptionPipeline::getTrackedObject(int trackId) const {
    auto detections = getLatestDetections();
    for (const auto& det : detections) {
        if (det.trackId == trackId) {
            return det;
        }
    }
    return std::nullopt;
}

size_t PerceptionPipeline::getDetectionCount() const {
    return getLatestDetections().size();
}

size_t PerceptionPipeline::getTrackingCount() const {
    size_t count = 0;
    for (const auto& det : getLatestDetections()) {
        if (det.isTracked()) count++;
    }
    return count;
}

PerceptionPipeline::Statistics PerceptionPipeline::getStatistics() const {
    if (!impl_) return {};
    return impl_->stats_;
}

void PerceptionPipeline::wait() {
    if (!impl_ || !impl_->isRunning_) return;
    
    while (impl_->isRunning_) {
        usleep(100000);
    }
}

void PerceptionPipeline::setClassEnabled(const std::string& className, bool enabled) {
    (void)className;
    (void)enabled;
}

void PerceptionPipeline::setConfidenceThreshold(float threshold) {
    if (impl_) {
        impl_->detectorConfig_.confidenceThreshold = threshold;
    }
}

} // namespace high_level
} // namespace sdk
} // namespace falconmind
