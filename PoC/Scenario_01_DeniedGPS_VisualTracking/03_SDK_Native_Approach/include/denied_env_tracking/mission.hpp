/**
 * @file mission.hpp
 * @brief Denied Environment Visual Tracking Mission
 * 
 * 拒止环境区域侦查与视觉制导跟踪任务的主控模块
 */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <functional>

#include "navigation.hpp"
#include "perception.hpp"
#include "control.hpp"

namespace falconmind {
namespace denied_env_tracking {

/**
 * @brief 任务阶段枚举
 */
enum class MissionPhase {
    INITIALIZING,      ///< VINS初始化
    SEARCHING,         ///< 区域侦查
    TARGET_ACQUIRED,   ///< 发现目标，等待确认
    TRACKING,          ///< 视觉跟踪
    RETURNING,         ///< 返航
    LANDED,            ///< 已降落
    ABORTED            ///< 任务中止
};

/**
 * @brief 任务配置
 */
struct MissionConfig {
    // 区域侦查参数
    std::vector<GeodeticPosition> search_area;  ///< 搜索区域边界
    double search_altitude{50.0};               ///< 侦查高度 (m)
    double search_speed{5.0};                   ///< 侦查速度 (m/s)
    std::string search_pattern{"LAWN_MOWER"};    ///< 搜索模式
    
    // 跟踪参数
    double desired_distance{30.0};              ///< 目标距离 (m)
    double desired_height{10.0};                ///< 目标高度 (m)
    double distance_tolerance{2.0};             ///< 距离容差 (m)
    double height_tolerance{1.0};               ///< 高度容差 (m)
    double max_tracking_speed{8.0};             ///< 最大跟踪速度 (m/s)
    double tracking_timeout{10.0};              ///< 目标丢失超时 (s)
    
    // VINS参数
    double vins_init_time{30.0};                ///< VINS初始化时间 (s)
    int vins_required_features{150};            ///< 所需特征点数量
    
    // GPS防护参数
    bool enable_gps_defense{true};              ///< 启用GPS防护
    double spoofing_check_interval{1.0};        ///< 检测间隔 (s)
    
    // 通信参数
    std::string gcs_endpoint;                   ///< 地面站地址
    double telemetry_rate{5.0};                 ///< 遥测频率 (Hz)
};

/**
 * @brief 任务状态
 */
struct MissionState {
    MissionPhase phase{MissionPhase::INITIALIZING};
    
    // 位置状态
    GeodeticPosition gnss_position;             ///< GNSS位置
    VisualPosition visual_position;             ///< VINS视觉位置
    VisualPosition fused_position;              ///< EKF融合位置
    
    // 目标状态
    std::vector<TargetDetection> detected_targets;
    std::optional<TargetSelection> selected_target;
    std::optional<TargetDetection> current_target;
    
    // 跟踪状态
    double tracking_duration{0.0};              ///< 跟踪时长 (s)
    double current_distance{0.0};               ///< 当前距离 (m)
    double current_height{0.0};                 ///< 当前高度 (m)
    
    // 系统状态
    double battery_percent{100.0};
    bool vins_initialized{false};
    bool gps_spoofing_detected{false};
    GNSSStatus gnss_status{GNSSStatus::HEALTHY};
    
    std::chrono::steady_clock::time_point last_update;
};

/**
 * @brief 任务事件回调
 */
struct MissionCallbacks {
    std::function<void(MissionPhase, MissionPhase)> on_phase_transition;
    std::function<void(const TargetDetection&)> on_target_detected;
    std::function<void(int)> on_target_selected;  ///< track_id
    std::function<void()> on_target_confirmed;
    std::function<void(double, double)> on_tracking_update;  ///< distance, height
    std::function<void(const std::string&)> on_error;
    std::function<void()> on_mission_complete;
};

/**
 * @brief 拒止环境视觉跟踪任务
 * 
 * 主控制器，协调导航、感知、控制三个子系统
 */
class DeniedEnvTrackingMission {
public:
    explicit DeniedEnvTrackingMission(const MissionConfig& config);
    ~DeniedEnvTrackingMission();
    
    // 禁止拷贝
    DeniedEnvTrackingMission(const DeniedEnvTrackingMission&) = delete;
    DeniedEnvTrackingMission& operator=(const DeniedEnvTrackingMission&) = delete;
    
    /**
     * @brief 初始化任务
     * @return 是否成功
     */
    bool initialize();
    
    /**
     * @brief 启动任务
     * @return 是否成功启动
     */
    bool start();
    
    /**
     * @brief 停止任务
     */
    void stop();
    
    /**
     * @brief 等待任务完成
     */
    void waitForCompletion();
    
    /**
     * @brief 选择目标
     * @param track_id 跟踪ID
     * @param operator_id 操作员ID
     * @return 是否成功
     */
    bool selectTarget(int track_id, const std::string& operator_id);
    
    /**
     * @brief 确认目标选择
     * @param confirmed 是否确认
     * @param operator_id 操作员ID
     * @return 是否成功
     */
    bool confirmTarget(bool confirmed, const std::string& operator_id);
    
    /**
     * @brief 中止任务
     * @param reason 中止原因
     */
    void abort(const std::string& reason);
    
    /**
     * @brief 获取当前状态
     */
    MissionState getState() const;
    
    /**
     * @brief 设置事件回调
     */
    void setCallbacks(const MissionCallbacks& callbacks);
    
    /**
     * @brief 处理地面站指令
     */
    void handleGCSCommand(const std::string& command, const std::string& params);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief 便捷API：一键启动拒止环境跟踪任务
 */
std::unique_ptr<DeniedEnvTrackingMission> createDeniedEnvTrackingMission(
    const MissionConfig& config,
    const MissionCallbacks& callbacks
);

} // namespace denied_env_tracking
} // namespace falconmind
