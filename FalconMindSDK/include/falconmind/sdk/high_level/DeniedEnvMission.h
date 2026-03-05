/**
 * @file DeniedEnvMission.h
 * @brief 拒止环境任务高层封装
 * 
 * 一键式拒止环境任务API，整合VINS、GPS防护、视觉跟踪能力：
 * - VINS初始化管理
 * - GPS欺骗自动防护
 * - 目标检测与跟踪
 * - 视觉伺服控制
 * 
 * @author FalconMind SDK Team
 * @version 1.0.0
 * @date 2026-03-04
 */

#pragma once

#include <memory>
#include <functional>
#include <vector>
#include <optional>
#include <falconmind/sdk/core/Node.h>
#include <falconmind/sdk/navigation/GPSDefender.h>
#include <falconmind/sdk/control/IBVSController.h>
#include <falconmind/sdk/perception/MonocularDistanceEstimator.h>
#include <falconmind/sdk/flight/FlightTypes.h>

namespace falconmind {
namespace sdk {
namespace high_level {

/**
 * @brief 搜索区域定义
 */
struct SearchArea {
    std::vector<std::pair<double, double>> boundary;  // 边界点 [(lat, lon), ...]
    double altitude{50.0};                            // 搜索高度
    double speed{5.0};                                // 搜索速度
    std::string pattern{"LAWN_MOWER"};               // 搜索模式
};

/**
 * @brief 目标选择信息
 */
struct TargetSelection {
    int track_id{-1};
    std::string class_name;
    double confidence{0.0};
    double estimated_distance{0.0};
    bool confirmed{false};
};

/**
 * @brief 跟踪参数
 */
struct TrackingParameters {
    double desired_distance{30.0};      // 期望距离
    double desired_height{10.0};        // 期望高度
    double distance_tolerance{2.0};     // 距离容差
    double height_tolerance{1.0};       // 高度容差
    double max_speed{8.0};              // 最大跟踪速度
    double tracking_timeout{10.0};      // 目标丢失超时
};

/**
 * @brief 任务事件回调
 */
struct DeniedEnvMissionCallbacks {
    // 阶段转换
    std::function<void(const std::string& from, const std::string& to)> onPhaseTransition;
    
    // VINS初始化
    std::function<void(double progress)> onVINSProgress;
    std::function<void(bool success)> onVINSComplete;
    
    // GPS防护
    std::function<void(navigation::SpoofingAlertLevel level, const std::string& reason)> onSpoofingAlert;
    
    // 目标检测
    std::function<void(const std::vector<perception::Detection>& detections)> onTargetsDetected;
    
    // 跟踪状态
    std::function<void(double distance, double height, double quality)> onTrackingUpdate;
    
    // 错误处理
    std::function<void(const std::string& error)> onError;
    
    // 任务完成
    std::function<void(bool success)> onMissionComplete;
};

/**
 * @brief 拒止环境任务配置
 */
struct DeniedEnvMissionConfig {
    // VINS配置
    double vins_init_timeout{60.0};     // VINS初始化超时
    int vins_required_features{150};    // 所需特征点数
    
    // GPS防护配置
    navigation::GPSDefenderConfig gps_defense_config;
    
    // 跟踪配置
    TrackingParameters tracking_params;
    control::IBVSConfig ibvs_config;
    
    // 相机配置
    perception::MonocularCameraIntrinsics camera_intrinsics;
    
    // 目标类别
    std::vector<std::string> target_classes{"person", "vehicle"};
    
    // 回调
    DeniedEnvMissionCallbacks callbacks;
};

/**
 * @brief 拒止环境视觉跟踪任务
 * 
 * 高层封装，一键启动拒止环境完整任务流程：
 * 
 * 使用示例：
 * @code
 * // 创建任务
 * DeniedEnvMissionConfig config;
 * config.tracking_params.desired_distance = 30.0;
 * config.callbacks.onTargetsDetected = [](const auto& dets) {
 *     for (const auto& det : dets) {
 *         std::cout << "Detected: " << det.className << "\n";
 *     }
 * };
 * 
 * auto mission = createDeniedEnvMission(config);
 * 
 * // 初始化VINS
 * if (!mission->initializeVINS()) {
 *     std::cerr << "VINS init failed\n";
 *     return 1;
 * }
 * 
 * // 设置搜索区域
 * SearchArea area;
 * area.boundary = {{40.0768, 116.3477}, {40.0778, 116.3477}, ...};
 * 
 * // 开始搜索
 * mission->startSearch(area);
 * 
 * // 目标选择后，开始跟踪
 * mission->startTracking(target_id);
 * 
 * // 任务完成
 * mission->returnToLaunch();
 * @endcode
 */
class DeniedEnvMission : public core::Node {
public:
    explicit DeniedEnvMission(const DeniedEnvMissionConfig& config);
    ~DeniedEnvMission() override = default;
    
    /**
     * @brief 初始化任务
     */
    bool initialize() override;
    
    /**
     * @brief 初始化VINS（阻塞直到完成或超时）
     * @return 是否成功
     */
    bool initializeVINS();
    
    /**
     * @brief 获取VINS初始化进度
     * @return 进度 0-1
     */
    double getVINSProgress() const;
    
    /**
     * @brief 启动区域搜索
     * @param area 搜索区域
     */
    bool startSearch(const SearchArea& area);
    
    /**
     * @brief 获取当前检测到的目标
     */
    std::vector<perception::Detection> getDetectedTargets() const;
    
    /**
     * @brief 选择跟踪目标
     * @param track_id 跟踪ID
     * @return 是否成功
     */
    bool selectTarget(int track_id);
    
    /**
     * @brief 确认目标选择（开始跟踪）
     * @return 是否成功
     */
    bool confirmTarget();
    
    /**
     * @brief 开始跟踪（直接指定目标ID）
     * @param track_id 跟踪ID
     */
    bool startTracking(int track_id);
    
    /**
     * @brief 开始跟踪（已知目标类别和位置）
     * @param class_name 目标类别
     * @param bbox 边界框
     */
    bool startTracking(const std::string& class_name, 
                      const perception::BoundingBox& bbox);
    
    /**
     * @brief 获取跟踪质量
     */
    control::TrackingQuality getTrackingQuality() const;
    
    /**
     * @brief 是否正在跟踪
     */
    bool isTracking() const;
    
    /**
     * @brief 中止任务并返航
     */
    void abortAndReturn();
    
    /**
     * @brief 返航
     */
    void returnToLaunch();
    
    /**
     * @brief 降落
     */
    void land();
    
    /**
     * @brief 获取GPS防护器
     */
    std::shared_ptr<navigation::GPSDefender> getGPSDefender() const;
    
    /**
     * @brief 获取IBVS控制器
     */
    std::shared_ptr<control::IBVSController> getIBVSController() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief 便捷函数：创建拒止环境任务
 */
std::shared_ptr<DeniedEnvMission> createDeniedEnvMission(
    const DeniedEnvMissionConfig& config
);

/**
 * @brief 便捷函数：创建标准拒止环境任务（使用默认配置）
 */
std::shared_ptr<DeniedEnvMission> createStandardDeniedEnvMission();

} // namespace high_level
} // namespace sdk
} // namespace falconmind
