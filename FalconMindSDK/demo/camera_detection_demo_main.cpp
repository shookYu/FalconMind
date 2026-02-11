#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/sensors/CameraSourceNode.h"
#include "falconmind/sdk/perception/DummyDetectionNode.h"
#include "TestNodes.h"

#include <chrono>
#include <thread>
#include <iostream>

using namespace falconmind::sdk;

int main() {
    // 1. 准备 Pipeline 配置
    core::PipelineConfig cfg;
    cfg.pipelineId = "camera_detection_demo";
    cfg.name = "Camera + DummyDetection Pipeline Demo";
    cfg.description = "camera_source → dummy detection → log sink";

    core::Pipeline pipeline(cfg);

    // 2. 创建节点：CameraSourceNode + DummyDetectionNode + LogSinkNode
    sensors::VideoSourceConfig vcfg;
    vcfg.sensorId = "cam0";
    vcfg.device   = "/dev/video0";  // 如需使用其它源可在后续配置中覆盖
    vcfg.width    = 640;
    vcfg.height   = 480;
    vcfg.fps      = 30.0;

    auto camNode = std::make_shared<sensors::CameraSourceNode>(vcfg);
    auto detNode = std::make_shared<perception::DummyDetectionNode>();
    auto logNode = std::make_shared<demo::LogSinkNode>();

    // 3. 将节点加入 Pipeline 并连线
    if (!pipeline.addNode(camNode) ||
        !pipeline.addNode(detNode) ||
        !pipeline.addNode(logNode)) {
        std::cerr << "[camera_detection_demo] Failed to add nodes to pipeline" << std::endl;
        return 1;
    }

    // camera_source.video_out → detection_transform.video_in
    if (!pipeline.link(camNode->id(), "video_out", detNode->id(), "video_in")) {
        std::cerr << "[camera_detection_demo] Failed to link camera to detection" << std::endl;
        return 1;
    }
    // detection_transform.detection_out → log_sink.in
    if (!pipeline.link(detNode->id(), "detection_out", logNode->id(), "in")) {
        std::cerr << "[camera_detection_demo] Failed to link detection to log sink" << std::endl;
        return 1;
    }

    // 4. 配置并启动各个节点
    std::unordered_map<std::string, std::string> camParams{
        {"device", "/dev/video0"}
    };
    camNode->configure(camParams);
    detNode->configure({{"modelName", "dummy-yolo"}});
    logNode->configure({{"prefix", "[DetectionLog]"}});

    camNode->start();
    detNode->start();
    logNode->start();

    pipeline.setState(core::PipelineState::Ready);
    pipeline.setState(core::PipelineState::Playing);

    // 5. 简单循环调用 process，模拟若干帧的处理
    for (int i = 0; i < 3; ++i) {
        camNode->process(); // 模拟采集一帧
        detNode->process(); // 模拟检测一帧
        logNode->process(); // 打印一条日志
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[camera_detection_demo] Finished." << std::endl;
    return 0;
}

