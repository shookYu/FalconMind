/**
 * @file MonocularDistanceEstimator.cpp
 * @brief 单目相机距离估计器实现
 */

#include <falconmind/sdk/perception/MonocularDistanceEstimator.h>
#include <falconmind/sdk/core/Logger.h>
#include <math>

namespace falconmind {
namespace sdk {
namespace perception {

class MonocularDistanceEstimator::Impl {
public:
    MonocularCameraIntrinsics intrinsics;
    std::map<std::string, ObjectDimensions> object_types;
    
    explicit Impl(const MonocularCameraIntrinsics& intr) : intrinsics(intr) {}
    
    /**
     * @brief 计算从高度估计距离
     */
    double estimateFromHeight(double pixel_height, double real_height) const {
        if (pixel_height < 1.0) {
            return -1.0;  // 避免除零
        }
        return (intrinsics.fy * real_height) / pixel_height;
    }
    
    /**
     * @brief 计算从宽度估计距离
     */
    double estimateFromWidth(double pixel_width, double real_width) const {
        if (pixel_width < 1.0) {
            return -1.0;
        }
        return (intrinsics.fx * real_width) / pixel_width;
    }
    
    /**
     * @brief 计算置信度（基于目标大小）
     */
    double computeConfidence(double pixel_size) const {
        // 目标太小或太大都降低置信度
        const double optimal_size = 100.0;  // 像素
        const double min_size = 20.0;
        const double max_size = 500.0;
        
        if (pixel_size < min_size || pixel_size > max_size) {
            return 0.3;
        }
        
        // 在最优大小附近置信度最高
        double ratio = pixel_size / optimal_size;
        if (ratio > 1.0) {
            ratio = 1.0 / ratio;
        }
        
        return 0.3 + 0.7 * ratio;
    }
};

MonocularDistanceEstimator::MonocularDistanceEstimator(
    const MonocularCameraIntrinsics& intrinsics
) : pImpl(std::make_unique<Impl>(intrinsics)) {
}

bool MonocularDistanceEstimator::initialize() {
    FALCONMIND_LOG_INFO("Initializing Monocular Distance Estimator");
    FALCONMIND_LOG_INFO("  Camera: {}x{}", pImpl->intrinsics.width, pImpl->intrinsics.height);
    FALCONMIND_LOG_INFO("  Focal: fx={}, fy={}", pImpl->intrinsics.fx, pImpl->intrinsics.fy);
    
    // 注册默认类型
    registerDefaultTypes(*this);
    
    return Node::initialize();
}

void MonocularDistanceEstimator::registerObjectType(
    const std::string& class_name,
    double height,
    double width,
    double depth
) {
    ObjectDimensions dims;
    dims.height = height;
    dims.width = width;
    dims.depth = depth;
    
    pImpl->object_types[class_name] = dims;
    
    FALCONMIND_LOG_DEBUG("Registered object type '{}' with dimensions {:.2f}x{:.2f}x{:.2f}m",
        class_name, height, width, depth);
}

DistanceEstimate MonocularDistanceEstimator::estimate(
    const BoundingBox& bbox,
    const std::string& class_name
) const {
    DistanceEstimate result;
    result.target_class = class_name;
    result.method = DistanceEstimationMethod::KNOWN_SIZE;
    
    auto it = pImpl->object_types.find(class_name);
    if (it == pImpl->object_types.end()) {
        FALCONMIND_LOG_WARN("Unknown object type: {}", class_name);
        result.confidence = 0.0;
        return result;
    }
    
    const auto& dims = it->second;
    
    // 使用高度估计（通常更准确）
    double pixel_height = bbox.y2 - bbox.y1;
    double distance_from_height = pImpl->estimateFromHeight(
        pixel_height, dims.height);
    
    // 如果宽度已知，也估计一次
    double distance = distance_from_height;
    if (dims.width > 0) {
        double pixel_width = bbox.x2 - bbox.x1;
        double distance_from_width = pImpl->estimateFromWidth(
            pixel_width, dims.width);
        
        // 如果两个估计都有效，取平均
        if (distance_from_height > 0 && distance_from_width > 0) {
            distance = (distance_from_height + distance_from_width) / 2.0;
        } else if (distance_from_width > 0) {
            distance = distance_from_width;
        }
    }
    
    result.distance = distance;
    result.confidence = pImpl->computeConfidence(pixel_height);
    
    return result;
}

DistanceEstimate MonocularDistanceEstimator::estimateWithHeight(
    const BoundingBox& bbox,
    double real_height
) const {
    DistanceEstimate result;
    result.target_class = "unknown";
    result.method = DistanceEstimationMethod::KNOWN_SIZE;
    
    double pixel_height = bbox.y2 - bbox.y1;
    result.distance = pImpl->estimateFromHeight(pixel_height, real_height);
    result.confidence = pImpl->computeConfidence(pixel_height);
    
    return result;
}

DistanceEstimate MonocularDistanceEstimator::estimateFromArea(
    const BoundingBox& bbox,
    const std::string& class_name
) const {
    DistanceEstimate result;
    result.target_class = class_name;
    result.method = DistanceEstimationMethod::KNOWN_SIZE;
    
    auto it = pImpl->object_types.find(class_name);
    if (it == pImpl->object_types.end()) {
        result.confidence = 0.0;
        return result;
    }
    
    const auto& dims = it->second;
    
    // 使用面积估计（假设目标投影为矩形）
    double real_area = dims.height * dims.width;
    double pixel_area = (bbox.y2 - bbox.y1) * (bbox.x2 - bbox.x1);
    
    if (pixel_area < 1.0) {
        result.distance = -1.0;
        result.confidence = 0.0;
        return result;
    }
    
    // 面积与距离平方成反比
    double focal_area = pImpl->intrinsics.fx * pImpl->intrinsics.fy;
    result.distance = std::sqrt(focal_area * real_area / pixel_area);
    result.confidence = pImpl->computeConfidence(std::sqrt(pixel_area));
    
    return result;
}

std::vector<DistanceEstimate> MonocularDistanceEstimator::estimateBatch(
    const std::vector<Detection>& detections
) const {
    std::vector<DistanceEstimate> results;
    results.reserve(detections.size());
    
    for (const auto& det : detections) {
        results.push_back(estimate(det.bbox, det.className));
    }
    
    return results;
}

void MonocularDistanceEstimator::setCameraIntrinsics(
    const MonocularCameraIntrinsics& intrinsics
) {
    pImpl->intrinsics = intrinsics;
    FALCONMIND_LOG_INFO("Camera intrinsics updated");
}

MonocularCameraIntrinsics MonocularDistanceEstimator::getCameraIntrinsics() const {
    return pImpl->intrinsics;
}

std::vector<std::string> MonocularDistanceEstimator::getRegisteredClasses() const {
    std::vector<std::string> classes;
    for (const auto& [name, _] : pImpl->object_types) {
        classes.push_back(name);
    }
    return classes;
}

std::optional<ObjectDimensions> MonocularDistanceEstimator::getObjectDimensions(
    const std::string& class_name
) const {
    auto it = pImpl->object_types.find(class_name);
    if (it != pImpl->object_types.end()) {
        return it->second;
    }
    return std::nullopt;
}

void MonocularDistanceEstimator::registerDefaultTypes(
    MonocularDistanceEstimator& estimator
) {
    // 人员（平均身高1.7m，宽度0.5m）
    estimator.registerObjectType("person", 1.7, 0.5, 0.3);
    estimator.registerObjectType("pedestrian", 1.7, 0.5, 0.3);
    
    // 车辆
    estimator.registerObjectType("car", 1.5, 1.8, 4.5);
    estimator.registerObjectType("vehicle", 1.6, 1.9, 4.8);
    estimator.registerObjectType("suv", 1.7, 1.9, 4.7);
    estimator.registerObjectType("truck", 2.5, 2.5, 10.0);
    
    // 两轮车
    estimator.registerObjectType("bicycle", 1.0, 0.6, 1.7);
    estimator.registerObjectType("motorcycle", 1.2, 0.7, 2.0);
    
    // 其他常见目标
    estimator.registerObjectType("dog", 0.5, 0.3, 0.6);
    estimator.registerObjectType("cat", 0.25, 0.2, 0.4);
    
    FALCONMIND_LOG_INFO("Registered {} default object types", 
        estimator.getRegisteredClasses().size());
}

std::shared_ptr<MonocularDistanceEstimator> createDistanceEstimator(
    const MonocularCameraIntrinsics& intrinsics
) {
    auto estimator = std::make_shared<MonocularDistanceEstimator>(intrinsics);
    return estimator;
}

} // namespace perception
} // namespace sdk
} // namespace falconmind
