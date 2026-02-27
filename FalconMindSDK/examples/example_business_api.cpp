/**
 * @file example_business_api.cpp
 * @brief 新业务API使用示例
 * 
 * 展示如何使用ConfigManager、MissionTemplates等新功能
 */

#include "falconmind/sdk/core/ConfigManager.h"
#include "falconmind/sdk/core/Logger.h"
#include "falconmind/sdk/core/Telemetry.h"
#include "falconmind/sdk/high_level/MissionTemplates.h"
#include "falconmind/sdk/high_level/FlightPipeline.h"
#include <iostream>

using namespace falconmind::sdk;

int main() {
    // ========================================================================
    // 1. 初始化日志系统
    // ========================================================================
    core::Logger::init({
        .level = core::LogLevel::Info,
        .outputFile = "mission.log",
        .enableConsole = true,
        .enableTelemetry = true
    });
    
    LOG_INFO("Main") << "FalconMindSDK Business API Demo Started";
    
    // ========================================================================
    // 2. 加载配置
    // ========================================================================
    core::ConfigManager config;
    auto result = config.loadFromFile("search_and_rescue.yaml");
    if (!result) {
        LOG_ERROR("Config") << "Failed to load config: " << result.error().message();
        return 1;
    }
    
    // 监听配置变更
    config.onChange<double>("flight.altitude", [](double oldVal, double newVal) {
        LOG_INFO("Config") << "Altitude changed from " << oldVal 
                  << " to " << newVal;
    });
    
    // ========================================================================
    // 3. 方法1: 使用Builder模式创建任务（推荐）
    // ========================================================================
    LOG_INFO("Demo") << "Creating Search and Rescue mission using Builder...";
    
    auto mission = high_level::MissionTemplates::createSearchAndRescue()
        .withSearchArea({
            {31.2304, 121.4737, 0},
            {31.2404, 121.4737, 0},
            {31.2404, 121.4837, 0},
            {31.2304, 121.4837, 0}
        })
        .withTargetTypes({
            high_level::TargetType::Person,
            high_level::TargetType::Vehicle
        })
        .withAltitude(config.get<double>("flight.altitude", 80.0))
        .withCamera(high_level::CameraType::Thermal)
        .withDetectionModel("yolov8_person_vehicle")
        .withConfidenceThreshold(0.6)
        .enableVideoRecording(true)
        .withGeofence({
            {31.2200, 121.4600, 0},
            {31.2500, 121.4600, 0},
            {31.2500, 121.5000, 0},
            {31.2200, 121.5000, 0}
        })
        .build();
    
    if (!mission) {
        LOG_ERROR("Mission") << "Failed to build mission: " << mission.error().message();
        return 1;
    }
    
    // ========================================================================
    // 4. 方法2: 从配置文件加载任务
    // ========================================================================
    LOG_INFO("Demo") << "Loading mission from config file...";
    
    auto mission2 = high_level::MissionTemplates::loadFromConfig("search_and_rescue.yaml");
    if (!mission2) {
        LOG_ERROR("Mission") << "Failed to load mission: " << mission2.error().message();
    }
    
    // ========================================================================
    // 5. 启动遥测系统
    // ========================================================================
    core::Telemetry telemetry;
    telemetry.start("ws://groundstation:8080/telemetry");
    
    // 订阅遥测数据
    telemetry.onVehicleState([](const core::VehicleState& state) {
        LOG_DEBUG("Telemetry") << "Position: " << state.position.latitude << ", "
                  << state.position.longitude << " Battery: " << state.batteryPercent << "%";
    });
    
    telemetry.onDetection([](const core::DetectionEvent& det) {
        LOG_INFO("Detection") << "Detected: " << det.className 
                  << " (confidence: " << det.confidence << ")";
    });
    
    telemetry.onMissionProgress([](const core::MissionProgress& progress) {
        LOG_INFO("Progress") << "Mission progress: " << progress.percentComplete << "%"
                  << " (" << progress.completedWaypoints << "/" << progress.totalWaypoints << " waypoints)";
    });
    
    // ========================================================================
    // 6. 启动任务
    // ========================================================================
    LOG_INFO("Mission") << "Starting mission...";
    
    auto& pipeline = *mission;
    if (!pipeline.start()) {
        LOG_ERROR("Mission") << "Failed to start mission";
        return 1;
    }
    
    // ========================================================================
    // 7. 等待任务完成或用户中断
    // ========================================================================
    LOG_INFO("Mission") << "Mission running. Press Enter to abort...";
    std::cin.get();
    
    // ========================================================================
    // 8. 优雅停止
    // ========================================================================
    LOG_INFO("Mission") << "Stopping mission...";
    pipeline.stop();
    telemetry.stop();
    core::Logger::shutdown();
    
    LOG_INFO("Main") << "Demo completed successfully";
    return 0;
}

// ========================================================================
// 其他业务场景示例
// ========================================================================

void example_inspection() {
    auto inspection = high_level::MissionTemplates::createInspection()
        .withInspectionPoints({
            {31.2300, 121.4700, 30},
            {31.2310, 121.4710, 30},
            {31.2320, 121.4720, 30}
        })
        .withCameraAngle(-45, 0)  // pitch, yaw
        .withZoomLevel(5)
        .withPhotoInterval(3.0)
        .enableAutoFocus(true)
        .withAltitude(30)
        .build();
    
    if (inspection) {
        inspection->start();
    }
}

void example_tracking() {
    auto tracking = high_level::MissionTemplates::createTracking()
        .withTargetId("person_001")
        .withTargetType(high_level::TargetType::Person)
        .withInitialPosition({31.2300, 121.4700, 0})
        .withTrackingDistance(50)
        .withTrackingAltitude(30)
        .withTrackingAngle(30)
        .enableAutoZoom(true)
        .withLostTargetTimeout(10)
        .build();
    
    if (tracking) {
        tracking->start();
    }
}

void example_survey() {
    auto survey = high_level::MissionTemplates::createSurvey()
        .withSurveyArea({
            {31.2200, 121.4600, 0},
            {31.2400, 121.4600, 0},
            {31.2400, 121.4900, 0},
            {31.2200, 121.4900, 0}
        })
        .withGroundSamplingDistance(2.0)  // 2cm/pixel
        .withAltitude(100)
        .withOverlap(80, 70)  // forward, side overlap
        .build();
    
    if (survey) {
        survey->start();
    }
}
