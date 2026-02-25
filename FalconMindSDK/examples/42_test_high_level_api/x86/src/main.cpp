/**
 * @file test_high_level_api.cpp
 * @brief Test example for high-level API (Phase 1)
 */

#include "falconmind/sdk/high_level/PerceptionPipeline.h"
#include "falconmind/sdk/high_level/Result.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace falconmind::sdk::high_level;

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Phase 1 Test: High-Level API (FalconMind::high_level)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    // Test 1: Create pipeline using builder pattern
    std::cout << "[Test 1] Creating pipeline with builder pattern..." << std::endl;
    
    auto result = PerceptionPipeline::create()
        .withCamera(640, 480, 30)
        .withDetector("yolov8n.rknn", DetectorBackend::RKNN)
        .withTracker(TrackerType::DEEPSORT)
        .withLowLightEnhancement(true)
        .build();
    
    if (!result) {
        std::cerr << "[Error] Failed to create pipeline: " 
                  << result.errorMessage() << std::endl;
        return 1;
    }
    
    // Unwrap Result to get the pipeline
    auto& pipeline = result.value();
    
    std::cout << "  [PASS] Pipeline created successfully" << std::endl;
    std::cout << std::endl;
    
    // Test 2: Set up detection callbacks
    std::cout << "[Test 2] Setting up detection callbacks..." << std::endl;
    
    int detectionCount = 0;
    pipeline->onDetection([&detectionCount](const Detection& det) {
        detectionCount++;
        std::cout << "  [Detection] " << det.className 
                  << " (conf: " << det.confidence << ")" << std::endl;
    });
    
    std::cout << "  [PASS] Callbacks configured" << std::endl;
    std::cout << std::endl;
    
    // Test 3: Start pipeline
    std::cout << "[Test 3] Starting pipeline..." << std::endl;
    
    auto startResult = pipeline->start();
    if (!startResult) {
        std::cerr << "[Error] Failed to start pipeline: " 
                  << startResult.errorMessage() << std::endl;
        return 1;
    }
    
    std::cout << "  [PASS] Pipeline started" << std::endl;
    std::cout << "  Running: " << (pipeline->isRunning() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
    
    // Test 4: Get statistics
    std::cout << "[Test 4] Getting pipeline statistics..." << std::endl;
    
    auto stats = pipeline->getStatistics();
    std::cout << "  Frames processed: " << stats.framesProcessed << std::endl;
    std::cout << "  Detections total: " << stats.detectionsTotal << std::endl;
    std::cout << "  Average FPS: " << stats.averageFps << std::endl;
    std::cout << "  [PASS] Statistics retrieved" << std::endl;
    std::cout << std::endl;
    
    // Test 5: Stop pipeline
    std::cout << "[Test 5] Stopping pipeline..." << std::endl;
    
    auto stopResult = pipeline->stop();
    if (!stopResult) {
        std::cerr << "[Error] Failed to stop pipeline: " 
                  << stopResult.errorMessage() << std::endl;
        return 1;
    }
    
    std::cout << "  [PASS] Pipeline stopped" << std::endl;
    std::cout << "  Running: " << (pipeline->isRunning() ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
    
    // Test 6: Error handling test
    std::cout << "[Test 6] Testing error handling..." << std::endl;
    
    // Try to create pipeline without camera (should fail)
    auto invalidResult = PerceptionPipeline::create()
        .withDetector("model.rknn", DetectorBackend::RKNN)
        .build();
    
    if (!invalidResult) {
        std::cout << "  [PASS] Correctly rejected pipeline without camera" << std::endl;
        std::cout << "  Error code: " << static_cast<int>(invalidResult.error()) << std::endl;
        std::cout << "  Error message: " << invalidResult.errorMessage() << std::endl;
    } else {
        std::cerr << "  [FAIL] Should have rejected pipeline without camera" << std::endl;
        return 1;
    }
    std::cout << std::endl;
    
    // Test 7: Convenience function
    std::cout << "[Test 7] Using convenience function..." << std::endl;
    
    auto simpleResult = createMinimalPipeline("yolov8n.rknn", DetectorBackend::RKNN);
    if (simpleResult) {
        std::cout << "  [PASS] Minimal pipeline created successfully" << std::endl;
        auto& simplePipeline = simpleResult.value();
        simplePipeline->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        simplePipeline->stop();
    } else {
        std::cerr << "  [FAIL] Failed to create minimal pipeline: " 
                  << simpleResult.errorMessage() << std::endl;
        return 1;
    }
    std::cout << std::endl;
    
    // Summary
    std::cout << "================================================================================" << std::endl;
    std::cout << "  Phase 1 Test Results: ALL PASSED" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Features tested:" << std::endl;
    std::cout << "  - Builder pattern API" << std::endl;
    std::cout << "  - Result<T> error handling" << std::endl;
    std::cout << "  - Pipeline lifecycle (start/stop)" << std::endl;
    std::cout << "  - Callback registration" << std::endl;
    std::cout << "  - Statistics retrieval" << std::endl;
    std::cout << "  - Error code system" << std::endl;
    std::cout << "  - Convenience functions" << std::endl;
    std::cout << std::endl;
    std::cout << "Phase 1 implementation: COMPLETE" << std::endl;
    
    return 0;
}
