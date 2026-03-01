/**
 * @file LawnMowerScenario.cpp
 * @brief 场景1.1: 单机网格搜索(LAWN_MOWER) - 实现文件
 * 
 * @author FalconMind SDK Team
 * @date 2026-02-27
 * @version 1.0.0
 */

#include "LawnMowerScenario.h"
#include <iostream>
#include <cmath>
#include <chrono>
#include <thread>
#include <algorithm>

namespace falconmind {
namespace scenarios {

// 常量定义
namespace {
    const double METERS_PER_DEGREE_LAT = 111320.0;  ///< 每度纬度对应的米数
}

/**
 * @brief ScenarioConfig默认构造函数
 */
ScenarioConfig::ScenarioConfig()
    : connection("udp://127.0.0.1:14550")
    , baudRate(57600)
    , takeoffAltitude(30.0f)
    , searchAltitude(50.0f)
    , speed(5.0f)
    , lineSpacing(30.0f)
    , searchArea()
{
    // 默认搜索区域：约100m x 60m的矩形
    searchArea.push_back(GeoPoint(34.052200, -118.243700, 30.0f));
    searchArea.push_back(GeoPoint(34.052200, -118.243000, 30.0f));
    searchArea.push_back(GeoPoint(34.052800, -118.243000, 30.0f));
    searchArea.push_back(GeoPoint(34.052800, -118.243700, 30.0f));
}

/**
 * @brief LawnMowerScenario构造函数
 */
LawnMowerScenario::LawnMowerScenario(const ScenarioConfig& config)
    : config_(config)
    , searchArea_(0.0)
    , executed_(false)
{
    // 计算搜索区域面积
    searchArea_ = calculatePolygonArea(config_.searchArea);
}

/**
 * @brief LawnMowerScenario析构函数
 */
LawnMowerScenario::~LawnMowerScenario()
{
    // 清理资源（如果需要）
}

/**
 * @brief 执行场景
 */
bool LawnMowerScenario::execute()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "场景1.1: 单机网格搜索(LAWN_MOWER)" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // 步骤1: 打印配置信息
    printConfiguration();
    
    // 步骤2: 生成搜索路径
    std::cout << "\n[步骤1] 生成网格搜索路径..." << std::endl;
    waypoints_ = generateLawnMowerPath(
        config_.searchArea, 
        config_.searchAltitude, 
        config_.lineSpacing
    );
    
    if (waypoints_.empty()) {
        std::cerr << "  [ERROR] 路径生成失败：航点列表为空" << std::endl;
        return false;
    }
    
    std::cout << "  [OK] 生成航点数: " << waypoints_.size() << std::endl;
    
    // 显示路径预览
    std::cout << "\n  路径预览 (前5个航点):" << std::endl;
    size_t previewCount = std::min(size_t(5), waypoints_.size());
    for (size_t i = 0; i < previewCount; ++i) {
        std::cout << "    WP" << i << ": (" 
                  << waypoints_[i].latitude << ", " 
                  << waypoints_[i].longitude << ", " 
                  << waypoints_[i].altitude << "m)" << std::endl;
    }
    
    if (waypoints_.size() > 5) {
        std::cout << "    ... 共" << waypoints_.size() << "个航点" << std::endl;
    }
    
    // 步骤3: 模拟执行任务
    std::cout << "\n[步骤2] 执行搜索任务..." << std::endl;
    
    for (size_t i = 0; i < waypoints_.size(); ++i) {
        float percent = static_cast<float>(i + 1) / waypoints_.size() * 100.0f;
        
        std::cout << "  进度: [" << (i + 1) << "/" << waypoints_.size() 
                  << "] " << static_cast<int>(percent) << "% - 飞往航点(" 
                  << waypoints_[i].latitude << ", " 
                  << waypoints_[i].longitude << ")" << std::endl;
        
        // 模拟飞行时间
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    
    std::cout << "\n[OK] 搜索任务完成" << std::endl;
    
    // 步骤4: 打印执行结果
    printResults();
    
    executed_ = true;
    return true;
}

/**
 * @brief 打印配置信息
 */
void LawnMowerScenario::printConfiguration() const
{
    std::cout << "【配置信息】" << std::endl;
    std::cout << "  飞控连接: " << config_.connection << std::endl;
    std::cout << "  波特率: " << config_.baudRate << std::endl;
    std::cout << "  搜索区域: " << config_.searchArea.size() << "个顶点" << std::endl;
    std::cout << "  起飞高度: " << config_.takeoffAltitude << "m" << std::endl;
    std::cout << "  搜索高度: " << config_.searchAltitude << "m" << std::endl;
    std::cout << "  飞行速度: " << config_.speed << "m/s" << std::endl;
    std::cout << "  线间距: " << config_.lineSpacing << "m" << std::endl;
    std::cout << "  区域面积: " << searchArea_ << " m²" << std::endl;
    
    // 计算区域尺寸
    if (config_.searchArea.size() >= 4) {
        double minLat = config_.searchArea[0].latitude;
        double maxLat = config_.searchArea[0].latitude;
        double minLon = config_.searchArea[0].longitude;
        double maxLon = config_.searchArea[0].longitude;
        
        for (const auto& p : config_.searchArea) {
            minLat = std::min(minLat, p.latitude);
            maxLat = std::max(maxLat, p.latitude);
            minLon = std::min(minLon, p.longitude);
            maxLon = std::max(maxLon, p.longitude);
        }
        
        double metersPerDegreeLon = METERS_PER_DEGREE_LAT * 
            std::cos((minLat + maxLat) / 2.0 * M_PI / 180.0);
        
        double widthMeters = (maxLon - minLon) * metersPerDegreeLon;
        double heightMeters = (maxLat - minLat) * METERS_PER_DEGREE_LAT;
        
        std::cout << "  区域尺寸: " << widthMeters << "m x " 
                  << heightMeters << "m" << std::endl;
    }
}

/**
 * @brief 生成LAWN_MOWER搜索路径
 */
std::vector<GeoPoint> LawnMowerScenario::generateLawnMowerPath(
    const std::vector<GeoPoint>& area, 
    float altitude, 
    float spacing)
{
    std::vector<GeoPoint> waypoints;
    
    // 验证输入
    if (area.size() < 4) {
        std::cerr << "  [ERROR] 搜索区域至少需要4个顶点" << std::endl;
        return waypoints;
    }
    
    if (spacing <= 0) {
        std::cerr << "  [ERROR] 线间距必须大于0" << std::endl;
        return waypoints;
    }
    
    // 计算边界
    double minLat = area[0].latitude;
    double maxLat = area[0].latitude;
    double minLon = area[0].longitude;
    double maxLon = area[0].longitude;
    
    for (const auto& p : area) {
        minLat = std::min(minLat, p.latitude);
        maxLat = std::max(maxLat, p.latitude);
        minLon = std::min(minLon, p.longitude);
        maxLon = std::max(maxLon, p.longitude);
    }
    
    // 转换为米
    double metersPerDegreeLon = METERS_PER_DEGREE_LAT * 
        std::cos((minLat + maxLat) / 2.0 * M_PI / 180.0);
    
    double widthMeters = (maxLon - minLon) * metersPerDegreeLon;
    
    // 计算需要的航线数
    int numLines = static_cast<int>(widthMeters / spacing) + 1;
    
    std::cout << "  区域宽度: " << widthMeters << "m" << std::endl;
    std::cout << "  航线数量: " << numLines << std::endl;
    
    bool goingUp = true;
    
    // 起飞点
    waypoints.push_back(GeoPoint(minLat, minLon, config_.takeoffAltitude));
    
    // 生成航线
    for (int i = 0; i < numLines; ++i) {
        double lon = minLon + (i * spacing / metersPerDegreeLon);
        if (lon > maxLon) {
            lon = maxLon;
        }
        
        if (goingUp) {
            waypoints.push_back(GeoPoint(minLat, lon, altitude));
            waypoints.push_back(GeoPoint(maxLat, lon, altitude));
        } else {
            waypoints.push_back(GeoPoint(maxLat, lon, altitude));
            waypoints.push_back(GeoPoint(minLat, lon, altitude));
        }
        goingUp = !goingUp;
    }
    
    // 返航点
    waypoints.push_back(GeoPoint(minLat, minLon, config_.takeoffAltitude));
    
    return waypoints;
}

/**
 * @brief 计算多边形面积（Shoelace公式）
 */
double LawnMowerScenario::calculatePolygonArea(
    const std::vector<GeoPoint>& points) const
{
    if (points.size() < 3) {
        return 0.0;
    }
    
    double area = 0.0;
    
    for (size_t i = 0; i < points.size(); ++i) {
        const GeoPoint& p1 = points[i];
        const GeoPoint& p2 = points[(i + 1) % points.size()];
        
        // 转换为平面坐标（近似）
        double x1 = p1.longitude * METERS_PER_DEGREE_LAT * 
                    std::cos(p1.latitude * M_PI / 180.0);
        double y1 = p1.latitude * METERS_PER_DEGREE_LAT;
        double x2 = p2.longitude * METERS_PER_DEGREE_LAT * 
                    std::cos(p2.latitude * M_PI / 180.0);
        double y2 = p2.latitude * METERS_PER_DEGREE_LAT;
        
        area += (x1 * y2 - x2 * y1);
    }
    
    return std::abs(area) / 2.0;
}

/**
 * @brief 打印执行结果
 */
void LawnMowerScenario::printResults() const
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "执行结果" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  状态: ✓ 成功" << std::endl;
    std::cout << "  总航点数: " << waypoints_.size() << std::endl;
    std::cout << "  覆盖面积: " << searchArea_ << " m²" << std::endl;
    std::cout << "========================================\n" << std::endl;
}

} // namespace scenarios
} // namespace falconmind
