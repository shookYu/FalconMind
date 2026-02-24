/**
 * FalconMindSDK 示例06：YOLOv11目标检测推理（纯C++实现）
 *
 * 真实推理，使用ONNX Runtime C++ API
 * - 模型: YOLOv11n
 * - 推理引擎: ONNX Runtime
 * - 测试图片: ultralytics.com bus.jpg
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sys/stat.h>
#include <curl/curl.h>

#include <onnxruntime_cxx_api.h>

namespace ort = Ort;

struct ImageData {
    std::vector<uint8_t> data;
    int width;
    int height;
};

struct Detection {
    float x1, y1, x2, y2;
    float score;
    int classId;
};

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

static bool fileExists(const std::string& path) {
    struct stat buffer;
    return stat(path.c_str(), &buffer) == 0;
}

static bool downloadFile(const std::string& url, const std::string& outputPath) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;
    
    FILE* fp = fopen(outputPath.c_str(), "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        return false;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, NULL);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
    
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);
    
    return res == CURLE_OK && fileExists(outputPath);
}

// 加载PPM图片 (简单格式)
static ImageData loadPPM(const std::string& path) {
    ImageData img;
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return img;
    
    std::string magic;
    file >> magic;
    if (magic != "P6") return img;
    
    int maxVal;
    file >> img.width >> img.height >> maxVal;
    
    file.ignore(1); // 跳过换行符
    size_t size = img.width * img.height * 3;
    img.data.resize(size);
    file.read(reinterpret_cast<char*>(img.data.data()), size);
    
    return img;
}

// 保存PPM图片
static bool savePPM(const std::string& path, const ImageData& img) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) return false;
    
    file << "P6\n" << img.width << " " << img.height << "\n255\n";
    file.write(reinterpret_cast<const char*>(img.data.data()), img.data.size());
    
    return true;
}

// 绘制检测框到图片
static void drawDetection(ImageData& img, const Detection& det, const std::string& label) {
    int x1 = std::max(0, static_cast<int>(det.x1));
    int y1 = std::max(0, static_cast<int>(det.y1));
    int x2 = std::min(img.width - 1, static_cast<int>(det.x2));
    int y2 = std::min(img.height - 1, static_cast<int>(det.y2));
    
    // 边界检查
    if (x1 >= x2 || y1 >= y2) return;
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    
    // 绘制矩形框 (绿色边框)
    int thickness = 3;
    uint8_t r = 0, g = 255, b = 0;
    
    // 顶部和底部边框
    for (int x = x1; x <= x2 && x < x1 + thickness; x++) {
        for (int y = y1; y <= y2; y++) {
            if (y >= 0 && y < img.height && x >= 0 && x < img.width) {
                int idx = (y * img.width + x) * 3;
                img.data[idx] = r;
                img.data[idx + 1] = g;
                img.data[idx + 2] = b;
            }
        }
    }
    for (int x = x2 - thickness + 1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            if (y >= 0 && y < img.height && x >= 0 && x < img.width) {
                int idx = (y * img.width + x) * 3;
                img.data[idx] = r;
                img.data[idx + 1] = g;
                img.data[idx + 2] = b;
            }
        }
    }
    // 左侧和右侧边框
    for (int y = y1; y <= y2 && y < y1 + thickness; y++) {
        for (int x = x1; x <= x2; x++) {
            if (y >= 0 && y < img.height && x >= 0 && x < img.width) {
                int idx = (y * img.width + x) * 3;
                img.data[idx] = r;
                img.data[idx + 1] = g;
                img.data[idx + 2] = b;
            }
        }
    }
    for (int y = y2 - thickness + 1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            if (y >= 0 && y < img.height && x >= 0 && x < img.width) {
                int idx = (y * img.width + x) * 3;
                img.data[idx] = r;
                img.data[idx + 1] = g;
                img.data[idx + 2] = b;
            }
        }
    }
}

static std::vector<float> preprocessImage(const uint8_t* data, size_t size,
                                          int targetW, int targetH, int origW, int origH) {
    std::vector<float> output(3 * targetW * targetH);
    
    for (int c = 0; c < 3; c++) {
        for (int y = 0; y < targetH; y++) {
            for (int x = 0; x < targetW; x++) {
                int srcX = std::min(x * origW / targetW, origW - 1);
                int srcY = std::min(y * origH / targetH, origH - 1);
                int srcIdx = (srcY * origW + srcX) * 3 + (2 - c);
                
                float val = 0.0f;
                if (srcIdx >= 0 && srcIdx < static_cast<int>(size)) {
                    val = static_cast<float>(data[srcIdx]) / 255.0f;
                }
                output[c * targetW * targetH + y * targetW + x] = val - 0.5f;
            }
        }
    }
    return output;
}

static float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

static std::vector<Detection> postprocessYOLO(const float* output, const std::vector<int64_t>& shape,
                                               float confThresh, float iouThresh,
                                               int imgW, int imgH) {
    std::vector<Detection> detections;
    // YOLO11输出格式: (batch, 84, 8400)
    // 84 = 4(box) + 1(obj) + 80(classes)
    if (shape.size() < 3) return detections;
    
    int numBoxes = static_cast<int>(shape[2]);  // 8400
    int numValues = static_cast<int>(shape[1]); // 84
    
    for (int b = 0; b < numBoxes; b++) {
        // Objectness (sigmoid)
        float objectness = sigmoid(output[4 * numBoxes + b]);
        if (objectness < 0.1f) continue;
        
        // Classes (sigmoid)
        float maxScore = 0.0f;
        int maxClass = -1;
        for (int c = 0; c < 80; c++) {
            float score = sigmoid(output[(5 + c) * numBoxes + b]);
            if (score > maxScore) {
                maxScore = score;
                maxClass = c;
            }
        }
        
        float conf = objectness * maxScore;
        if (conf < confThresh) continue;
        
        // Box coordinates (already in pixel values 0-640)
        float cx = output[0 * numBoxes + b];
        float cy = output[1 * numBoxes + b];
        float w = output[2 * numBoxes + b];
        float h = output[3 * numBoxes + b];
        
        Detection det;
        det.x1 = cx - w * 0.5f;
        det.y1 = cy - h * 0.5f;
        det.x2 = cx + w * 0.5f;
        det.y2 = cy + h * 0.5f;
        det.score = conf;
        det.classId = maxClass;
        detections.push_back(det);
    }
    
    // NMS
    std::sort(detections.begin(), detections.end(), 
              [](const Detection& a, const Detection& b) { return a.score > b.score; });
    
    std::vector<bool> suppressed(detections.size(), false);
    for (size_t i = 0; i < detections.size(); i++) {
        if (suppressed[i]) continue;
        for (size_t j = i + 1; j < detections.size(); j++) {
            if (suppressed[j]) continue;
            if (detections[i].classId != detections[j].classId) continue;
            
            float x1 = std::max(detections[i].x1, detections[j].x1);
            float y1 = std::max(detections[i].y1, detections[j].y1);
            float x2 = std::min(detections[i].x2, detections[j].x2);
            float y2 = std::min(detections[i].y2, detections[j].y2);
            
            float inter = std::max(0.0f, x2 - x1) * std::max(0.0f, y2 - y1);
            float area1 = (detections[i].x2 - detections[i].x1) * (detections[i].y2 - detections[i].y1);
            float area2 = (detections[j].x2 - detections[j].x1) * (detections[j].y2 - detections[j].y1);
            float unionArea = area1 + area2 - inter;
            
            if (unionArea > 0 && inter / unionArea > iouThresh) {
                suppressed[j] = true;
            }
        }
    }
    
    std::vector<Detection> result;
    for (size_t i = 0; i < detections.size(); i++) {
        if (!suppressed[i]) result.push_back(detections[i]);
    }
    return result;
}

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "            FalconMindSDK 示例06: YOLOv11目标检测推理 (纯C++)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;
    
    // 使用共享资源目录
    std::string baseDir = "/home/shook/study/opencode/FalconMindSDK/examples/assets";
    std::string modelPath = baseDir + "/models/yolov11n_opset21.onnx";
    std::string imagePath = baseDir + "/images/bus.jpg";
    std::string outDir = "./out";
    system("mkdir -p ./out");
    
    std::cout << "[1] 检查 YOLOv11n ONNX 模型" << std::endl;
    std::cout << "    模型路径: " << modelPath << std::endl;
    
    bool modelExists = fileExists(modelPath);
    if (!modelExists) {
        std::cout << "    模型不存在，请运行: python3 create_test_model.py" << std::endl;
    } else {
        std::cout << "    模型已存在" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "[2] 检查测试图片" << std::endl;
    if (fileExists(imagePath)) {
        std::cout << "    图片已存在: " << imagePath << std::endl;
    } else {
        std::cout << "    图片不存在: " << imagePath << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "[3] 加载测试图片" << std::endl;
    // JPG转PPM (使用Python/PIL)
    std::string ppmPath = outDir + "/temp_input.ppm";
    std::string cmd = "python3 -c \"from PIL import Image; img = Image.open('" + imagePath + "'); img = img.resize((640, 480)); img.save('" + ppmPath + "')\" 2>/dev/null";
    system(cmd.c_str());
    
    ImageData image = loadPPM(ppmPath);
    if (image.data.empty()) {
        std::cout << "    错误: 无法加载图片" << std::endl;
        return 1;
    }
    std::cout << "    图片尺寸: " << image.width << "x" << image.height << std::endl;
    std::cout << std::endl;
    
    if (!modelExists) {
        std::cout << "================================================================================" << std::endl;
        std::cout << "                    模型文件不存在，请先下载模型" << std::endl;
        std::cout << "================================================================================" << std::endl;
        return 1;
    }
    
    std::cout << "[4] 初始化 ONNX Runtime" << std::endl;
    ort::Env env(ORT_LOGGING_LEVEL_WARNING, "FalconMindSDK");
    ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(4);
    sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    
    ort::Session* session = nullptr;
    try {
        session = new ort::Session(env, modelPath.c_str(), sessionOptions);
        std::cout << "    ONNX Runtime 初始化成功" << std::endl;
        std::cout << "    模型输入数: " << session->GetInputCount() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "    ONNX Runtime 加载失败: " << e.what() << std::endl;
        return 1;
    }
    std::cout << std::endl;
    
    std::cout << "[5] 预处理图片" << std::endl;
    const int inputW = 640;
    const int inputH = 640;
    std::vector<float> inputTensor = preprocessImage(
        image.data.data(), image.data.size(), inputW, inputH, image.width, image.height);
    std::cout << "    输入张量: 1x3x" << inputW << "x" << inputH << std::endl;
    std::cout << std::endl;
    
    std::cout << "[6] 执行推理" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<int64_t> inputShape = {1, 3, inputH, inputW};
    ort::Value inputOrt = ort::Value::CreateTensor<float>(
        ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault),
        inputTensor.data(), inputTensor.size(),
        inputShape.data(), inputShape.size());
    
    std::vector<std::string> inputNames;
    std::vector<std::string> outputNames;
    std::vector<const char*> inputNamesC;
    std::vector<const char*> outputNamesC;
    ort::AllocatorWithDefaultOptions allocator;
    
    for (size_t i = 0; i < session->GetInputCount(); i++) {
        inputNames.push_back(session->GetInputNameAllocated(i, allocator).get());
    }
    for (size_t i = 0; i < session->GetOutputCount(); i++) {
        outputNames.push_back(session->GetOutputNameAllocated(i, allocator).get());
    }
    
    for (const auto& name : inputNames) inputNamesC.push_back(name.c_str());
    for (const auto& name : outputNames) outputNamesC.push_back(name.c_str());
    
    std::vector<ort::Value> outputTensors = session->Run(
        ort::RunOptions{nullptr},
        inputNamesC.data(), &inputOrt, 1,
        outputNamesC.data(), outputNamesC.size());
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "    推理耗时: " << duration.count() << "ms" << std::endl;
    std::cout << std::endl;
    
    std::cout << "[7] 后处理检测结果" << std::endl;
    float* outputData = outputTensors[0].GetTensorMutableData<float>();
    auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
    
    std::vector<Detection> detections = postprocessYOLO(
        outputData, outputShape, 0.5f, 0.45f, image.width, image.height);
    std::cout << "    检测到目标数: " << detections.size() << std::endl;
    std::cout << std::endl;
    
    std::cout << "[8] 检测结果" << std::endl;
    if (!detections.empty()) {
        std::cout << "    +------------------------------------------------------------------+" << std::endl;
        std::cout << "    |  ID |     类别     |  置信度  |       边界框 (x1,y1,x2,y2)       |" << std::endl;
        std::cout << "    +-----+-------------+----------+--------------------------------+" << std::endl;
        
        for (size_t i = 0; i < detections.size(); ++i) {
            const auto& det = detections[i];
            std::string className = (det.classId >= 0 && det.classId < (int)COCO_LABELS.size()) 
                ? COCO_LABELS[det.classId] : "unknown";
            std::cout << "    | " << std::setw(2) << (i + 1) << " | " << std::setw(11) << std::left << className << " | ";
            std::cout << std::setw(8) << std::fixed << std::setprecision(4) << det.score << " | ";
            std::cout << "(" << std::setw(4) << (int)det.x1 << "," << std::setw(4) << (int)det.y1 
                      << "," << std::setw(4) << (int)det.x2 << "," << std::setw(4) << (int)det.y2 << ")" << std::endl;
        }
        std::cout << "    +------------------------------------------------------------------+" << std::endl;
    } else {
        std::cout << "    未检测到目标" << std::endl;
    }
    std::cout << std::endl;
    
    std::cout << "[8.5] 生成可视化结果" << std::endl;
    // 绘制检测框到原图
    for (const auto& det : detections) {
        drawDetection(image, det, "");
    }
    // 保存带框图片 (PPM格式)
    std::string outputPath = outDir + "/detection_result.ppm";
    if (savePPM(outputPath, image)) {
        std::cout << "    可视化结果已保存: " << outputPath << std::endl;
        // 转换为JPG
        std::string cmd = "python3 -c \"from PIL import Image; img = Image.open('" + outputPath + "'); img.save('" + outDir + "/detection_result.jpg'); print('    已转换为JPG')\" 2>/dev/null";
        system(cmd.c_str());
    }
    std::cout << std::endl;
    
    std::cout << "[9] 清理资源" << std::endl;
    delete session;
    std::cout << "    资源释放完成" << std::endl;
    std::cout << std::endl;
    
    std::cout << "================================================================================" << std::endl;
    std::cout << "                    测试完成: YOLOv11目标检测推理" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    return 0;
}
