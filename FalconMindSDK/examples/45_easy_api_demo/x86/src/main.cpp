/**
 * FalconMindSDK 示例45：Easy API 高级封装演示（x86平台版本）
 *
 * 本示例演示如何使用新的 FalconMind::Easy API（即 high_level API）
 * 大幅简化感知流水线的创建和使用。
 *
 * 对比：
 *   - 传统API: 需要手动创建Node、配置Pad、连接Pipeline（50+行代码）
 *   - Easy API: 仅需几行代码即可启动完整感知流程
 *
 * 新特性：
 *   - Result<T> 错误处理（替代bool返回值）
 *   - Builder模式流式配置
 *   - 自动资源管理
 */

#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

#include "falconmind/sdk/high_level/PerceptionPipeline.h"
#include "falconmind/sdk/high_level/Result.h"

using namespace falconmind::sdk::high_level;

// 示例1：使用Builder模式创建流水线
void demo_builder_pattern() {
    std::cout << "\n=== 示例1: Builder模式创建感知流水线 ===" << std::endl;
    
    // 新的Easy API：流式配置，一行代码创建完整流水线
    auto result = PerceptionPipeline::create()
        .withCamera(640, 480, 30)                    // 配置相机
        .withCameraDevice("/dev/video0")           // 设备路径
        .withDetector("yolov8n.onnx", DetectorBackend::ONNX_RUNTIME)  // 检测器
        .withTracker(TrackerType::DEEPSORT)        // 跟踪器
        .withLowLightEnhancement(true)             // 低光增强
        .build();                                  // 构建
    
    // Result<T> 错误处理：不再是简单的bool
    if (result.isError()) {
        std::cerr << "❌ 创建流水线失败: " << result.errorMessage() << std::endl;
        return;
    }
    
    auto pipeline = result.value();
    std::cout << "✅ 流水线创建成功" << std::endl;
    
    // 设置回调
    pipeline->onDetection([](const std::vector<Detection>& detections) {
        std::cout << "[检测回调] 发现 " << detections.size() << " 个目标" << std::endl;
        for (const auto& det : detections) {
            std::cout << "  - " << det.className << " (" << det.confidence << ")" << std::endl;
        }
    });
    
    // 启动流水线
    auto startResult = pipeline->start();
    if (startResult.isError()) {
        std::cerr << "❌ 启动失败: " << startResult.errorMessage() << std::endl;
        return;
    }
    
    std::cout << "✅ 流水线已启动" << std::endl;
    
    // 运行3秒
    std::this_thread::sleep_for(std::chrono::seconds(3));
    
    // 停止
    pipeline->stop();
    std::cout << "✅ 流水线已停止" << std::endl;
}

// 示例2：使用便捷函数快速创建
void demo_convenience_functions() {
    std::cout << "\n=== 示例2: 便捷函数快速创建 ===" << std::endl;
    
    // 方式1：使用 createPerceptionPipeline 便捷函数
    CameraConfig camConfig(1280, 720, 30);
    DetectorConfig detConfig("yolov8s.onnx", DetectorBackend::ONNX_RUNTIME);
    TrackerConfig trackConfig(TrackerType::SORT);
    
    auto result = createPerceptionPipeline(camConfig, detConfig, trackConfig);
    
    if (result) {
        std::cout << "✅ 通过便捷函数创建成功" << std::endl;
        auto pipeline = result.value();
        
        // 单目标回调
        pipeline->onDetection([](const Detection& det) {
            if (det.isTracked()) {
                std::cout << "[跟踪] ID=" << det.trackId << " 类型=" << det.className << std::endl;
            }
        });
        
        pipeline->start();
        std::this_thread::sleep_for(std::chrono::seconds(2));
        pipeline->stop();
    } else {
        std::cerr << "❌ 创建失败: " << result.errorMessage() << std::endl;
    }
}

// 示例3：极简模式（自动配置）
void demo_minimal_mode() {
    std::cout << "\n=== 示例3: 极简模式（一行代码启动） ===" << std::endl;
    
    // 一行代码创建完整流水线，自动选择最佳后端
    auto result = createMinimalPipeline("yolov8n.onnx", DetectorBackend::AUTO);
    
    if (!result) {
        std::cerr << "❌ 极简模式创建失败: " << result.errorMessage() << std::endl;
        return;
    }
    
    auto pipeline = result.value();
    std::cout << "✅ 极简流水线创建成功（自动配置）" << std::endl;
    
    // 显示统计信息
    auto stats = pipeline->getStatistics();
    std::cout << "统计: FPS=" << stats.averageFps 
              << " 处理帧数=" << stats.framesProcessed << std::endl;
    
    pipeline->start();
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // 运行时调整参数
    pipeline->setConfidenceThreshold(0.7f);
    std::cout << "已调整置信度阈值到 0.7" << std::endl;
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    pipeline->stop();
}

// 示例4：特定对象检测
void demo_specific_object_detection() {
    std::cout << "\n=== 示例4: 特定对象检测 ===" << std::endl;
    
    auto result = PerceptionPipeline::create()
        .withCamera(640, 480, 30)
        .withDetector("yolov8n.onnx", DetectorBackend::ONNX_RUNTIME)
        .withTracker(TrackerType::DEEPSORT)
        .build();
    
    if (result.isError()) {
        std::cerr << "❌ 创建失败" << std::endl;
        return;
    }
    
    auto pipeline = result.value();
    
    // 只关注特定类别
    pipeline->onObjectDetected("person", [](const Detection& det) {
        std::cout << "🚶 检测到行人! 置信度=" << det.confidence 
                  << " 位置=[" << det.bbox.x << "," << det.bbox.y << "]" << std::endl;
    });
    
    pipeline->onObjectDetected("car", [](const Detection& det) {
        std::cout << "🚗 检测到车辆! 跟踪ID=" << det.trackId << std::endl;
    });
    
    pipeline->start();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    pipeline->stop();
}

// 示例5：错误处理演示
void demo_error_handling() {
    std::cout << "\n=== 示例5: Result<T> 错误处理演示 ===" << std::endl;
    
    // 错误1：缺少相机配置
    std::cout << "\n测试1: 缺少相机配置" << std::endl;
    auto result1 = PerceptionPipeline::create()
        .withDetector("model.onnx")  // 只配置检测器
        .build();
    
    if (result1.isError()) {
        std::cout << "✅ 正确捕获错误: " << result1.errorMessage() << std::endl;
        std::cout << "   错误码: " << static_cast<int>(result1.error()) << std::endl;
    }
    
    // 错误2：缺少检测器配置
    std::cout << "\n测试2: 缺少检测器配置" << std::endl;
    auto result2 = PerceptionPipeline::create()
        .withCamera(640, 480, 30)  // 只配置相机
        .build();
    
    if (result2.isError()) {
        std::cout << "✅ 正确捕获错误: " << result2.errorMessage() << std::endl;
    }
    
    // 成功场景
    std::cout << "\n测试3: 正确配置" << std::endl;
    auto result3 = PerceptionPipeline::create()
        .withCamera(640, 480, 30)
        .withDetector("yolov8n.onnx")
        .build();
    
    if (result3.isSuccess()) {
        std::cout << "✅ 配置成功!" << std::endl;
    }
    
    // 使用函数式风格的错误处理
    std::cout << "\n测试4: 函数式错误处理" << std::endl;
    auto result4 = PerceptionPipeline::create()
        .withCamera(640, 480, 30)
        .withDetector("yolov8n.onnx")
        .build()
        .map([](std::shared_ptr<PerceptionPipeline> p) {
            std::cout << "✅ Map操作: 成功获取流水线" << std::endl;
            return p;
        });
    
    if (!result4) {
        std::cout << "❌ Map链式处理后的错误: " << result4.errorMessage() << std::endl;
    }
}

// 主函数
int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "                    FalconMindSDK 示例45: Easy API 演示" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "本示例演示 FalconMind::Easy API（high_level API）的使用" << std::endl;
    std::cout << "特性：" << std::endl;
    std::cout << "  • Builder模式流式配置" << std::endl;
    std::cout << "  • Result<T> 强类型错误处理" << std::endl;
    std::cout << "  • 便捷函数快速启动" << std::endl;
    std::cout << "  • 极简一行代码部署" << std::endl;
    std::cout << std::endl;
    
    try {
        demo_builder_pattern();
        demo_convenience_functions();
        demo_minimal_mode();
        demo_specific_object_detection();
        demo_error_handling();
        
        std::cout << "\n================================================================================" << std::endl;
        std::cout << "                    所有演示完成" << std::endl;
        std::cout << "================================================================================" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ 异常: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
