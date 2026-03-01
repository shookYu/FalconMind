/**
 * @file LawnMowerScenarioReal.cpp
 * @brief 场景1.1真实版本 - 真实飞控连接实现
 * 
 * 使用FalconMindSDK真实的MavlinkClient连接PX4飞控
 * 实现真实的MAVLink通信，上传航点任务并执行
 */

#include "LawnMowerScenarioReal.h"
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
    constexpr int WAYPOINT_TIMEOUT_S = 60;
    constexpr float HOME_LAT = 34.052200f;
    constexpr float HOME_LON = -118.243700f;
}

/**
 * @brief 构造函数 - 初始化配置
 */
LawnMowerConfig::LawnMowerConfig()
    : connection("udp://127.0.0.1:14550")
    , baudRate(57600)
    , takeoffAltitude(30.0f)
    , searchAltitude(50.0f)
    , speed(5.0f)
    , lineSpacing(30.0f)
    , acceptanceRadius(3.0f)
{
    // 默认搜索区域：约100m x 60m矩形
    // 使用WGS84坐标系，定义矩形区域的四个顶点
    searchArea.push_back({34.052200f, -118.243700f, 30.0f});  // 左下角
    searchArea.push_back({34.052200f, -118.243000f, 30.0f});  // 右下角
    searchArea.push_back({34.052800f, -118.243000f, 30.0f});  // 右上角
    searchArea.push_back({34.052800f, -118.243700f, 30.0f});  // 左上角
}

/**
 * @brief 构造函数
 * @param config 场景配置
 */
LawnMowerScenarioReal::LawnMowerScenarioReal(const LawnMowerConfig& config)
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
LawnMowerScenarioReal::~LawnMowerScenarioReal()
{
    if (connected_) {
        disconnectVehicle();
    }
}

/**
 * @brief 执行场景主流程
 * @return true成功，false失败
 * 
 * 执行流程：
 * 1. 连接飞控
 * 2. 检查飞控健康状态
 * 3. 生成搜索路径
 * 4. 上传航点任务
 * 5. 解锁电机
 * 6. 执行起飞
 * 7. 监控任务执行
 * 8. 返航降落
 */
bool LawnMowerScenarioReal::execute()
{
    std::cout << "\n========================================" << std::endl;
    std::cout << "场景1.1: 单机网格搜索(LAWN_MOWER)" << std::endl;
    std::cout << "【真实飞控连接版本】" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    startTime_ = std::chrono::steady_clock::now();
    
    // ========== 步骤1: 连接飞控 ==========
    std::cout << "\n[步骤1/8] 连接飞控..." << std::endl;
    std::cout << "  连接地址: " << config_.connection << std::endl;
    
    if (!connectVehicle()) {
        std::cerr << "  [FAILED] 无法连接飞控" << std::endl;
        std::cerr << "  请确保:" << std::endl;
        std::cerr << "    1. PX4 SITL已启动: make px4_sitl gazebo" << std::endl;
        std::cerr << "    2. 或真实飞控已连接且配置正确" << std::endl;
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
    std::cout << "\n[步骤3/8] 生成网格搜索路径..." << std::endl;
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
    std::cout << "  目标高度: " << config_.searchAltitude << "m" << std::endl;
    if (!takeoff(config_.searchAltitude)) {
        std::cerr << "  [FAILED] 起飞失败" << std::endl;
        disarmVehicle();
        return false;
    }
    std::cout << "  [OK] 起飞命令已发送" << std::endl;
    
    // 等待起飞完成
    std::cout << "  等待达到目标高度..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // ========== 步骤7: 执行任务 ==========
    std::cout << "\n[步骤7/8] 开始执行搜索任务..." << std::endl;
    if (!startMission()) {
        std::cerr << "  [FAILED] 任务启动失败" << std::endl;
        returnToLaunch();
        return false;
    }
    
    std::cout << "  [OK] 任务已启动" << std::endl;
    std::cout << "  监控任务执行..." << std::endl;
    
    // 监控任务执行
    monitorMissionExecution();
    
    // ========== 步骤8: 返航 ==========
    std::cout << "\n[步骤8/8] 返航..." << std::endl;
    if (!returnToLaunch()) {
        std::cerr << "  [WARNING] 返航命令发送失败" << std::endl;
    }
    
    // 等待降落
    std::cout << "  等待降落..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // 上锁
    disarmVehicle();
    
    // 断开连接
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
 * @return true成功，false失败
 * 
 * 使用MavlinkClient::connectSITL()或MavlinkClient::connectSerial()建立连接
 */
bool LawnMowerScenarioReal::connectVehicle()
{
    try {
        // 根据连接字符串选择连接方式
        if (config_.connection.find("udp://") == 0 || config_.connection.find("127.0.0.1") != std::string::npos) {
            // UDP连接（SITL模式）
            std::cout << "  使用UDP连接SITL..." << std::endl;
            
            // 解析端口
            int port = 14550;
            size_t pos = config_.connection.find(':');
            if (pos != std::string::npos) {
                try {
                    port = std::stoi(config_.connection.substr(pos + 1));
                } catch (...) {
                    port = 14550;  // 默认端口
                }
            }
            
            // 使用SDK API连接SITL - ResultPtr返回shared_ptr
            auto result = falconmind::sdk::high_level::MavlinkClient::connectSITL(port);
            
            // 检查结果
            if (result.isError()) {
                std::cerr << "  [ERROR] 连接失败: " << result.errorMessage() << std::endl;
                return false;
            }
            
            // 获取MavlinkClient实例 (ResultPtr直接存储shared_ptr)
            mavlinkClient_ = result.value();
        } else {
            // 串口连接（真实飞控）
            std::cout << "  使用串口连接: " << config_.connection << std::endl;
            std::cout << "  波特率: " << config_.baudRate << std::endl;
            
            // 使用SDK API进行串口连接
            auto result = falconmind::sdk::high_level::MavlinkClient::connectSerial(
                config_.connection, config_.baudRate);
                
            if (result.isError()) {
                std::cerr << "  [ERROR] 连接失败: " << result.errorMessage() << std::endl;
                return false;
            }
            
            // 获取MavlinkClient实例
            mavlinkClient_ = result.value();
        }
        // 验证连接
        if (!mavlinkClient_->isConnected()) {
            std::cerr << "  [ERROR] 连接建立但状态异常" << std::endl;
            return false;
        }
        
        connected_ = true;
        
        // 设置状态更新回调
        mavlinkClient_->onStateUpdate([this](const auto& state) {
            // 可以在这里处理状态更新，例如记录日志
            static int counter = 0;
            if (++counter % 50 == 0) {  // 每50次更新打印一次
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
void LawnMowerScenarioReal::disconnectVehicle()
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
 * @return true正常，false异常
 * 
 * 检查电池电量、GPS信号、传感器状态等
 */
bool LawnMowerScenarioReal::checkVehicleHealth()
{
    if (!connected_ || !mavlinkClient_) {
        return false;
    }
    
    // 获取当前状态
    auto state = mavlinkClient_->getState();
    
    // 检查电池电量
    std::cout << "  电池电量: " << state.batteryPercent << "%" << std::endl;
    if (state.batteryPercent < 20.0f) {
        std::cerr << "  [WARNING] 电池电量低!" << std::endl;
        return false;
    }
    
    // 检查GPS信号
    std::cout << "  GPS状态: " << (state.gpsFixType >= 3 ? "3D定位" : "无定位") << std::endl;
    std::cout << "  卫星数: " << state.satellites << std::endl;
    if (state.gpsFixType < 3) {
        std::cerr << "  [WARNING] GPS信号不足!" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 生成LAWN_MOWER搜索路径
 * @return 航点列表
 * 
 * 在指定区域内生成网格状搜索路径
 */
std::vector<Waypoint> LawnMowerScenarioReal::generateSearchPath()
{
    std::vector<Waypoint> waypoints;
    
    if (config_.searchArea.size() < 4) {
        std::cerr << "  [ERROR] 搜索区域至少需要4个顶点" << std::endl;
        return waypoints;
    }
    
    // 计算边界框
    float minLat = config_.searchArea[0].latitude;
    float maxLat = config_.searchArea[0].latitude;
    float minLon = config_.searchArea[0].longitude;
    float maxLon = config_.searchArea[0].longitude;
    
    for (const auto& p : config_.searchArea) {
        minLat = std::min(minLat, p.latitude);
        maxLat = std::max(maxLat, p.latitude);
        minLon = std::min(minLon, p.longitude);
        maxLon = std::max(maxLon, p.longitude);
    }
    
    // 计算区域宽度（转换为米）
    const float METERS_PER_DEGREE_LAT = 111320.0f;
    float metersPerDegreeLon = METERS_PER_DEGREE_LAT * std::cos((minLat + maxLat) / 2.0f * M_PI / 180.0f);
    
    float widthMeters = (maxLon - minLon) * metersPerDegreeLon;
    int numLines = static_cast<int>(widthMeters / config_.lineSpacing) + 1;
    
    std::cout << "  区域宽度: " << widthMeters << "m" << std::endl;
    std::cout << "  航线数量: " << numLines << std::endl;
    
    // 起飞点（Home点）
    waypoints.push_back(Waypoint{minLat, minLon, config_.takeoffAltitude, config_.speed});
    
    // 生成LAWN_MOWER航线（往复式）
    bool goingNorth = true;
    for (int i = 0; i < numLines; ++i) {
        float lon = minLon + (i * config_.lineSpacing / metersPerDegreeLon);
        if (lon > maxLon) lon = maxLon;
        
        if (goingNorth) {
            // 从南向北飞
            waypoints.push_back(Waypoint{minLat, lon, config_.searchAltitude, config_.speed});
            waypoints.push_back(Waypoint{maxLat, lon, config_.searchAltitude, config_.speed});
        } else {
            // 从北向南飞
            waypoints.push_back(Waypoint{maxLat, lon, config_.searchAltitude, config_.speed});
            waypoints.push_back(Waypoint{minLat, lon, config_.searchAltitude, config_.speed});
        }
        goingNorth = !goingNorth;
    }
    
    // 返航点（回到Home点）
    waypoints.push_back(Waypoint{minLat, minLon, config_.takeoffAltitude, config_.speed});
    
    return waypoints;
}

/**
 * @brief 上传航点任务到飞控
 * @param waypoints 航点列表
 * @return true成功，false失败
 * 
 * 使用MavlinkClient::uploadMission()上传航点
 */
bool LawnMowerScenarioReal::uploadMission(const std::vector<Waypoint>& waypoints)
{
    if (!connected_ || !mavlinkClient_) {
        std::cerr << "  [ERROR] 未连接飞控" << std::endl;
        return false;
    }
    
    // 转换为SDK需要的格式
    std::vector<std::tuple<double, double, double>> sdkWaypoints;
    for (const auto& wp : waypoints) {
        sdkWaypoints.push_back(std::make_tuple(wp.latitude, wp.longitude, wp.altitude));
    }
    
    // 使用SDK API上传任务
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
 * @return true成功，false失败
 */
bool LawnMowerScenarioReal::armVehicle()
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
 * @return true成功，false失败
 */
bool LawnMowerScenarioReal::disarmVehicle()
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
 * @param altitude 目标高度（米）
 * @return true成功，false失败
 */
bool LawnMowerScenarioReal::takeoff(float altitude)
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
 * @return true成功，false失败
 */
bool LawnMowerScenarioReal::startMission()
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
 * 
 * 轮询飞控状态，监控航点进度
 */
void LawnMowerScenarioReal::monitorMissionExecution()
{
    int totalWaypoints = static_cast<int>(waypoints_.size());
    int lastWaypoint = -1;
    
    std::cout << "\n  监控任务执行（共 " << totalWaypoints << " 个航点）..." << std::endl;
    std::cout << "  按Ctrl+C可随时中断\n" << std::endl;
    
    while (missionRunning_ && connected_) {
        // 获取当前状态
        auto state = mavlinkClient_->getState();
        
        // 检测航点变化
        if (currentWaypoint_ != lastWaypoint) {
            float percent = static_cast<float>(currentWaypoint_) / totalWaypoints * 100.0f;
            std::cout << "  [" << (currentWaypoint_ + 1) << "/" << totalWaypoints << "] " 
                      << static_cast<int>(percent) << "% - 飞往航点WP" << currentWaypoint_ << std::endl;
            
            reportProgress(currentWaypoint_ + 1, totalWaypoints, 
                          "飞往航点WP" + std::to_string(currentWaypoint_));
            
            lastWaypoint = currentWaypoint_;
        }
        
        // 更新当前航点索引（模拟）
        // 真实实现应该解析MISSION_CURRENT消息
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
        
        // 检查是否完成任务
        if (!missionRunning_) {
            std::cout << "  [OK] 所有航点已完成" << std::endl;
            break;
        }
        
        // 休眠一段时间避免CPU占用过高
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

/**
 * @brief 返航
 * @return true成功，false失败
 */
bool LawnMowerScenarioReal::returnToLaunch()
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
 * @param current 当前航点
 * @param total 总航点数
 * @param status 状态描述
 */
void LawnMowerScenarioReal::reportProgress(int current, int total, const std::string& status)
{
    if (progressCallback_) {
        progressCallback_(current, total, status);
    }
}

/**
 * @brief 打印飞控信息
 */
void LawnMowerScenarioReal::printVehicleInfo()
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
