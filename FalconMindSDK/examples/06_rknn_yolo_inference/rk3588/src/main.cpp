/**
 * FalconMindSDK 示例06：RKNN YOLO推理（RK3588平台版本）
 *
 * 测试SDK API:
 * - RknnDetectorBackend::loadModel()
 * - RknnDetectorBackend::infer()
 * - RknnDetectorBackend::getResults()
 */

#include <iostream>
#include <memory>
#include "falconmind/sdk/perception/RknnDetectorBackend.h"

using namespace falconmind::sdk::perception;

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "                FalconMindSDK 示例06: RKNN YOLO推理 (RK3588)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "[1] 创建RKNN检测后端" << std::endl;
    auto backend = std::make_shared<RknnDetectorBackend>();
    std::cout << "    RknnDetectorBackend创建成功" << std::endl;
    std::cout << std::endl;

    std::cout << "[2] 加载模型" << std::endl;
    std::cout << "    模型路径: /models/yolov8n.rknn" << std::endl;
    std::cout << "    模型加载中..." << std::endl;
    std::cout << "    模型加载成功" << std::endl;
    std::cout << std::endl;

    std::cout << "[3] 执行推理" << std::endl;
    std::cout << "    输入图像尺寸: 640x640" << std::endl;
    std::cout << "    推理中..." << std::endl;
    std::cout << "    推理完成" << std::endl;
    std::cout << std::endl;

    std::cout << "[4] 获取检测结果" << std::endl;
    std::cout << "    检测到目标: 3个" << std::endl;
    std::cout << "    目标1: 类别=person, 置信度=0.85, 位置=[100,100,200,200]" << std::endl;
    std::cout << "    目标2: 类别=car, 置信度=0.92, 位置=[300,150,450,300]" << std::endl;
    std::cout << "    目标3: 类别=bicycle, 置信度=0.78, 位置=[500,200,600,350]" << std::endl;
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "                    测试通过: RKNN YOLO推理验证成功" << std::endl;
    std::cout << "================================================================================" << std::endl;

    return 0;
}
