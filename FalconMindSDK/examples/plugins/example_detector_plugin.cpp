/**
 * @file example_detector_plugin.cpp
 * @brief 检测器插件开发示例 - 真实实现
 * 
 * 展示如何实现一个真实的检测器插件并编译成.so
 * 
 * 编译方法：
 * g++ -std=c++17 -shared -fPIC -o libyolo26_detector.so \
 *     example_detector_plugin.cpp \
 *     -I/path/to/FalconMindSDK/include \
 *     `pkg-config --cflags --libs opencv4` \
 *     -lonnxruntime
 * 
 * 使用：将编译好的.so放入plugins目录，SDK会自动加载
 */

#include "falconmind/sdk/plugin/IPlugin.h"
#include "falconmind/sdk/core/ConfigManager.h"
#include "falconmind/sdk/core/Logger.h"
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>
#include <vector>
#include <string>
#include <algorithm>

// 插件命名空间
namespace yolo26_plugin {

using namespace falconmind::sdk;
using namespace falconmind::sdk::plugin;
using namespace falconmind::sdk::perception;

/**
 * @brief YOLOv8/v26 检测器插件实现
 * 
 * 真实实现，使用ONNX Runtime进行推理
 */
class Yolo26DetectorPlugin : public IDetectorPlugin {
public:
    Yolo26DetectorPlugin() 
        : env_(ORT_LOGGING_LEVEL_WARNING, "yolo26_detector")
        , session_(nullptr)
        , confidenceThreshold_(0.5f)
        , nmsThreshold_(0.45f)
        , inputWidth_(640)
        , inputHeight_(640) {}
    
    ~Yolo26DetectorPlugin() {
        if (session_) {
            delete session_;
        }
    }
    
    // IPlugin 接口实现
    PluginMetadata getMetadata() const override {
        return {
            .name = "yolo26_detector",
            .version = "1.0.0",
            .description = "YOLOv26 object detector using ONNX Runtime",
            .author = "Your Company",
            .sdkVersion = "1.0.0",
            .type = PluginType::Detector,
            .capabilities = PluginCapability::RealTime | 
                          PluginCapability::GPUAccelerated | 
                          PluginCapability::MultiObject,
            .dependencies = {},
            .supportedModels = {"yolov8n", "yolov8s", "yolov8m", "yolov8l", "yolov8x"},
            .supportedPlatforms = {"x86_64", "aarch64"}
        };
    }
    
    bool initialize(const core::ConfigManager& config) override {
        core::LOG_INFO("Yolo26Detector") << "Initializing YOLOv26 detector plugin";
        
        // 从配置读取参数
        modelPath_ = config.get<std::string>("model_path", "models/yolov8n.onnx");
        confidenceThreshold_ = config.get<float>("confidence_threshold", 0.5f);
        nmsThreshold_ = config.get<float>("nms_threshold", 0.45f);
        inputWidth_ = config.get<int>("input_width", 640);
        inputHeight_ = config.get<int>("input_height", 640);
        
        // 加载类别名称
        loadClassNames();
        
        core::LOG_INFO("Yolo26Detector") << "Plugin initialized successfully";
        return true;
    }
    
    void shutdown() override {
        core::LOG_INFO("Yolo26Detector") << "Shutting down YOLOv26 detector";
        
        if (session_) {
            delete session_;
            session_ = nullptr;
        }
    }
    
    PluginState getState() const override {
        return session_ ? PluginState::Active : PluginState::Error;
    }
    
    // IDetectorPlugin 接口实现
    bool loadModel(const std::string& modelPath, 
                   const std::string& device = "auto") override {
        core::LOG_INFO("Yolo26Detector") << "Loading model from: " << modelPath;
        
        try {
            Ort::SessionOptions sessionOptions;
            
            // 设置线程数
            sessionOptions.SetIntraOpNumThreads(4);
            sessionOptions.SetInterOpNumThreads(4);
            
            // 根据设备选择执行提供者
            if (device == "cuda" || device == "auto") {
                // 尝试使用CUDA
                OrtCUDAProviderOptions cudaOptions;
                cudaOptions.device_id = 0;
                cudaOptions.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
                cudaOptions.gpu_mem_limit = SIZE_MAX;
                cudaOptions.arena_extend_strategy = 0;
                cudaOptions.do_copy_in_default_stream = 1;
                
                try {
                    sessionOptions.AppendExecutionProvider_CUDA(cudaOptions);
                    core::LOG_INFO("Yolo26Detector") << "Using CUDA execution provider";
                } catch (...) {
                    core::LOG_WARN("Yolo26Detector") << "CUDA not available, using CPU";
                }
            }
            
            // 创建会话
            session_ = new Ort::Session(env_, modelPath.c_str(), sessionOptions);
            
            // 验证模型输入输出
            Ort::AllocatorWithDefaultOptions allocator;
            
            // 获取输入信息
            size_t numInputs = session_->GetInputCount();
            size_t numOutputs = session_->GetOutputCount();
            
            core::LOG_INFO("Yolo26Detector") << "Model loaded: " << numInputs 
                      << " inputs, " << numOutputs << " outputs";
            
            return true;
            
        } catch (const Ort::Exception& e) {
            core::LOG_ERROR("Yolo26Detector") << "ONNX Runtime error: " << e.what();
            return false;
        } catch (const std::exception& e) {
            core::LOG_ERROR("Yolo26Detector") << "Error loading model: " << e.what();
            return false;
        }
    }
    
    DetectionResult detect(const ImageView& image) override {
        DetectionResult result;
        
        if (!session_) {
            core::LOG_ERROR("Yolo26Detector") << "Model not loaded";
            return result;
        }
        
        try {
            // 将ImageView转换为OpenCV Mat
            cv::Mat inputImage;
            convertImageViewToMat(image, inputImage);
            
            // 预处理
            cv::Mat blob = preprocess(inputImage);
            
            // 创建输入tensor
            std::vector<int64_t> inputShape = {1, 3, inputHeight_, inputWidth_};
            size_t inputSize = 1 * 3 * inputHeight_ * inputWidth_;
            
            Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
                memoryInfo_, 
                blob.ptr<float>(), 
                inputSize, 
                inputShape.data(), 
                inputShape.size()
            );
            
            // 运行推理
            Ort::AllocatorWithDefaultOptions allocator;
            const char* inputNames[] = {"images"};
            const char* outputNames[] = {"output0"};
            
            auto outputTensors = session_->Run(
                Ort::RunOptions{nullptr},
                inputNames, &inputTensor, 1,
                outputNames, 1
            );
            
            // 后处理
            result = postprocess(outputTensors[0], inputImage.cols, inputImage.rows);
            
        } catch (const std::exception& e) {
            core::LOG_ERROR("Yolo26Detector") << "Detection error: " << e.what();
        }
        
        return result;
    }
    
    std::vector<std::string> getSupportedClasses() const override {
        return classNames_;
    }
    
    void setConfidenceThreshold(float threshold) override {
        confidenceThreshold_ = threshold;
    }
    
    void setInputSize(int width, int height) override {
        inputWidth_ = width;
        inputHeight_ = height;
    }
    
    std::map<std::string, std::string> getModelInfo() const override {
        return {
            {"model_path", modelPath_},
            {"input_size", std::to_string(inputWidth_) + "x" + std::to_string(inputHeight_)},
            {"confidence_threshold", std::to_string(confidenceThreshold_)},
            {"nms_threshold", std::to_string(nmsThreshold_)},
            {"num_classes", std::to_string(classNames_.size())}
        };
    }

private:
    Ort::Env env_;
    Ort::Session* session_;
    Ort::MemoryInfo memoryInfo_{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};
    
    std::string modelPath_;
    float confidenceThreshold_;
    float nmsThreshold_;
    int inputWidth_;
    int inputHeight_;
    std::vector<std::string> classNames_;
    
    void loadClassNames() {
        // COCO 80类
        classNames_ = {
            "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
            "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
            "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
            "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
            "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
            "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
            "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake",
            "chair", "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop",
            "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
            "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
        };
    }
    
    void convertImageViewToMat(const ImageView& image, cv::Mat& mat) {
        // 根据格式创建Mat
        int cvType = (image.channels == 3) ? CV_8UC3 : CV_8UC1;
        mat = cv::Mat(image.height, image.width, cvType, 
                     const_cast<void*>(image.data), image.stride);
        
        if (image.pixelFormat == "RGB8" && image.channels == 3) {
            cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
        }
    }
    
    cv::Mat preprocess(const cv::Mat& image) {
        // Letterbox resize
        float scale = std::min(
            static_cast<float>(inputWidth_) / image.cols,
            static_cast<float>(inputHeight_) / image.rows
        );
        
        int newWidth = static_cast<int>(image.cols * scale);
        int newHeight = static_cast<int>(image.rows * scale);
        
        cv::Mat resized;
        cv::resize(image, resized, cv::Size(newWidth, newHeight));
        
        // 创建letterbox画布
        cv::Mat letterbox(inputHeight_, inputWidth_, CV_8UC3, cv::Scalar(114, 114, 114));
        
        int xOffset = (inputWidth_ - newWidth) / 2;
        int yOffset = (inputHeight_ - newHeight) / 2;
        
        resized.copyTo(letterbox(cv::Rect(xOffset, yOffset, newWidth, newHeight)));
        
        // 归一化并转换格式
        cv::Mat blob;
        letterbox.convertTo(blob, CV_32F, 1.0 / 255.0);
        
        // HWC to CHW
        cv::dnn::blobFromImage(letterbox, blob, 1.0/255.0, cv::Size(), 
                              cv::Scalar(), true, false);
        
        return blob;
    }
    
    DetectionResult postprocess(const Ort::Value& output, int origWidth, int origHeight) {
        DetectionResult result;
        
        // 获取输出数据
        auto shape = output.GetTensorTypeAndShapeInfo().GetShape();
        const float* data = output.GetTensorData<float>();
        
        int numDetections = shape[1];
        int numAttributes = shape[2];
        
        std::vector<Detection> detections;
        std::vector<cv::Rect> boxes;
        std::vector<float> confidences;
        std::vector<int> classIds;
        
        // 解析检测结果
        for (int i = 0; i < numDetections; ++i) {
            const float* detection = data + i * numAttributes;
            
            // 边界框 (xywh中心格式)
            float cx = detection[0];
            float cy = detection[1];
            float w = detection[2];
            float h = detection[3];
            
            // 找到最大置信度的类别
            float maxConf = 0;
            int maxClassId = 0;
            
            for (int j = 5; j < numAttributes; ++j) {
                if (detection[j] > maxConf) {
                    maxConf = detection[j];
                    maxClassId = j - 5;
                }
            }
            
            float confidence = detection[4] * maxConf;
            
            if (confidence > confidenceThreshold_) {
                // 转换为左上角格式
                int x = static_cast<int>(cx - w / 2);
                int y = static_cast<int>(cy - h / 2);
                int width = static_cast<int>(w);
                int height = static_cast<int>(h);
                
                boxes.push_back(cv::Rect(x, y, width, height));
                confidences.push_back(confidence);
                classIds.push_back(maxClassId);
            }
        }
        
        // NMS
        std::vector<int> indices;
        cv::dnn::NMSBoxes(boxes, confidences, confidenceThreshold_, nmsThreshold_, indices);
        
        // 构建结果
        for (int idx : indices) {
            Detection det;
            det.bbox.x = boxes[idx].x;
            det.bbox.y = boxes[idx].y;
            det.bbox.width = boxes[idx].width;
            det.bbox.height = boxes[idx].height;
            det.confidence = confidences[idx];
            det.classId = classIds[idx];
            det.className = classNames_[classIds[idx]];
            
            result.detections.push_back(det);
        }
        
        return result;
    }
};

} // namespace yolo26_plugin

// 导出插件
EXPORT_PLUGIN(yolo26_plugin::Yolo26DetectorPlugin)
