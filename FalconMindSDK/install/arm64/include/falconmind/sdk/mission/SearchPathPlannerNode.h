// FalconMindSDK - Search Path Planner Node
#pragma once

#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/mission/SearchTypes.h"

#include <vector>
#include <memory>

namespace falconmind::sdk::mission {

/**
 * 搜索路径规划节点
 * 根据搜索区域和参数生成搜索路径（航点列表）
 */
class SearchPathPlannerNode : public core::Node {
public:
    SearchPathPlannerNode();
    ~SearchPathPlannerNode() override = default;

    // 配置搜索区域和参数
    void setSearchArea(const SearchArea& area);
    void setSearchParams(const SearchParams& params);

    // 获取生成的航点列表
    const std::vector<GeoPoint>& getWaypoints() const { return waypoints_; }

    // Node 接口实现
    bool configure(const std::unordered_map<std::string, std::string>& params) override;
    bool start() override;
    void stop() override;
    void process() override;

private:
    // 生成网格搜索路径（蛇形）
    void generateLawnMowerPath();
    
    // 生成螺旋搜索路径
    void generateSpiralPath();
    
    // 生成Z字形搜索路径
    void generateZigzagPath();
    
    // 生成扇形搜索路径
    void generateSectorPath();
    
    // 计算多边形的边界框
    void computeBoundingBox(const std::vector<GeoPoint>& polygon,
                           double& minLat, double& maxLat,
                           double& minLon, double& maxLon);
    
    // 检查点是否在多边形内（射线法）
    bool isPointInPolygon(const GeoPoint& point, const std::vector<GeoPoint>& polygon);
    
    // 计算两点之间的距离（米）
    double calculateDistance(const GeoPoint& p1, const GeoPoint& p2);
    
    // 优化路径：移除重复点、优化航点顺序
    void optimizePath();
    
    // 在多边形内生成网格点
    std::vector<GeoPoint> generateGridPoints(double minLat, double maxLat,
                                             double minLon, double maxLon,
                                             double latSpacing, double lonSpacing);

    SearchArea searchArea_;
    SearchParams searchParams_;
    std::vector<GeoPoint> waypoints_;
    bool configured_{false};
};

} // namespace falconmind::sdk::mission
