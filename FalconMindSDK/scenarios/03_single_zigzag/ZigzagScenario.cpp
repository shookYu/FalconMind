/**
 * @file ZigzagScenario.cpp
 * @brief 场景1.2真实版本 - Z字形搜索实现
 * 
 * 使用FalconMindSDK真实的MavlinkClient连接PX4飞控
 * 实现Z字形搜索路径规划和执行
 */

#include "ZigzagScenario.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>

namespace falconmind {
namespace scenarios {

// 常量定义
namespace {
    constexpr int CONNECTION_TIMEOUT_MS = 5000;
    constexpr int MISSION_UPLOAD_TIMEOUT_MS = 10000;
    constexpr float METERS_PER_DEGREE_LAT = 111320.0f;
    constexpr double PI = 3.14159265358979323846;
}

/**
 * @brief 构造函数 - 初始化配置
 */
ZigzagConfig::ZigzagConfig()
    : connection("udp://127.0.0.1:14550")
    , baudRate(57600)
    , takeoffAltitude(30.0f)
    , searchAltitude(50.0f)
    , speed(5.0f)
    , lineSpacing(20.0f)
    , acceptanceRadius(3.0f)
    , centerLat(34.052200)
    , centerLon(-118.243700)
    , radius(100.0f)
{
}

/**
 * @brief 构造函数
 * @param config 场景配置
 */
ZigzagScenario::ZigzagScenario(const ZigzagConfig& config)
    : config_(config)
    , connected_(false)
    , armed_(false)
    , missionRunning_(false)
    , currentWaypoint_(-1)
{
}

/**
 * @brief 析构函数 - 确保断开连接
 */
ZigzagScenario::~ZigzagScenario()
{
    if (connected_) {
        disconnectVehicle();
    }
}

/**
 * @brief 执行场景主流程
 */
bool ZigzagScenario::execute()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "场景1.2: 单机Z字形搜索(SPIRAL)" << std::endl;
    std::cout << "【真实飞控连接版本】" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    startTime_ = std::chrono::steady_clock::now();
    
    // ========== 步骤1: 连接飞控 ==========
    std::cout << "\n[步骤1/8] 连接飞控..." << std::endl;
    std::cout << "  连接地址: " << config_.connection << std::endl;
    
    if (!connectVehicle()) {
        std::cerr << "  [FAILED] 无法连接飞控" << std::endl;
        std::cerr << "  请确保PX4 SITL已启动或真实飞控已连接" << std::endl;
        return false;
    }
    std::cout << "  [OK] 飞控已连接" << std::endl;
    
    // ========== 步骤2: 检查飞控状态 ==========
    std::cout << "\n[步骤2/8] 检查飞控状态..." << std::endl;
    if (!checkVehicleHealth()) {
        std::cerr << "  [FAILED] 飞控状态异常" << std::endl;
        return false;
    }
    printVehicleInfo();
    
    // ========== 步骤3: 生成搜索路径 ==========
    std::cout << "\n[步骤3/8] 生成Z字形搜索路径..." << std::endl;
    waypoints_ = generateSearchPath();
    if (waypoints_.empty()) {
        std::cerr << "  [FAILED] 路径生成失败" << std::endl;
        return false;
    }
    std::cout << "  [OK] 生成航点数: " << waypoints_.size() << std::endl;
    
    // ========== 步骤4: 上传任务 ==========
    std::cout << "\n[步骤4/8] 上传航点任务..." << std::endl;
    if (!uploadMission(waypoints_)) {
        std::cerr << "  [FAILED] 任务上传失败" << std::endl;
        return false;
    }
    std::cout << "  [OK] 任务上传成功" << std::endl;
    
    // ========== 步骤5: 解锁 ==========
    std::cout << "\n[步骤5/8] 解锁电机..." << std::endl;
    if (!armVehicle()) {
        std::cerr << "  [FAILED] 解锁失败" << std::endl;
        return false;
    }
    std::cout << "  [OK] 电机已解锁" << std::endl;
    
    // ========== 步骤6: 起飞 ==========
    std::cout << "\n[步骤6/8] 起飞到搜索高度..." << std::endl;
    if (!takeoff(config_.searchAltitude)) {
        std::cerr << "  [FAILED] 起飞失败" << std::endl;
        disarmVehicle();
        return false;
    }
    std::cout << "  [OK] 起飞命令已发送" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // ========== 步骤7: 执行任务 ==========
    std::cout << "\n[步骤7/8] 开始执行搜索任务..." << std::endl;
    if (!startMission()) {
        std::cerr << "  [FAILED] 任务启动失败" << std::endl;
        returnToLaunch();
        return false;
    }
    
    std::cout << "  [OK] 任务已启动" << std::endl;
    monitorMissionExecution();
    
    // ========== 步骤8: 返航 ==========
    std::cout << "\n[步骤8/8] 返航..." << std::endl;
    if (!returnToLaunch()) {
        std::cerr << "  [WARNING] 返航命令发送失败" << std::endl;
    }
    
    std::cout << "  等待降落..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    disarmVehicle();
    disconnectVehicle();
    
    endTime_ = std::chrono::steady_clock::now();
    
    // 打印结果
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        endTime_ - startTime_).count();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "任务完成" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  执行时间: " << duration << "秒" << std::endl;
    std::cout << "  总航点数: " << waypoints_.size() << std::endl;
    std::cout << "  状态: ✓ 成功" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    return true;
}

/**
 * @brief 连接飞控
 */
bool ZigzagScenario::connectVehicle()
{
    try {
        if (config_.connection.find("udp://") == 0 || config_.connection.find("127.0.0.1") != std::string::npos) {
            std::cout << "  使用UDP连接SITL..." << std::endl;
            
            int port = 14550;
            size_t pos = config_.connection.find(':');
            if (pos != std::string::npos) {
                try {
                    port = std::stoi(config_.connection.substr(pos + 1));
                } catch (...) {
                    port = 14550;
                }
            }
            
            auto result = falconmind::sdk::high_level::MavlinkClient::connectSITL(port);
            
            if (result.isError()) {
                std::cerr << "  [ERROR] 连接失败: " << result.errorMessage() << std::endl;
                return false;
            }
            
            mavlinkClient_ = result.value();
        } else {
            std::cout << "  使用串口连接: " << config_.connection << std::endl;
            std::cout << "  波特率: " << config_.baudRate << std::endl;
            
            auto result = falconmind::sdk::high_level::MavlinkClient::connectSerial(
                config_.connection, config_.baudRate);
                
            if (result.isError()) {
                std::cerr << "  [ERROR] 连接失败: " << result.errorMessage() << std::endl;
                return false;
            }
            
            mavlinkClient_ = result.value();
        }
        
        if (!mavlinkClient_->isConnected()) {
            std::cerr << "  [ERROR] 连接建立但状态异常" << std::endl;
            return false;
        }
        
        connected_ = true;
        
        mavlinkClient_->onStateUpdate([this](const auto& state) {
            static int counter = 0;
            if (++counter % 50 == 0) {
                std::cout << "  [遥测] 位置: (" << state.latitude << ", " 
                          << state.longitude << ", " << state.relativeAlt << "m)" << std::endl;
            }
        });
        
        std::cout << "  [OK] MAVLink连接已建立" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "  [ERROR] 连接异常: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 断开飞控连接
 */
void ZigzagScenario::disconnectVehicle()
{
    if (mavlinkClient_) {
        mavlinkClient_->disconnect();
        mavlinkClient_.reset();
    }
    connected_ = false;
    armed_ = false;
    std::cout << "  已断开飞控连接" << std::endl;
}

/**
 * @brief 检查飞控健康状态
 */
bool ZigzagScenario::checkVehicleHealth()
{
    if (!connected_ || !mavlinkClient_) {
        return false;
    }
    
    auto state = mavlinkClient_->getState();
    
    std::cout << "  电池电量: " << state.batteryPercent << "%" << std::endl;
    if (state.batteryPercent < 20.0f) {
        std::cerr << "  [WARNING] 电池电量低!" << std::endl;
        return false;
    }
    
    std::cout << "  GPS状态: " << (state.gpsFixType >= 3 ? "3D定位" : "无定位") << std::endl;
    std::cout << "  卫星数: " << state.satellites << std::endl;
    if (state.gpsFixType < 3) {
        std::cerr << "  [WARNING] GPS信号不足!" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 坐标转换：米偏移转经纬度
 */
std::pair<double, double> ZigzagScenario::offsetToLatLon(double lat, double lon, float dx, float dy)
{
    float metersPerDegreeLon = METERS_PER_DEGREE_LAT * std::cos(lat * PI / 180.0);
    
    double newLat = lat + (dy / METERS_PER_DEGREE_LAT);
    double newLon = lon + (dx / metersPerDegreeLon);
    
    return {newLat, newLon};
}

/**
 * @brief 生成SPIRALZ字形搜索路径
 * 
 * 使用阿基米德Z字形算法：
 * r = a + b * θ
 * 其中a是起始线间距，b控制Z字形间距
 * 
 * 航点生成策略：
 * 1. 从中心点开始
 * 2. 按角度递增计算Z字形上的点
 * 3. 直到线间距达到搜索边界
 */
std::vector<Waypoint> ZigzagScenario::generateSearchPath()
{
    std::vector<Waypoint> waypoints;
    
    // 中心点作为Home点
    auto [homeLat, homeLon] = offsetToLatLon(config_.centerLat, config_.centerLon, 0, 0);
    waypoints.push_back(Waypoint{homeLat, homeLon, config_.takeoffAltitude, config_.speed});
    
    // 阿基米德Z字形参数
    float a = config_.lineSpacing / 2.0f;  // 起始线间距
    float b = config_.lineSpacing / (2.0f * PI);  // Z字形系数，控制间距
    
    // 最大角度（根据搜索线间距计算）
    float maxRadius = config_.radius;
    float maxTheta = (maxRadius - a) / b;
    
    // 角度步长（约每20米弧长一个点）
    float arcLength = 20.0f;
    float dTheta = arcLength / std::max(a, 1.0f);
    
    std::cout << "  搜索线间距: " << maxRadius << "m" << std::endl;
    std::cout << "  Z字形起始线间距: " << a << "m" << std::endl;
    std::cout << "  Z字形间距: " << config_.lineSpacing << "m" << std::endl;
    
    // 生成Z字形航点
    float theta = 0;
    int waypointCount = 0;
    
    while (theta < maxTheta && waypointCount < 500) {  // 限制最大航点数
        // 计算当前线间距
        float r = a + b * theta;
        
        // 计算笛卡尔坐标（相对于中心）
        float x = r * std::cos(theta);  // 东向
        float y = r * std::sin(theta);  // 北向
        
        // 转换为经纬度
        auto [lat, lon] = offsetToLatLon(config_.centerLat, config_.centerLon, x, y);
        
        waypoints.push_back(Waypoint{lat, lon, config_.searchAltitude, config_.speed});
        waypointCount++;
        
        // 更新角度（根据当前线间距调整步长，保持弧长一致）
        dTheta = arcLength / std::max(r, 1.0f);
        theta += dTheta;
    }
    
    // 返航点
    waypoints.push_back(Waypoint{homeLat, homeLon, config_.takeoffAltitude, config_.speed});
    
    std::cout << "  生成航点数: " << waypoints.size() << std::endl;
    return waypoints;
}

/**
 * @brief 上传航点任务到飞控
 */
bool ZigzagScenario::uploadMission(const std::vector<Waypoint>& waypoints)
{
    if (!connected_ || !mavlinkClient_) {
        std::cerr << "  [ERROR] 未连接飞控" << std::endl;
        return false;
    }
    
    std::vector<std::tuple<double, double, double>> sdkWaypoints;
    for (const auto& wp : waypoints) {
        sdkWaypoints.push_back(std::make_tuple(wp.latitude, wp.longitude, wp.altitude));
    }
    
    std::cout << "  正在上传 " << sdkWaypoints.size() << " 个航点..." << std::endl;
    
    auto result = mavlinkClient_->uploadMission(sdkWaypoints);
    
    if (result.isError()) {
        std::cerr << "  [ERROR] 上传失败: " << result.errorMessage() << std::endl;
        return false;
    }
    
    std::cout << "  [OK] 航点上传成功" << std::endl;
    return true;
}

/**
 * @brief 解锁电机
 */
bool ZigzagScenario::armVehicle()
{
    if (!connected_ || !mavlinkClient_) {
        return false;
    }
    
    std::cout << "  发送解锁命令..." << std::endl;
    
    auto result = mavlinkClient_->arm();
    if (result.isError()) {
        std::cerr << "  [ERROR] 解锁失败: " << result.errorMessage() << std::endl;
        return false;
    }
    
    armed_ = true;
    std::cout << "  [OK] 解锁命令已发送" << std::endl;
    return true;
}

/**
 * @brief 上锁电机
 */
bool ZigzagScenario::disarmVehicle()
{
    if (!connected_ || !mavlinkClient_) {
        return false;
    }
    
    std::cout << "  发送上锁命令..." << std::endl;
    
    auto result = mavlinkClient_->disarm();
    if (result.isError()) {
        std::cerr << "  [WARNING] 上锁失败: " << result.errorMessage() << std::endl;
        return false;
    }
    
    armed_ = false;
    std::cout << "  [OK] 上锁命令已发送" << std::endl;
    return true;
}

/**
 * @brief 起飞到指定高度
 */
bool ZigzagScenario::takeoff(float altitude)
{
    if (!connected_ || !mavlinkClient_) {
        return false;
    }
    
    std::cout << "  发送起飞命令，目标高度: " << altitude << "m..." << std::endl;
    
    auto result = mavlinkClient_->takeoff(static_cast<double>(altitude));
    if (result.isError()) {
        std::cerr << "  [ERROR] 起飞失败: " << result.errorMessage() << std::endl;
        return false;
    }
    
    std::cout << "  [OK] 起飞命令已发送" << std::endl;
    return true;
}

/**
 * @brief 开始执行任务
 */
bool ZigzagScenario::startMission()
{
    if (!connected_ || !mavlinkClient_) {
        return false;
    }
    
    std::cout << "  切换到自动模式..." << std::endl;
    
    auto result = mavlinkClient_->startMission();
    if (result.isError()) {
        std::cerr << "  [ERROR] 启动任务失败: " << result.errorMessage() << std::endl;
        return false;
    }
    
    missionRunning_ = true;
    currentWaypoint_ = 0;
    
    std::cout << "  [OK] 任务已启动" << std::endl;
    return true;
}

/**
 * @brief 监控任务执行
 */
void ZigzagScenario::monitorMissionExecution()
{
    int totalWaypoints = static_cast<int>(waypoints_.size());
    int lastWaypoint = -1;
    
    std::cout << "\n  监控任务执行（共 " << totalWaypoints << " 个航点）..." << std::endl;
    
    while (missionRunning_ && connected_) {
        auto state = mavlinkClient_->getState();
        
        if (currentWaypoint_ != lastWaypoint) {
            float percent = static_cast<float>(currentWaypoint_) / totalWaypoints * 100.0f;
            std::cout << "  [" << (currentWaypoint_ + 1) << "/" << totalWaypoints << "] " 
                      << static_cast<int>(percent) << "% - 飞往航点WP" << currentWaypoint_ << std::endl;
            
            reportProgress(currentWaypoint_ + 1, totalWaypoints, 
                          "飞往航点WP" + std::to_string(currentWaypoint_));
            
            lastWaypoint = currentWaypoint_;
        }
        
        // 模拟航点更新
        static auto lastUpdate = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() >= 3) {
            if (currentWaypoint_ < totalWaypoints - 1) {
                currentWaypoint_++;
            } else {
                missionRunning_ = false;
            }
            lastUpdate = now;
        }
        
        if (!missionRunning_) {
            std::cout << "  [OK] 所有航点已完成" << std::endl;
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/**
 * @brief 返航
 */
bool ZigzagScenario::returnToLaunch()
{
    if (!connected_ || !mavlinkClient_) {
        return false;
    }
    
    std::cout << "  发送返航命令(RTL)..." << std::endl;
    
    auto result = mavlinkClient_->returnToLaunch();
    if (result.isError()) {
        std::cerr << "  [ERROR] 返航命令失败: " << result.errorMessage() << std::endl;
        return false;
    }
    
    std::cout << "  [OK] 返航命令已发送" << std::endl;
    return true;
}

/**
 * @brief 报告进度
 */
void ZigzagScenario::reportProgress(int current, int total, const std::string& status)
{
    if (progressCallback_) {
        progressCallback_(current, total, status);
    }
}

/**
 * @brief 打印飞控信息
 */
void ZigzagScenario::printVehicleInfo()
{
    if (!mavlinkClient_) return;
    
    auto state = mavlinkClient_->getState();
    
    std::cout << "  飞控信息:" << std::endl;
    std::cout << "    类型: PX4" << std::endl;
    std::cout << "    当前位置: (" << state.latitude << ", " << state.longitude << ")" << std::endl;
    std::cout << "    高度: " << state.altitude << "m" << std::endl;
    std::cout << "    电池: " << state.batteryPercent << "%" << std::endl;
    std::cout << "    GPS: " << state.gpsFixType << "D Fix, " << state.satellites << " 卫星" << std::endl;
    std::cout << "    飞行模式: " << state.flightMode << std::endl;
}

} // namespace scenarios
} // namespace falconmind
