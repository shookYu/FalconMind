/**
 * @file main.cpp
 * @brief 场景2.2: 单机搜索 + 目标跟踪
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include "falconmind/sdk/high_level/PerceptionPipeline.h"
#include "falconmind/sdk/high_level/TrackingMission.h"
#include "falconmind/sdk/core/Logger.h"

using namespace falconmind::sdk;
using namespace falconmind::sdk::high_level;
using namespace falconmind::sdk::perception;

class SearchTrackingScenario {
public:
    bool execute() {
        LOG_INFO("Scenario") << "=== 场景2.2: 搜索 + 目标跟踪 ===";
        
        // 创建感知流水线用于跟踪
        auto pipelineResult = PerceptionPipeline::create()
            .withCamera(640, 480, 30)
            .withCameraDevice("/dev/video0")
            .withDetector("yolov8n.onnx", DetectorBackend::ONNX_RUNTIME)
            .withTracker(TrackerType::DEEPSORT)
            .build();
        
        if (!pipelineResult) {
            LOG_ERROR("Scenario") << "感知流水线创建失败: " + pipelineResult.errorMessage();
            return false;
        }
        
        auto pipeline = pipelineResult.value();
        
        // 跟踪状态
        std::atomic<int> trackedTargets{0};
        std::atomic<bool> targetLocked{false};
        
        // 检测回调
        pipeline->onDetection([&trackedTargets, &targetLocked](const std::vector<Detection>& detections) {
            int currentTracked = 0;
            for (const auto& det : detections) {
                if (det.isTracked && det.trackId >= 0) {
                    currentTracked++;
                    LOG_INFO("Scenario") << "[跟踪] ID=" + std::to_string(det.trackId + 
                             " 类型=" + det.className);
                }
            }
            trackedTargets = currentTracked;
            if (currentTracked > 0) {
                targetLocked = true;
            }
        });
        
        // 启动感知
        pipeline->start();
        
        // 模拟搜索过程中跟踪
        LOG_INFO("Scenario") << "搜索中... 等待目标进入视野";
        for (int i = 0; i < 30; ++i) {  // 30秒
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (targetLocked) {
                LOG_INFO("Scenario") << "已锁定目标，持续跟踪中...";
            }
        }
        
        pipeline->stop();
        
        LOG_INFO("Scenario") << "跟踪任务完成，跟踪目标数: " + std::to_string(trackedTargets.load());
        return true;
    }
};

int main() {
    std::cout << "场景2.2: 单机搜索 + 目标跟踪" << std::endl;
    return SearchTrackingScenario().execute() ? 0 : 1;
}
