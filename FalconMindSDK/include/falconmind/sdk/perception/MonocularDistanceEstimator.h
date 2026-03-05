/**
 * @file MonocularDistanceEstimator.h
 * @brief 单目相机距离估计器
 * 
 * 使用已知目标尺寸和相机内参估计距离：
 * distance = (focal_length * real_height) / pixel_height
 * 
 * @author FalconMind SDK Team
 * @version 1.0.0
 * @date 2026-03-04
 */

#pragma once

#include <math>
#include <map>
#include <string>
#include <falconmind/sdk/core/Node.h>
#include <falconmind/sdk/perception/DetectionTypes.h>

namespace falconmind {
namespace sdk {
namespace perception {

/**
 * @brief 目标类型预设尺寸
 */
struct ObjectDimensions {
    double height{1.7};    /// 高度 (m)
    double width{0.5};     /// 宽度 (m)
    double depth{0.3};     /// 深度 (m)
};

/**
 * @brief 距离估计方法
 */
enum class DistanceEstimationMethod {
    KNOWN_SIZE,     /// 已知目标尺寸法
    NEURAL_NETWORK, /// 深度神经网络
    STEREO_DISPARITY /// 双目视差（如果有双目相机）
};

/**
 * @brief 距离估计结果
 */
struct DistanceEstimate {
    double distance{-1.0};          /// 估计距离 (m)，-1表示失败
    double confidence{0.0};         /// 置信度 0-1
    DistanceEstimationMethod method; /// 使用的方法
    std::string target_class;       /// 目标类别
};

/**
 * @brief 单目相机内参
 */
struct MonocularCameraIntrinsics {
    double fx{1000.0};      /// x方向焦距 (像素)
    double fy{1000.0};      /// y方向焦距 (像素)
    double cx{960.0};       /// x方向光心
    double cy{540.0};       /// y方向光心
    int width{1920};        /// 图像宽度
    int height{1080};       /// 图像高度
    
    /**
     * @brief 从FOV计算焦距
     * @param fov_degrees 视场角 (度)
     * @param is_horizontal 是否为水平FOV
     */
    static MonocularCameraIntrinsics fromFOV(
        double fov_degrees, 
        int width, 
        int height,
        bool is_horizontal = true
    ) {
        MonocularCameraIntrinsics intrinsics;
        intrinsics.width = width;
        intrinsics.height = height;
        intrinsics.cx = width / 2.0;
        intrinsics.cy = height / 2.0;
        
        double fov_rad = fov_degrees * M_PI / 180.0;
        if (is_horizontal) {
            intrinsics.fx = (width / 2.0) / std::tan(fov_rad / 2.0);
            intrinsics.fy = intrinsics.fx;  // 假设方形像素
        } else {
            intrinsics.fy = (height / 2.0) / std::tan(fov_rad / 2.0);
            intrinsics.fx = intrinsics.fy;
        }
        
        return intrinsics;
    }
};

/**
 * @brief 单目距离估计器
 * 
 * 基于已知目标尺寸和相机内参估计距离。
 * 适用于跟踪特定类型目标（如人员、车辆）。
 * 
 * 使用示例：
 * @code
 * MonocularDistanceEstimator estimator(camera_intrinsics);
 * 
 * // 注册目标类型尺寸
 * estimator.registerObjectType("person", 1.7, 0.5, 0.3);
 * estimator.registerObjectType("car", 1.5, 1.8, 4.5);
 * 
 * // 估计距离
 * auto result = estimator.estimate(detection.bbox, "person");
 * if (result.distance > 0) {
 *     std::cout << "Distance: " << result.distance << "m\n";
 * }
 * @endcode
 */
class MonocularDistanceEstimator : public core::Node {
public:
    /**
     * @brief 构造函数
     * @param intrinsics 相机内参
     */
    explicit MonocularDistanceEstimator(
        const MonocularCameraIntrinsics& intrinsics = MonocularCameraIntrinsics{}
    );
    
    ~MonocularDistanceEstimator() override = default;
    
    /**
     * @brief 初始化估计器
     */
    bool initialize() override;
    
    /**
     * @brief 注册目标类型尺寸
     * @param class_name 类别名称
     * @param height 高度 (m)
     * @param width 宽度 (m)
     * @param depth 深度 (m)
     */
    void registerObjectType(
        const std::string& class_name,
        double height,
        double width = 0.0,
        double depth = 0.0
    );
    
    /**
     * @brief 使用预设尺寸估计距离
     * @param bbox 边界框
     * @param class_name 目标类别
     * @return 距离估计结果
     */
    DistanceEstimate estimate(
        const BoundingBox& bbox,
        const std::string& class_name
    ) const;
    
    /**
     * @brief 使用指定高度估计距离
     * @param bbox 边界框
     * @param real_height 目标真实高度 (m)
     * @return 距离估计结果
     */
    DistanceEstimate estimateWithHeight(
        const BoundingBox& bbox,
        double real_height
    ) const;
    
    /**
     * @brief 估计距离（使用边界框面积）
     * @param bbox 边界框
     * @param class_name 目标类别
     * @return 距离估计结果
     */
    DistanceEstimate estimateFromArea(
        const BoundingBox& bbox,
        const std::string& class_name
    ) const;
    
    /**
     * @brief 批量估计
     * @param detections 检测列表
     * @return 距离估计列表
     */
    std::vector<DistanceEstimate> estimateBatch(
        const std::vector<Detection>& detections
    ) const;
    
    /**
     * @brief 设置相机内参
     */
    void setCameraIntrinsics(const MonocularCameraIntrinsics& intrinsics);
    
    /**
     * @brief 获取相机内参
     */
    MonocularCameraIntrinsics getCameraIntrinsics() const;
    
    /**
     * @brief 获取已注册的类别列表
     */
    std::vector<std::string> getRegisteredClasses() const;
    
    /**
     * @brief 获取类别的尺寸
     */
    std::optional<ObjectDimensions> getObjectDimensions(
        const std::string& class_name
    ) const;

    /**
     * @brief 注册默认类别（人员、车辆等）
     */
    static void registerDefaultTypes(MonocularDistanceEstimator& estimator);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief 便捷函数：创建标准距离估计器（预设常见类别）
 */
std::shared_ptr<MonocularDistanceEstimator> createDistanceEstimator(
    const MonocularCameraIntrinsics& intrinsics = MonocularCameraIntrinsics{}
);

} // namespace perception
} // namespace sdk
} // namespace falconmind
