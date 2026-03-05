/**
 * @file mission_config.hpp
 * @brief Mission配置数据结构
 * 
 * 定义拒止环境任务的Mission配置结构
 */

#pragma once

#include <vector>
#include <string>

#include <nlohmann/json.hpp>

namespace falconmind {
namespace sdk {
namespace mission {

using json = nlohmann::json;

/**
 * @brief WGS84地理坐标
 */
struct GeodeticPosition {
    double latitude;   ///< 纬度 (度)
    double longitude;  ///< 经度 (度)
    double altitude;   ///< 椭球高 (米)
};

/**
 * @brief PID参数
 */
struct PIDParams {
    double kp = 0.5;   ///< 比例增益
    double ki = 0.1;   ///< 积分增益
    double kd = 0.2;   ///< 微分增益
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PIDParams, kp, ki, kd)
};

/**
 * @brief 搜索配置
 */
struct SearchConfig {
    std::vector<GeodeticPosition> area;  ///< 搜索区域多边形
    double altitude = 50.0;               ///< 搜索高度 (米)
    double speed = 5.0;                   ///< 搜索速度 (m/s)
    std::string pattern = "LAWN_MOWER";   ///< 搜索模式
    double overlap_rate = 0.2;            ///< 重叠率
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(SearchConfig, area, altitude, speed, pattern, overlap_rate)
};

/**
 * @brief 跟踪配置
 */
struct TrackingConfig {
    std::string target_class = "person";     ///< 目标类别
    double desired_distance = 30.0;          ///< 期望距离 (米)
    double distance_tolerance = 2.0;         ///< 距离容差 (米)
    double desired_height = 10.0;            ///< 期望高度 (米)
    double height_tolerance = 1.0;           ///< 高度容差 (米)
    double max_speed = 8.0;                  ///< 最大跟踪速度 (m/s)
    int control_frequency = 20;              ///< 控制频率 (Hz)
    double tracking_timeout = 10.0;          ///< 目标丢失超时 (秒)
    PIDParams pid_params;                    ///< PID参数
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(TrackingConfig, target_class, desired_distance, 
        distance_tolerance, desired_height, height_tolerance, max_speed, 
        control_frequency, tracking_timeout, pid_params)
};

/**
 * @brief VINS配置
 */
struct VINSConfig {
    double init_timeout = 30.0;          ///< 初始化超时 (秒)
    int required_features = 150;         ///< 所需特征点数量
    double init_height = 1.5;            ///< 初始化高度 (米)
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(VINSConfig, init_timeout, required_features, init_height)
};

/**
 * @brief GPS欺骗防护配置
 */
struct GPSDefenseConfig {
    bool enabled = true;                    ///< 是否启用
    double check_interval = 1.0;            ///< 检测间隔 (秒)
    double raim_threshold = 3.0;            ///< RAIM阈值
    double velocity_threshold = 3.0;        ///< 速度差阈值 (m/s)
    double position_threshold = 10.0;       ///< 位置差阈值 (米)
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(GPSDefenseConfig, enabled, check_interval,
        raim_threshold, velocity_threshold, position_threshold)
};

/**
 * @brief 感知配置
 */
struct PerceptionConfig {
    std::string yolo_model = "yolov8n.rknn";              ///< YOLO模型文件
    std::vector<std::string> classes = {"person"};        ///< 检测类别
    float confidence_threshold = 0.6f;                     ///< 置信度阈值
    float nms_threshold = 0.45f;                          ///< NMS阈值
    bool use_npu = true;                                  ///< 使用NPU加速
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(PerceptionConfig, yolo_model, classes,
        confidence_threshold, nms_threshold, use_npu)
};

/**
 * @brief 通信配置
 */
struct CommunicationConfig {
    double telemetry_rate = 5.0;            ///< 遥测上报频率 (Hz)
    std::string video_quality = "720p";     ///< 视频质量
    std::string video_codec = "H.265";      ///< 视频编码
    bool low_bandwidth_mode = true;         ///< 低带宽模式
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(CommunicationConfig, telemetry_rate,
        video_quality, video_codec, low_bandwidth_mode)
};

/**
 * @brief 拒止环境任务配置
 */
struct DeniedEnvMissionConfig {
    std::string mission_id;                  ///< 任务ID
    std::string mission_type = "denied_env"; ///< 任务类型
    std::string description;                 ///< 任务描述
    
    SearchConfig search;                     ///< 搜索配置
    TrackingConfig tracking;                 ///< 跟踪配置
    VINSConfig vins;                         ///< VINS配置
    GPSDefenseConfig gps_defense;            ///< GPS防护配置
    PerceptionConfig perception;             ///< 感知配置
    CommunicationConfig communication;       ///< 通信配置
    
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(DeniedEnvMissionConfig, mission_id, mission_type,
        description, search, tracking, vins, gps_defense, perception, communication)
};

/**
 * @brief Mission配置解析器
 */
class MissionConfigParser {
public:
    /**
     * @brief 从YAML文件解析
     */
    static DeniedEnvMissionConfig parseFromYAML(const std::string& yaml_path);
    
    /**
     * @brief 从JSON解析
     */
    static DeniedEnvMissionConfig parseFromJSON(const json& json_data);
    
    /**
     * @brief 验证配置有效性
     */
    static bool validate(const DeniedEnvMissionConfig& config, std::string& error_msg);
    
    /**
     * @brief 保存为YAML
     */
    static bool saveToYAML(const DeniedEnvMissionConfig& config, const std::string& yaml_path);
};

} // namespace mission
} // namespace sdk
} // namespace falconmind
