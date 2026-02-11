#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/sensors/CameraSourceNode.h"
#include "falconmind/sdk/perception/DummyDetectionNode.h"
#include "falconmind/sdk/perception/TrackingTransformNode.h"
#include "falconmind/sdk/perception/SimpleTrackerBackend.h"
#include "TestNodes.h"

#include <chrono>
#include <thread>
#include <iostream>

using namespace falconmind::sdk;

int main() {
    core::PipelineConfig cfg;
    cfg.pipelineId = "tracking_demo";
    cfg.name = "Camera + Detection + Tracking Pipeline Demo";
    cfg.description = "camera_source → dummy detection → tracking_transform → log_sink";

    core::Pipeline pipeline(cfg);

    sensors::VideoSourceConfig vcfg;
    vcfg.sensorId = "cam0";
    vcfg.device   = "/dev/video0";
    vcfg.width    = 640;
    vcfg.height   = 480;
    vcfg.fps      = 30.0;

    auto camNode = std::make_shared<sensors::CameraSourceNode>(vcfg);
    auto detNode = std::make_shared<perception::DummyDetectionNode>();
    auto trackNode = std::make_shared<perception::TrackingTransformNode>();
    auto logNode = std::make_shared<demo::LogSinkNode>();

    // 为 tracking 节点注入简单 tracker 后端
    auto trackerBackend = std::make_shared<perception::SimpleTrackerBackend>();
    trackerBackend->load();
    trackNode->setBackend(trackerBackend);

    if (!pipeline.addNode(camNode) ||
        !pipeline.addNode(detNode) ||
        !pipeline.addNode(trackNode) ||
        !pipeline.addNode(logNode)) {
        std::cerr << "[tracking_demo] Failed to add nodes to pipeline" << std::endl;
        return 1;
    }

    if (!pipeline.link(camNode->id(), "video_out", detNode->id(), "video_in") ||
        !pipeline.link(detNode->id(), "detection_out", trackNode->id(), "detection_in") ||
        !pipeline.link(trackNode->id(), "tracking_out", logNode->id(), "in")) {
        std::cerr << "[tracking_demo] Failed to link nodes" << std::endl;
        return 1;
    }

    std::unordered_map<std::string, std::string> camParams{{"device", "/dev/video0"}};
    camNode->configure(camParams);
    detNode->configure({{"modelName", "dummy-yolo"}});
    logNode->configure({{"prefix", "[TrackingLog]"}});

    camNode->start();
    detNode->start();
    trackNode->start();
    logNode->start();

    pipeline.setState(core::PipelineState::Ready);
    pipeline.setState(core::PipelineState::Playing);

    for (int i = 0; i < 3; ++i) {
        camNode->process();
        detNode->process();
        trackNode->process();
        logNode->process();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[tracking_demo] Finished." << std::endl;
    return 0;
}

