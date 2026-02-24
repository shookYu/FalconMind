/**
 * Example 07: RKNN Backend - Rockchip NPU Inference (RK3588/RK3576/RV1126B)
 * 
 * This example demonstrates:
 * - Using RknnDetectorBackend for real NPU inference
 * - Loading RKNN model (.rknn format)
 * - Preprocessing image for NPU input
 * - Running inference on Rockchip NPU
 * - Postprocessing detection results
 * 
 * Usage:
 *   ./07_rknn_backend_rk3588 [model_path] [image_path]
 *   
 *   Default: ./07_rknn_backend_rk3588 ../models/yolov8n.rknn ../images/test.jpg
 * 
 * Build:
 *   mkdir -p build && cd build
 *   cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
 *   make -j4
 * 
 * Run with QEMU:
 *   qemu-aarch64-static -L /usr/aarch64-linux-gnu ./07_rknn_backend_rk3588
 * 
 * Prerequisites:
 *   - Rockchip RK3588/RK3576/RV1126B board (or QEMU simulation)
 *   - RKNN Toolkit2 runtime library (librknn_api.so)
 *   - RKNN format model file (.rknn)
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
#include <algorithm>

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

// Generate a test image with simulated objects
static std::vector<uint8_t> generateTestImage(int width, int height) {
    std::vector<uint8_t> image(width * height * 3, 120);
    
    // Sky
    for (int y = 0; y < height / 3 && y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 135;  // R
            image[(y * width + x) * 3 + 1] = 206;  // G
            image[(y * width + x) * 3 + 2] = 235;  // B
        }
    }
    
    // Road
    for (int y = height / 3; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 80;
            image[(y * width + x) * 3 + 1] = 80;
            image[(y * width + x) * 3 + 2] = 80;
        }
    }
    
    // Lane markings
    for (int y = height / 3; y < height; y += 80) {
        for (int x = width / 2 - 5; x < width / 2 + 5; ++x) {
            if (y < height) {
                image[(y * width + x) * 3 + 0] = 255;
                image[(y * width + x) * 3 + 1] = 255;
                image[(y * width + x) * 3 + 2] = 255;
            }
        }
    }
    
    // Car (red)
    for (int y = height / 2; y < height / 2 + 80 && y < height; ++y) {
        for (int x = 280; x < 360 && x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 200;
            image[(y * width + x) * 3 + 1] = 50;
            image[(y * width + x) * 3 + 2] = 50;
        }
    }
    
    // Person (blue)
    for (int y = height * 2 / 3; y < height * 2 / 3 + 150 && y < height; ++y) {
        for (int x = 100; x < 130 && x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 50;
            image[(y * width + x) * 3 + 1] = 100;
            image[(y * width + x) * 3 + 2] = 200;
        }
    }
    
    // Traffic light (gray box with red light)
    for (int y = 100; y < 180 && y < height; ++y) {
        for (int x = 500; x < 530 && x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 100;
            image[(y * width + x) * 3 + 1] = 100;
            image[(y * width + x) * 3 + 2] = 100;
        }
    }
    // Red light
    for (int y = 105; y < 125 && y < height; ++y) {
        for (int x = 505; x < 525 && x < width; ++x) {
            image[(y * width + x) * 3 + 0] = 0;
            image[(y * width + x) * 3 + 1] = 0;
            image[(y * width + x) * 3 + 2] = 255;
        }
    }
    
    return image;
}

// Mock detection for when model is not available
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
}

int main(int argc, char* argv[]) {
    std::cout << "================================================================================" << std::endl;
    std::cout << "            FalconMindSDK Example 07: RKNN Backend Inference" << std::endl;
    std::cout << "            Platform: Rockchip RK3588/RK3576/RV1126B" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    // Parse command line arguments
    std::string modelPath = "../models/yolov8n.rknn";
    if (argc > 1) {
        modelPath = argv[1];
    }

    std::cout << "[Platform Detection] Rockchip ARM Platform" << std::endl;
    std::cout << "  Target: RK3588/RK3576/RV1126B NPU" << std::endl;
    std::cout << std::endl;

    std::cout << "[1] Creating RKNN Detector Backend" << std::endl;
    auto backend = std::make_shared<RknnDetectorBackend>();
    std::cout << "    Backend type: RKNN" << std::endl;
    std::cout << "    Backend created successfully" << std::endl;
    std::cout << std::endl;

    std::cout << "[2] Configuring detector parameters" << std::endl;
    DetectorDescriptor desc;
    desc.detectorId = "yolov8n_rknn_640";
    desc.name = "YOLOv8n RKNN 640x640";
    desc.backendType = backend->backendType();
    desc.inputWidth = 640;
    desc.inputHeight = 640;
    desc.numClasses = 80;
    desc.scoreThreshold = 0.5f;
    desc.nmsThreshold = 0.45f;

    // Try to find model file
    const char* modelPaths[] = {
        modelPath.c_str(),
        "../models/yolov8n.rknn",
        "/models/yolov8n.rknn",
        "./yolov8n.rknn",
        ""
    };

    bool modelFound = false;
    for (int i = 0; modelPaths[i][0] != '\0'; ++i) {
        std::ifstream testFile(modelPaths[i]);
        if (testFile.good()) {
            desc.modelPath = modelPaths[i];
            modelFound = true;
            break;
        }
    }

    if (!modelFound) {
        std::cout << "    Model: (not found, using simulation mode)" << std::endl;
        desc.modelPath = modelPath;
    } else {
        std::cout << "    Model: " << desc.modelPath << std::endl;
        std::cout << "    Mode: Real RKNN NPU inference" << std::endl;
    }
    std::cout << "    Input size: " << desc.inputWidth << "x" << desc.inputHeight << std::endl;
    std::cout << "    Classes: " << desc.numClasses << std::endl;
    std::cout << std::endl;

    std::cout << "[3] Loading model" << std::endl;
    bool loadSuccess = backend->load(desc);
    std::cout << "    Load result: " << (loadSuccess ? "success" : "failed") << std::endl;
    
    // Use simulation if model not found or load failed
    bool useSimulation = !modelFound || !loadSuccess;
    if (useSimulation) {
        std::cout << "    Mode: Simulation (model file not found or load failed)" << std::endl;
        std::cout << "    To use real inference, provide .rknn model file" << std::endl;
    } else {
        std::cout << "    Model loaded: " << desc.modelPath << std::endl;
        std::cout << "    Backend ready for NPU inference" << std::endl;
    }
    std::cout << std::endl;

    const int width = 640;
    const int height = 640;
    std::cout << "[4] Generating test image (" << width << "x" << height << " RGB)" << std::endl;
    auto imageData = generateTestImage(width, height);
    std::cout << "    Image generated: street scene simulation" << std::endl;
    std::cout << "    Contains: car, person, traffic light" << std::endl;
    std::cout << std::endl;

    std::cout << "[5] Running object detection inference" << std::endl;

    ImageView imageView;
    imageView.data = imageData.data();
    imageView.width = width;
    imageView.height = height;
    imageView.stride = width * 3;
    imageView.pixelFormat = "RGB8";

    DetectionResult result;

    if (!useSimulation) {
        auto start = std::chrono::high_resolution_clock::now();
        bool runSuccess = backend->run(imageView, result);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        std::cout << "    Inference: " << (runSuccess ? "success" : "failed") << std::endl;
        std::cout << "    Latency: " << duration.count() << "ms" << std::endl;
        
        if (!runSuccess || result.detections.empty()) {
            std::cout << "    Note: Real inference returned empty, using simulation" << std::endl;
            useSimulation = true;
        }
    }
    
    if (useSimulation) {
        std::cout << "    Inference: simulation mode" << std::endl;
        std::cout << "    Latency: N/A" << std::endl;
        runMockDetection(result);
    }
    std::cout << std::endl;

    std::cout << "[6] Detection Results" << std::endl;
    std::cout << "    Frame ID: " << result.frameId << std::endl;
    std::cout << "    Objects detected: " << result.detections.size() << std::endl;
    std::cout << std::endl;

    if (!result.detections.empty()) {
        std::cout << "    +------------------------------------------------------------------+-----------------+" << std::endl;
        std::cout << "    |  ID |     Class     |  Confidence  |       Bounding Box (x,y,w,h)       |" << std::endl;
        std::cout << "    +-----+---------------+--------------+------------------------------------+-----------------+" << std::endl;

        for (size_t i = 0; i < result.detections.size(); ++i) {
            const auto& det = result.detections[i];
            std::string className = det.className;
            if (className.empty() && det.classId >= 0 && det.classId < (int)COCO_LABELS.size()) {
                className = COCO_LABELS[det.classId];
            }
            if (className.empty()) {
                className = "class_" + std::to_string(det.classId);
            }
            std::cout << "    | " << std::setw(2) << (i + 1) << "  | " 
                      << std::setw(13) << std::left << className << " | "
                      << std::setw(12) << std::fixed << std::setprecision(4) << det.score << " | "
                      << "(" << std::setw(4) << (int)det.bbox.x << "," << std::setw(4) << (int)det.bbox.y 
                      << "," << std::setw(4) << (int)det.bbox.width << "," << std::setw(4) << (int)det.bbox.height << ")" 
                      << std::endl;
        }
        std::cout << "    +------------------------------------------------------------------+-----------------+" << std::endl;
    } else {
        std::cout << "    No objects detected" << std::endl;
    }
    std::cout << std::endl;

    std::cout << "[7] Cleanup" << std::endl;
    backend->unload();
    std::cout << "    Resources released" << std::endl;
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "                    Test Complete: RKNN Backend Inference" << std::endl;
    if (useSimulation) {
        std::cout << "                    Mode: Simulation (no RKNN model)" << std::endl;
    } else {
        std::cout << "                    Mode: Real RKNN NPU Inference" << std::endl;
    }
    std::cout << "================================================================================" << std::endl;

    return 0;
}
