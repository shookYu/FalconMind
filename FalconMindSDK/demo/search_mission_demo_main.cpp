// FalconMindSDK - Search Mission Demo
// 演示单机区域搜索场景：起飞 → 搜索路径规划 → 检测 → 上报 → 返航

#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/flight/FlightConnectionService.h"
#include "falconmind/sdk/flight/FlightNodes.h"
#include "falconmind/sdk/mission/SearchTypes.h"
#include "falconmind/sdk/mission/SearchPathPlannerNode.h"
#include "falconmind/sdk/mission/EventReporterNode.h"
#include "falconmind/sdk/mission/SearchMissionAction.h"
#include "falconmind/sdk/mission/BehaviorTree.h"
#include "falconmind/sdk/sensors/CameraSourceNode.h"
#include "falconmind/sdk/perception/DummyDetectionNode.h"
#include "falconmind/sdk/telemetry/TelemetryPublisher.h"

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace falconmind::sdk;

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "FalconMindSDK - Search Mission Demo" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::endl;

    // 1. 创建 FlightConnectionService
    flight::FlightConnectionConfig flightConfig;
    flightConfig.remoteAddress = "127.0.0.1";
    flightConfig.remotePort = 14540;
    
    auto flightSvc = std::make_shared<flight::FlightConnectionService>();
    if (!flightSvc->connect(flightConfig)) {
        std::cerr << "Failed to connect to PX4-SITL" << std::endl;
        return 1;
    }

    // 2. 定义搜索区域（北京市昌平区昌平公园附近，矩形区域）
    mission::SearchArea searchArea;
    searchArea.polygon = {
        {39.9000, 116.3900, 50.0},  // 左下角
        {39.9100, 116.3900, 50.0},  // 左上角
        {39.9100, 116.4000, 50.0},  // 右上角
        {39.9000, 116.4000, 50.0}   // 右下角
    };
    searchArea.minAltitude = 30.0;
    searchArea.maxAltitude = 100.0;

    // 3. 定义搜索参数
    mission::SearchParams searchParams;
    searchParams.pattern = mission::SearchPattern::LAWN_MOWER;
    searchParams.altitude = 50.0;  // 50米高度
    searchParams.speed = 5.0;       // 5 m/s
    searchParams.spacing = 20.0;     // 20米间距
    searchParams.loiterTime = 2.0;   // 每个航点悬停2秒
    searchParams.enableDetection = true;
    searchParams.detectionClasses = {"person", "vehicle"};

    // 4. 创建搜索路径规划节点
    auto pathPlanner = std::make_shared<mission::SearchPathPlannerNode>();
    pathPlanner->setSearchArea(searchArea);
    pathPlanner->setSearchParams(searchParams);

    // 5. 创建事件上报节点
    auto eventReporter = std::make_shared<mission::EventReporterNode>();

    // 6. 创建搜索任务行为树节点
    auto searchAction = std::make_shared<mission::SearchMissionAction>(
        *flightSvc, pathPlanner, eventReporter
    );
    searchAction->setSearchArea(searchArea);
    searchAction->setSearchParams(searchParams);

    // 7. 创建 Pipeline
    core::PipelineConfig pipelineConfig;
    pipelineConfig.pipelineId = "search_mission_pipeline";
    pipelineConfig.name = "Search Mission Pipeline";
    core::Pipeline pipeline(pipelineConfig);

    // 8. 添加节点到 Pipeline
    auto flightStateSource = std::make_shared<flight::FlightStateSourceNode>(*flightSvc);
    auto flightCommandSink = std::make_shared<flight::FlightCommandSinkNode>(*flightSvc);
    
    pipeline.addNode(flightStateSource);
    pipeline.addNode(flightCommandSink);
    pipeline.addNode(pathPlanner);
    pipeline.addNode(eventReporter);

    // 9. 配置并启动节点
    std::unordered_map<std::string, std::string> nodeParams;
    flightStateSource->configure(nodeParams);
    flightCommandSink->configure(nodeParams);
    pathPlanner->configure(nodeParams);
    eventReporter->configure(nodeParams);
    
    flightStateSource->start();
    flightCommandSink->start();
    pathPlanner->start();
    eventReporter->start();

    // 10. 启动 Pipeline
    pipeline.setState(core::PipelineState::Playing);

    // 11. 创建行为树执行器
    mission::BehaviorTreeExecutor executor(searchAction);

    std::cout << "Starting search mission..." << std::endl;
    std::cout << "Search area: (" << searchArea.polygon[0].lat << ", " 
              << searchArea.polygon[0].lon << ") to (" 
              << searchArea.polygon[2].lat << ", " 
              << searchArea.polygon[2].lon << ")" << std::endl;
    std::cout << "Search pattern: LAWN_MOWER" << std::endl;
    std::cout << "Altitude: " << searchParams.altitude << " m" << std::endl;
    std::cout << "Spacing: " << searchParams.spacing << " m" << std::endl;
    std::cout << std::endl;

    // 12. 执行行为树（搜索任务）
    int tickCount = 0;
    while (true) {
        // 轮询飞行状态
        flightSvc->pollState();
        
        // 执行行为树
        auto status = executor.tick();
        
        if (status == mission::NodeStatus::Success) {
            std::cout << "Search mission completed!" << std::endl;
            break;
        } else if (status == mission::NodeStatus::Failure) {
            std::cerr << "Search mission failed!" << std::endl;
            break;
        }
        
        // 处理 Pipeline 数据流（当前 Pipeline 没有 process 方法，由节点自己处理）
        // pipeline.process();  // TODO: 如果 Pipeline 有 process 方法，可以在这里调用
        
        tickCount++;
        if (tickCount % 100 == 0) {
            std::cout << "Tick: " << tickCount << ", Status: Running" << std::endl;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 13. 停止 Pipeline
    pipeline.setState(core::PipelineState::Null);
    
    flightStateSource->stop();
    flightCommandSink->stop();
    pathPlanner->stop();
    eventReporter->stop();

    std::cout << "Demo completed." << std::endl;
    return 0;
}
