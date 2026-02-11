// FalconMindSDK - Search Path Planner Node Implementation
#include "falconmind/sdk/mission/SearchPathPlannerNode.h"
#include "falconmind/sdk/core/Pad.h"
#include "falconmind/sdk/core/Caps.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace falconmind::sdk::mission {

SearchPathPlannerNode::SearchPathPlannerNode() : core::Node("search_path_planner") {
    // 添加输出端口：航点列表（使用 Source 类型表示输出）
    addPad(std::make_shared<core::Pad>("waypoints", core::PadType::Source));
}

bool SearchPathPlannerNode::configure(const std::unordered_map<std::string, std::string>& params) {
    core::Node::configure(params);
    configured_ = true;
    return true;
}

bool SearchPathPlannerNode::start() {
    core::Node::start();
    // 生成路径
    switch (searchParams_.pattern) {
        case SearchPattern::LAWN_MOWER:
            generateLawnMowerPath();
            break;
        case SearchPattern::SPIRAL:
            generateSpiralPath();
            break;
        case SearchPattern::ZIGZAG:
            generateZigzagPath();
            break;
        case SearchPattern::SECTOR:
            generateSectorPath();
            break;
        case SearchPattern::WAYPOINT_LIST:
            // WAYPOINT_LIST 模式直接使用提供的航点，不需要生成
            break;
    }
    // 优化路径：移除重复点、优化航点顺序
    optimizePath();
    return true;
}

void SearchPathPlannerNode::stop() {
    core::Node::stop();
}

void SearchPathPlannerNode::process() {
    // 路径规划节点通常在启动时生成路径，process 中不需要重复处理
    // 如果需要，可以在这里输出航点数据
}

void SearchPathPlannerNode::setSearchArea(const SearchArea& area) {
    searchArea_ = area;
}

void SearchPathPlannerNode::setSearchParams(const SearchParams& params) {
    searchParams_ = params;
}

void SearchPathPlannerNode::computeBoundingBox(const std::vector<GeoPoint>& polygon,
                                              double& minLat, double& maxLat,
                                              double& minLon, double& maxLon) {
    if (polygon.empty()) {
        minLat = maxLat = minLon = maxLon = 0.0;
        return;
    }
    
    minLat = maxLat = polygon[0].lat;
    minLon = maxLon = polygon[0].lon;
    
    for (const auto& point : polygon) {
        minLat = std::min(minLat, point.lat);
        maxLat = std::max(maxLat, point.lat);
        minLon = std::min(minLon, point.lon);
        maxLon = std::max(maxLon, point.lon);
    }
}

void SearchPathPlannerNode::generateLawnMowerPath() {
    waypoints_.clear();
    
    if (searchArea_.polygon.size() < 3) {
        return; // 无效的多边形
    }
    
    // 计算边界框
    double minLat, maxLat, minLon, maxLon;
    computeBoundingBox(searchArea_.polygon, minLat, maxLat, minLon, maxLon);
    
    // 优化的网格搜索：使用多边形裁剪，只保留在多边形内的航点
    const double spacing = searchParams_.spacing > 0 ? searchParams_.spacing : 50.0; // 默认50米
    
    // 将间距从米转换为度数（近似）
    // 1度纬度 ≈ 111km，1度经度 ≈ 111km * cos(纬度)
    const double latCenter = (minLat + maxLat) / 2.0;
    const double latSpacing = spacing / 111000.0; // 转换为度数
    const double lonSpacing = spacing / (111000.0 * std::cos(latCenter * M_PI / 180.0));
    
    // 生成网格点
    std::vector<GeoPoint> gridPoints = generateGridPoints(minLat, maxLat, minLon, maxLon, latSpacing, lonSpacing);
    
    // 按行组织网格点，生成蛇形路径
    bool leftToRight = true;
    double currentLat = minLat;
    std::vector<GeoPoint> rowPoints;
    
    while (currentLat <= maxLat) {
        rowPoints.clear();
        // 收集当前行的点
        for (const auto& point : gridPoints) {
            if (std::abs(point.lat - currentLat) < latSpacing / 2.0) {
                if (isPointInPolygon(point, searchArea_.polygon)) {
                    rowPoints.push_back(point);
                }
            }
        }
        
        // 按经度排序
        std::sort(rowPoints.begin(), rowPoints.end(),
                  [](const GeoPoint& a, const GeoPoint& b) { return a.lon < b.lon; });
        
        // 根据方向添加航点
        if (!rowPoints.empty()) {
            if (leftToRight) {
                waypoints_.insert(waypoints_.end(), rowPoints.begin(), rowPoints.end());
            } else {
                waypoints_.insert(waypoints_.end(), rowPoints.rbegin(), rowPoints.rend());
            }
        }
        
        leftToRight = !leftToRight;
        currentLat += latSpacing;
    }
}

void SearchPathPlannerNode::generateSpiralPath() {
    waypoints_.clear();
    
    if (searchArea_.polygon.size() < 3) {
        return;
    }
    
    // 计算边界框和中心点
    double minLat, maxLat, minLon, maxLon;
    computeBoundingBox(searchArea_.polygon, minLat, maxLat, minLon, maxLon);
    
    const double centerLat = (minLat + maxLat) / 2.0;
    const double centerLon = (minLon + maxLon) / 2.0;
    const double radiusLat = (maxLat - minLat) / 2.0;
    const double radiusLon = (maxLon - minLon) / 2.0;
    const double maxRadius = std::max(radiusLat, radiusLon);
    
    // 生成螺旋路径（优化：使用多边形裁剪）
    const double spacing = searchParams_.spacing > 0 ? searchParams_.spacing : 50.0;
    const double latSpacing = spacing / 111000.0;
    const int numTurns = static_cast<int>(maxRadius / latSpacing);
    
    for (int i = 0; i <= numTurns * 16; ++i) {
        const double angle = i * M_PI / 8.0; // 每22.5度一个点，更平滑
        const double radius = (i / 16.0) * latSpacing;
        
        if (radius > maxRadius) {
            break;
        }
        
        const double lat = centerLat + radius * std::cos(angle);
        const double lon = centerLon + radius * std::sin(angle);
        
        GeoPoint point = {lat, lon, searchParams_.altitude};
        // 使用多边形裁剪，只保留在多边形内的点
        if (isPointInPolygon(point, searchArea_.polygon)) {
            waypoints_.push_back(point);
        }
    }
}

void SearchPathPlannerNode::generateZigzagPath() {
    waypoints_.clear();
    
    if (searchArea_.polygon.size() < 3) {
        return;
    }
    
    // 计算边界框
    double minLat, maxLat, minLon, maxLon;
    computeBoundingBox(searchArea_.polygon, minLat, maxLat, minLon, maxLon);
    
    const double spacing = searchParams_.spacing > 0 ? searchParams_.spacing : 50.0;
    const double latCenter = (minLat + maxLat) / 2.0;
    const double latSpacing = spacing / 111000.0;
    const double lonSpacing = spacing / (111000.0 * std::cos(latCenter * M_PI / 180.0));
    
    // Z字形路径：对角线方向
    bool up = true;
    double currentLat = minLat;
    double currentLon = minLon;
    
    while (currentLat <= maxLat && currentLon <= maxLon) {
        GeoPoint point = {currentLat, currentLon, searchParams_.altitude};
        if (isPointInPolygon(point, searchArea_.polygon)) {
            waypoints_.push_back(point);
        }
        
        if (up) {
            currentLat += latSpacing;
            currentLon += lonSpacing;
            if (currentLon > maxLon) {
                currentLon = maxLon;
                up = false;
            }
        } else {
            currentLat += latSpacing;
            currentLon -= lonSpacing;
            if (currentLon < minLon) {
                currentLon = minLon;
                up = true;
            }
        }
    }
}

void SearchPathPlannerNode::generateSectorPath() {
    waypoints_.clear();
    
    if (searchArea_.polygon.size() < 3) {
        return;
    }
    
    // 计算边界框和中心点
    double minLat, maxLat, minLon, maxLon;
    computeBoundingBox(searchArea_.polygon, minLat, maxLat, minLon, maxLon);
    
    const double centerLat = (minLat + maxLat) / 2.0;
    const double centerLon = (minLon + maxLon) / 2.0;
    const double maxRadius = std::max((maxLat - minLat) / 2.0, (maxLon - minLon) / 2.0);
    
    const double spacing = searchParams_.spacing > 0 ? searchParams_.spacing : 50.0;
    const double latSpacing = spacing / 111000.0;
    const int numSectors = 8; // 8个扇形
    const int pointsPerSector = 10;
    
    // 从中心向外生成扇形路径
    for (int sector = 0; sector < numSectors; ++sector) {
        const double startAngle = sector * 2.0 * M_PI / numSectors;
        const double endAngle = (sector + 1) * 2.0 * M_PI / numSectors;
        
        for (int i = 0; i <= pointsPerSector; ++i) {
            const double radius = (i / static_cast<double>(pointsPerSector)) * maxRadius;
            const double angle = startAngle + (endAngle - startAngle) * (i / static_cast<double>(pointsPerSector));
            
            const double lat = centerLat + radius * std::cos(angle);
            const double lon = centerLon + radius * std::sin(angle);
            
            GeoPoint point = {lat, lon, searchParams_.altitude};
            if (isPointInPolygon(point, searchArea_.polygon)) {
                waypoints_.push_back(point);
            }
        }
    }
}

bool SearchPathPlannerNode::isPointInPolygon(const GeoPoint& point, const std::vector<GeoPoint>& polygon) {
    if (polygon.size() < 3) {
        return false;
    }
    
    // 射线法：从点向右发射一条射线，计算与多边形边的交点数量
    // 奇数个交点表示点在多边形内
    int intersections = 0;
    const size_t n = polygon.size();
    
    for (size_t i = 0; i < n; ++i) {
        const GeoPoint& p1 = polygon[i];
        const GeoPoint& p2 = polygon[(i + 1) % n];
        
        // 检查射线是否与边相交
        if (((p1.lat > point.lat) != (p2.lat > point.lat)) &&
            (point.lon < (p2.lon - p1.lon) * (point.lat - p1.lat) / (p2.lat - p1.lat) + p1.lon)) {
            intersections++;
        }
    }
    
    return (intersections % 2) == 1;
}

double SearchPathPlannerNode::calculateDistance(const GeoPoint& p1, const GeoPoint& p2) {
    // 使用 Haversine 公式计算两点间的大圆距离
    const double R = 6371000.0; // 地球半径（米）
    const double dLat = (p2.lat - p1.lat) * M_PI / 180.0;
    const double dLon = (p2.lon - p1.lon) * M_PI / 180.0;
    const double a = std::sin(dLat / 2.0) * std::sin(dLat / 2.0) +
                    std::cos(p1.lat * M_PI / 180.0) * std::cos(p2.lat * M_PI / 180.0) *
                    std::sin(dLon / 2.0) * std::sin(dLon / 2.0);
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    return R * c;
}

void SearchPathPlannerNode::optimizePath() {
    if (waypoints_.size() < 2) {
        return;
    }
    
    // 移除重复点和距离过近的点
    const double minDistance = 5.0; // 最小距离5米
    std::vector<GeoPoint> optimized;
    optimized.push_back(waypoints_[0]);
    
    for (size_t i = 1; i < waypoints_.size(); ++i) {
        const double dist = calculateDistance(optimized.back(), waypoints_[i]);
        if (dist >= minDistance) {
            optimized.push_back(waypoints_[i]);
        }
    }
    
    waypoints_ = optimized;
}

std::vector<GeoPoint> SearchPathPlannerNode::generateGridPoints(double minLat, double maxLat,
                                                                double minLon, double maxLon,
                                                                double latSpacing, double lonSpacing) {
    std::vector<GeoPoint> points;
    
    for (double lat = minLat; lat <= maxLat; lat += latSpacing) {
        for (double lon = minLon; lon <= maxLon; lon += lonSpacing) {
            points.push_back({lat, lon, searchParams_.altitude});
        }
    }
    
    return points;
}

} // namespace falconmind::sdk::mission
