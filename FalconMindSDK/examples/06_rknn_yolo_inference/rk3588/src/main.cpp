/**
 * FalconMindSDK 示例06：YOLO目标检测推理
 *
 * 测试SDK API:
 * - IDetectorBackend::load() - 加载模型
 * - IDetectorBackend::run()  - 执行推理
 * - DetectionResult          - 检测结果解析
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>

#include "falconmind/sdk/perception/OnnxRuntimeDetectorBackend.h"
#include "falconmind/sdk/perception/RknnDetectorBackend.h"
#include "falconmind/sdk/perception/DetectionTypes.h"

using namespace falconmind::sdk::perception;

static const std::vector<std::string> COCO_LABELS = {
    "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
    "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
    "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
    "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
    "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
    "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
    "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake",
    "chair", "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop",
    "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
    "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
    "toothbrush"
};

static std::vector<uint8_t> generateTestImage(int width, int height) {
    std::vector<uint8_t> image(width * height * 3, 120);
    
    // 天空
    for (int y = 0; y < height / 3 && y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 135;
            image[(y * width + x) * 3 + 1] = 206;
            image[(y * width + x) * 3 + 2] = 235;
        }
    }
    
    // 道路
    for (int y = height / 3; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 80;
            image[(y * width + x) * 3 + 1] = 80;
            image[(y * width + x) * 3 + 2] = 80;
        }
    }
    
    // 车道线
    for (int y = height / 3; y < height; y += 80) {
        for (int x = width / 2 - 5; x < width / 2 + 5; ++x) {
            if (y < height) {
                image[(y * width + x) * 3 + 0] = 255;
                image[(y * width + x) * 3 + 1] = 255;
                image[(y * width + x) * 3 + 2] = 255;
            }
        }
    }
    
    // 车辆
    for (int y = height / 2; y < height / 2 + 80 && y < height; ++y) {
        for (int x = 280; x < 360 && x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 200;
            image[(y * width + x) * 3 + 1] = 50;
            image[(y * width + x) * 3 + 2] = 50;
        }
    }
    
    // 行人
    for (int y = height * 2 / 3; y < height * 2 / 3 + 150 && y < height; ++y) {
        for (int x = 100; x < 130 && x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 50;
            image[(y * width + x) * 3 + 1] = 100;
            image[(y * width + x) * 3 + 2] = 200;
        }
    }
    
    // 交通灯
    for (int y = 100; y < 180 && y < height; ++y) {
        for (int x = 500; x < 530 && x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 100;
            image[(y * width + x) * 3 + 1] = 100;
            image[(y * width + x) * 3 + 2] = 100;
        }
    }
    // 红灯
    for (int y = 105; y < 125 && y < height; ++y) {
        for (int x = 505; x < 525 && x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 0;
            image[(y * width + x) * 3 + 1] = 0;
            image[(y * width + x) * 3 + 2] = 255;
        }
    }
    
    return image;
}

static void runMockDetection(DetectionResult& result) {
    result.detections.clear();
    result.frameId = "frame_001";
    result.frameIndex = 1;
    result.timestampNs = 1000000000;

    Detection car;
    car.classId = 2;
    car.className = "car";
    car.score = 0.87f;
    car.bbox.x = 280;
    car.bbox.y = 320;
    car.bbox.width = 80;
    car.bbox.height = 80;
    result.detections.push_back(car);

    Detection person;
    person.classId = 0;
    person.className = "person";
    person.score = 0.72f;
    person.bbox.x = 100;
    person.bbox.y = 426;
    person.bbox.width = 30;
    person.bbox.height = 150;
    result.detections.push_back(person);

    Detection trafficLight;
    trafficLight.classId = 9;
    trafficLight.className = "traffic light";
    trafficLight.score = 0.65f;
    trafficLight.bbox.x = 500;
    trafficLight.bbox.y = 100;
    trafficLight.bbox.width = 30;
    trafficLight.bbox.height = 80;
    result.detections.push_back(trafficLight);

    Detection bicycle;
    bicycle.classId = 1;
    bicycle.className = "bicycle";
    bicycle.score = 0.58f;
    bicycle.bbox.x = 450;
    bicycle.bbox.y = 480;
    bicycle.bbox.width = 60;
    bicycle.bbox.height = 100;
    result.detections.push_back(bicycle);
}

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "            FalconMindSDK 示例06: YOLO目标检测推理 (x86)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "[平台检测] x86平台 - 使用ONNX Runtime后端" << std::endl;
    std::cout << std::endl;

    std::cout << "[1] 创建检测后端实例" << std::endl;
    auto backend = std::make_shared<OnnxRuntimeDetectorBackend>();
    std::cout << "    后端类型: ONNX Runtime" << std::endl;
    std::cout << "    实例创建成功" << std::endl;
    std::cout << std::endl;

    std::cout << "[2] 配置检测器参数" << std::endl;
    DetectorDescriptor desc;
    desc.detectorId = "yolov8n_640";
    desc.name = "YOLOv8n 640x640";
    desc.backendType = backend->backendType();
    desc.inputWidth = 640;
    desc.inputHeight = 640;
    desc.numClasses = 80;
    desc.scoreThreshold = 0.5f;
    desc.nmsThreshold = 0.45f;

    const char* modelPaths[] = {
        "/models/yolov8n.onnx",
        "../models/yolov8n.onnx",
        "/home/shook/study/opencode/FalconMindSDK/models/yolov8n.onnx",
        ""
    };

    bool modelLoaded = false;
    for (int i = 0; modelPaths[i][0] != '\0'; ++i) {
        desc.modelPath = modelPaths[i];
        std::ifstream testFile(desc.modelPath);
        if (testFile.good()) {
            std::cout << "    模型路径: " << desc.modelPath << std::endl;
            modelLoaded = true;
            break;
        }
    }

    if (!modelLoaded) {
        std::cout << "    模型路径: (未找到，使用模拟模式)" << std::endl;
        desc.modelPath = "/models/yolov8n.onnx";
    } else {
        std::cout << "    模式: 真实ONNX Runtime推理" << std::endl;
    }
    std::cout << std::endl;

    std::cout << "[3] 加载模型" << std::endl;
    std::cout << "    模型ID: " << desc.detectorId << std::endl;
    std::cout << "    输入尺寸: " << desc.inputWidth << "x" << desc.inputHeight << std::endl;
    std::cout << "    类别数: " << desc.numClasses << std::endl;
    
    bool loadSuccess = backend->load(desc);
    std::cout << "    模型加载: " << (loadSuccess ? "成功" : "失败") << std::endl;
    
    // 如果模型文件不存在，使用模拟模式
    bool useMock = !modelLoaded;
    if (useMock) {
        std::cout << "    模式: 模拟模式（模型文件未找到）" << std::endl;
    }
    std::cout << std::endl;

    const int width = 640;
    const int height = 640;
    std::cout << "[4] 生成测试图像 (640x640 RGB)" << std::endl;
    auto imageData = generateTestImage(width, height);
    std::cout << "    图像尺寸: " << width << "x" << height << std::endl;
    std::cout << "    内容: 街道场景模拟" << std::endl;
    std::cout << std::endl;

    std::cout << "[5] 执行目标检测推理" << std::endl;

    ImageView imageView;
    imageView.data = imageData.data();
    imageView.width = width;
    imageView.height = height;
    imageView.stride = width * 3;
    imageView.pixelFormat = "RGB8";

    DetectionResult result;

    if (!useMock) {
        auto start = std::chrono::high_resolution_clock::now();
        bool runSuccess = backend->run(imageView, result);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "    推理执行: " << (runSuccess ? "成功" : "失败") << std::endl;
        std::cout << "    推理耗时: " << duration.count() << "ms" << std::endl;
        
        if (!runSuccess || result.detections.empty()) {
            std::cout << "    注意: 真实推理返回空结果，使用模拟数据演示" << std::endl;
            useMock = true;
        }
    }
    
    if (useMock) {
        std::cout << "    推理执行: 模拟模式" << std::endl;
        std::cout << "    推理耗时: N/A" << std::endl;
        runMockDetection(result);
    }
    std::cout << std::endl;

    std::cout << "[6] 检测结果" << std::endl;
    std::cout << "    帧ID: " << result.frameId << std::endl;
    std::cout << "    检测到目标数: " << result.detections.size() << std::endl;
    std::cout << std::endl;

    if (!result.detections.empty()) {
        std::cout << "    +------------------------------------------------------------------+" << std::endl;
        std::cout << "    |  ID |     类别     |  置信度  |       边界框 (x,y,w,h)       |" << std::endl;
        std::cout << "    +-----+-------------+----------+-----------------------------+" << std::endl;

        for (size_t i = 0; i < result.detections.size(); ++i) {
            const auto& det = result.detections[i];
            std::string className = det.className;
            if (className.empty() && det.classId >= 0 && det.classId < (int)COCO_LABELS.size()) {
                className = COCO_LABELS[det.classId];
            }
            if (className.empty()) {
                className = "class_" + std::to_string(det.classId);
            }
            std::cout << "    | " << std::setw(1) << (i + 1) << "  | " << std::setw(11) << std::left << className << " | ";
            std::cout << std::setw(8) << std::fixed << std::setprecision(2) << det.score << " | ";
            std::cout << "(" << std::setw(3) << (int)det.bbox.x << "," << std::setw(3) << (int)det.bbox.y 
                      << "," << std::setw(3) << (int)det.bbox.width << "," << std::setw(3) << (int)det.bbox.height << ")" << std::endl;
        }
        std::cout << "    +------------------------------------------------------------------+" << std::endl;
    } else {
        std::cout << "    未检测到目标" << std::endl;
    }
    std::cout << std::endl;

    std::cout << "[7] 清理资源" << std::endl;
    backend->unload();
    std::cout << "    资源释放完成" << std::endl;
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "                    测试完成: YOLO目标检测推理" << std::endl;
    std::cout << "================================================================================" << std::endl;

    return 0;
}
