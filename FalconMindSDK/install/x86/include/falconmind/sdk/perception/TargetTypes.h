/**
 * @file TargetTypes.h
 * @brief 目标跟踪相关类型定义
 */

#pragma once

#include "falconmind/sdk/sensors/NavigationTypes.h"
#include "falconmind/sdk/perception/DetectionTypes.h"
#include <cstdint>
#include <string>
#include <vector>
#include <chrono>

namespace falconmind {
namespace sdk {
namespace perception {

/**
 * @brief 目标类型
 */
enum class TargetType {
    UNKNOWN,
    PERSON,
    VEHICLE,
    BUILDING,
    ANIMAL,
    OBJECT,
    CUSTOM
};

/**
 * @brief 目标信息
 * 
 * 用于视觉制导和跟踪任务
 */
struct Target {
    uint64_t id{0};                     ///< 目标唯一ID
    std::string className;              ///< 类别名称
    TargetType type{TargetType::UNKNOWN};
    
    // 图像坐标
    float imageX{0.0f};                 ///< 图像X坐标（像素）
    float imageY{0.0f};                 ///< 图像Y坐标（像素）
    float imageWidth{0.0f};             ///< 图像中宽度（像素）
    float imageHeight{0.0f};            ///< 图像中高度（像素）
    
    // 置信度
    float confidence{0.0f};             ///< 检测置信度（0-1）
    
    // 3D位置（如果有）
    std::optional<sensors::GeoPoint> geoPosition;      ///< 地理坐标
    std::optional<sensors::LocalPosition> localPosition; ///< 本地坐标
    std::optional<sensors::RelativePosition> relativePosition; ///< 相对位置
    
    // 目标属性
    double estimatedDistance{0.0};      ///< 估计距离（米）
    double estimatedSpeed{0.0};         ///< 估计速度（m/s）
    double estimatedSize{0.0};          ///< 估计大小（米）
    
    // 跟踪信息
    int trackId{-1};                    ///< 跟踪ID（-1表示未跟踪）
    std::chrono::steady_clock::time_point firstSeen;
    std::chrono::steady_clock::time_point lastSeen;
    int framesTracked{0};               ///< 已跟踪帧数
    
    // 历史轨迹
    std::vector<sensors::GeoPoint> trajectory;
    
    // 状态
    bool isTracked{false};
    bool isLost{false};
    bool isConfirmed{false};            ///< 是否确认为真实目标（过滤误检）
    
    Target() = default;
    Target(const Detection& detection) 
        : className(detection.className),
          confidence(detection.score),
          imageX(detection.bbox.x + detection.bbox.width / 2),
          imageY(detection.bbox.y + detection.bbox.height / 2),
          imageWidth(detection.bbox.width),
          imageHeight(detection.bbox.height) {}
    
    bool isValid() const {
        return confidence > 0.0 && !className.empty();
    }
    
    float getImageCenterX() const { return imageX; }
    float getImageCenterY() const { return imageY; }
    
    std::chrono::seconds getTrackingDuration() const {
        return std::chrono::duration_cast<std::chrono::seconds>(
            lastSeen - firstSeen);
    }
};

/**
 * @brief 目标列表
 */
using TargetList = std::vector<Target>;

/**
 * @brief 目标选择策略
 */
enum class TargetSelectionStrategy {
    NEAREST,        ///< 选择最近的
    LARGEST,        ///< 选择最大的
    MOST_CONFIDENT, ///< 选择置信度最高的
    CENTER_MOST,    ///< 选择最靠近图像中心的
    SPECIFIC_ID     ///< 选择特定ID
};

/**
 * @brief 目标选择器
 */
struct TargetSelector {
    TargetSelectionStrategy strategy{TargetSelectionStrategy::NEAREST};
    std::string targetClass;            ///< 目标类别过滤
    int specificTrackId{-1};            ///< 特定跟踪ID
    float minConfidence{0.5f};          ///< 最小置信度
    float maxDistance{100.0f};          ///< 最大距离
    
    std::optional<Target> select(const TargetList& targets) const;
    std::vector<Target> filter(const TargetList& targets) const;
};

} // namespace perception
} // namespace sdk
} // namespace falconmind
