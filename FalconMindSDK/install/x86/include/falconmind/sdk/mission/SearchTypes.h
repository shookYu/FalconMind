// FalconMindSDK - Search Mission Types
#pragma once

#include <vector>
#include <string>

namespace falconmind::sdk::mission {

// 搜索模式
enum class SearchPattern {
    LAWN_MOWER,      // 网格搜索（蛇形）
    SPIRAL,          // 螺旋搜索
    ZIGZAG,          // Z字形搜索
    SECTOR,          // 扇形搜索
    WAYPOINT_LIST    // 航点列表
};

// 地理坐标点（经纬度）
struct GeoPoint {
    double lat;  // 纬度（度）
    double lon;  // 经度（度）
    double alt;  // 高度（米）
};

// 搜索区域（多边形）
struct SearchArea {
    std::vector<GeoPoint> polygon;  // 多边形顶点（至少3个点）
    double minAltitude;             // 最小飞行高度（米）
    double maxAltitude;             // 最大飞行高度（米）
};

// 搜索参数
struct SearchParams {
    SearchPattern pattern;          // 搜索模式
    double altitude;                // 飞行高度（米）
    double speed;                   // 飞行速度（m/s）
    double spacing;                 // 搜索线间距（米，用于网格搜索）
    double loiterTime;              // 每个航点悬停时间（秒）
    bool enableDetection;           // 是否启用目标检测
    std::vector<std::string> detectionClasses; // 关注的检测类别
};

// 搜索进度
struct SearchProgress {
    double coveragePercent;         // 已覆盖区域百分比（0.0 - 1.0）
    int waypointIndex;              // 当前航点索引
    int totalWaypoints;             // 总航点数
    GeoPoint currentPosition;       // 当前位置
};

// 搜索事件类型
enum class SearchEventType {
    TARGET_DETECTED,    // 目标检测
    INTEREST_POINT,     // 兴趣点
    ANOMALY,            // 异常
    WAYPOINT_REACHED,   // 到达航点
    SEARCH_COMPLETE     // 搜索完成
};

// 搜索事件
struct SearchEvent {
    SearchEventType type;
    std::string description;
    GeoPoint position;
    int64_t timestampNs;           // 时间戳（纳秒）
    std::string metadata;           // 额外元数据（JSON 字符串）
};

} // namespace falconmind::sdk::mission
