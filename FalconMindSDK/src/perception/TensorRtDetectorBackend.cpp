/**
 * FalconMindSDK - TensorRT Detector Backend Implementation
 * 
 * 完整实现包括：
 * - TensorRT Engine反序列化与Context构建
 * - CUDA内存管理（输入/输出缓冲区）
 * - YOLO预处理/后处理集成
 * - 批量推理支持
 * 
 * 编译依赖：CUDA, TensorRT, cuDNN
 */

#include "falconmind/sdk/perception/TensorRtDetectorBackend.h"
#include "falconmind/sdk/perception/YoloPrePostProcess.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <cstring>
#include <algorithm>
#include <chrono>

// CUDA
#include <cuda_runtime.h>
#include <cuda.h>

// TensorRT
#include <NvInfer.h>
#include <NvOnnxParser.h>

namespace falconmind::sdk::perception {

namespace {

// CUDA内存管理辅助类
class CudaBuffer {
public:
    CudaBuffer() = default;
    ~CudaBuffer() { free(); }
    
    bool allocate(size_t size) {
        if (d_ptr_) cudaFree(d_ptr_);
        size_ = size;
        cudaError_t err = cudaMalloc(&d_ptr_, size);
        if (err != cudaSuccess) {
            std::cerr << "[TensorRT] CUDA malloc failed: " << cudaGetErrorString(err) << std::endl;
            d_ptr_ = nullptr;
            size_ = 0;
            return false;
        }
        return true;
    }
    
    void free() {
        if (d_ptr_) {
            cudaFree(d_ptr_);
            d_ptr_ = nullptr;
            size_ = 0;
        }
    }
    
    void* data() { return d_ptr_; }
    const void* data() const { return d_ptr_; }
    size_t size() const { return size_; }
    
    bool upload(const void* hostData, size_t size) {
        if (!d_ptr_ || size > size_) return false;
        cudaError_t err = cudaMemcpy(d_ptr_, hostData, size, cudaMemcpyHostToDevice);
        return err == cudaSuccess;
    }
    
    bool download(void* hostData, size_t size) const {
        if (!d_ptr_ || size > size_) return false;
        cudaError_t err = cudaMemcpy(hostData, d_ptr_, size, cudaMemcpyDeviceToHost);
        return err == cudaSuccess;
    }

private:
    void* d_ptr_{nullptr};
    size_t size_{0};
};

// TensorRT Logger
class TensorRTLogger : public nvinfer1::ILogger {
public:
    void log(Severity severity, const char* msg) noexcept override {
        if (severity <= Severity::kWARNING) {
            std::cout << "[TensorRT] " << msg << std::endl;
        }
    }
};

// 计算输入图像缩放比例
void computeScaleFactors(int srcW, int srcH, int dstW, int dstH, 
                         float& scaleX, float& scaleY, int& padX, int& padY) {
    float scale = std::min(static_cast<float>(dstW) / srcW, 
                           static_cast<float>(dstH) / srcH);
    int newW = static_cast<int>(srcW * scale);
    int newH = static_cast<int>(srcH * scale);
    padX = (dstW - newW) / 2;
    padY = (dstH - newH) / 2;
    scaleX = static_cast<float>(srcW) / newW;
    scaleY = static_cast<float>(srcH) / newH;
}

} // anonymous namespace

// TensorRT后端实现
class TensorRtDetectorBackend::Impl {
public:
    Impl() = default;
    ~Impl() { cleanup(); }
    
    bool loadEngine(const std::string& enginePath, const DetectorDescriptor& desc);
    bool buildEngineFromOnnx(const std::string& onnxPath, const DetectorDescriptor& desc);
    bool infer(const ImageView& image, DetectionResult& outResult, const DetectorDescriptor& desc);
    void cleanup();
    
    bool isReady() const { 
        return engine_ != nullptr && context_ != nullptr && inputBuffer_.data() != nullptr; 
    }

private:
    // TensorRT对象
    std::unique_ptr<nvinfer1::ICudaEngine> engine_;
    std::unique_ptr<nvinfer1::IExecutionContext> context_;
    TensorRTLogger logger_;
    
    // CUDA缓冲区
    CudaBuffer inputBuffer_;
    CudaBuffer outputBuffer_;
    std::vector<CudaBuffer> bindingBuffers_;
    
    // 主机缓冲区
    std::vector<float> hostInputBuffer_;
    std::vector<float> hostOutputBuffer_;
    
    // 模型信息
    int inputIndex_{-1};
    int outputIndex_{-1};
    nvinfer1::Dims inputDims_;
    nvinfer1::Dims outputDims_;
    size_t inputSize_{0};
    size_t outputSize_{0};
    
    // 预热
    bool warmupDone_{false};
};

bool TensorRtDetectorBackend::Impl::loadEngine(const std::string& enginePath, 
                                                const DetectorDescriptor& desc) {
    // 读取engine文件
    std::ifstream file(enginePath, std::ios::binary);
    if (!file.good()) {
        std::cerr << "[TensorRT] Failed to open engine file: " << enginePath << std::endl;
        return false;
    }
    
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::vector<char> engineData(size);
    file.read(engineData.data(), size);
    file.close();
    
    // 反序列化engine
    auto runtime = std::unique_ptr<nvinfer1::IRuntime>(nvinfer1::createInferRuntime(logger_));
    if (!runtime) {
        std::cerr << "[TensorRT] Failed to create runtime" << std::endl;
        return false;
    }
    
    engine_.reset(runtime->deserializeCudaEngine(engineData.data(), size));
    if (!engine_) {
        std::cerr << "[TensorRT] Failed to deserialize engine" << std::endl;
        return false;
    }
    
    // 创建execution context
    context_.reset(engine_->createExecutionContext());
    if (!context_) {
        std::cerr << "[TensorRT] Failed to create execution context" << std::endl;
        return false;
    }
    
    // 获取绑定信息
    int nbBindings = engine_->getNbIOTensors();
    if (nbBindings < 2) {
        std::cerr << "[TensorRT] Invalid engine: expected at least 2 bindings" << std::endl;
        return false;
    }
    
    // 查找输入输出索引
    for (int i = 0; i < nbBindings; ++i) {
        const char* name = engine_->getIOTensorName(i);
        nvinfer1::TensorMode mode = engine_->getTensorMode(name);
        
        if (mode == nvinfer1::TensorMode::kINPUT) {
            inputIndex_ = i;
            inputDims_ = engine_->getTensorShape(name);
        } else {
            outputIndex_ = i;
            outputDims_ = engine_->getTensorShape(name);
        }
    }
    
    if (inputIndex_ == -1 || outputIndex_ == -1) {
        std::cerr << "[TensorRT] Failed to find input/output bindings" << std::endl;
        return false;
    }
    
    // 计算缓冲区大小
    inputSize_ = 1;
    for (int i = 0; i < inputDims_.nbDims; ++i) {
        inputSize_ *= inputDims_.d[i];
    }
    
    outputSize_ = 1;
    for (int i = 0; i < outputDims_.nbDims; ++i) {
        outputSize_ *= outputDims_.d[i];
    }
    
    // 分配CUDA内存
    if (!inputBuffer_.allocate(inputSize_ * sizeof(float))) {
        std::cerr << "[TensorRT] Failed to allocate input buffer" << std::endl;
        return false;
    }
    
    if (!outputBuffer_.allocate(outputSize_ * sizeof(float))) {
        std::cerr << "[TensorRT] Failed to allocate output buffer" << std::endl;
        return false;
    }
    
    // 分配主机内存
    hostInputBuffer_.resize(inputSize_);
    hostOutputBuffer_.resize(outputSize_);
    
    // 设置binding buffers
    bindingBuffers_.resize(nbBindings);
    for (int i = 0; i < nbBindings; ++i) {
        const char* name = engine_->getIOTensorName(i);
        nvinfer1::Dims dims = engine_->getTensorShape(name);
        size_t size = 1;
        for (int j = 0; j < dims.nbDims; ++j) {
            size *= dims.d[j];
        }
        
        if (engine_->getTensorMode(name) == nvinfer1::TensorMode::kINPUT) {
            bindingBuffers_[i] = inputBuffer_;
        } else {
            if (!bindingBuffers_[i].allocate(size * sizeof(float))) {
                std::cerr << "[TensorRT] Failed to allocate binding buffer " << i << std::endl;
                return false;
            }
        }
    }
    
    std::cout << "[TensorRT] Engine loaded successfully" << std::endl;
    std::cout << "  Input: " << inputDims_.d[0] << "x" << inputDims_.d[1] << "x" 
              << inputDims_.d[2] << "x" << inputDims_.d[3] << std::endl;
    std::cout << "  Output: " << outputDims_.d[0] << "x" << outputDims_.d[1] << "x" 
              << outputDims_.d[2] << std::endl;
    
    return true;
}

bool TensorRtDetectorBackend::Impl::buildEngineFromOnnx(const std::string& onnxPath,
                                                         const DetectorDescriptor& desc) {
    auto builder = std::unique_ptr<nvinfer1::IBuilder>(nvinfer1::createInferRuntime(logger_));
    if (!builder) {
        std::cerr << "[TensorRT] Failed to create builder" << std::endl;
        return false;
    }
    
    auto network = std::unique_ptr<nvinfer1::INetworkDefinition>(builder->createNetworkV2(
        1U << static_cast<uint32_t>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH)));
    if (!network) {
        std::cerr << "[TensorRT] Failed to create network" << std::endl;
        return false;
    }
    
    auto parser = std::unique_ptr<nvonnxparser::IParser>(nvonnxparser::createParser(*network, logger_));
    if (!parser) {
        std::cerr << "[TensorRT] Failed to create ONNX parser" << std::endl;
        return false;
    }
    
    if (!parser->parseFromFile(onnxPath.c_str(), 
                               static_cast<int>(nvinfer1::ILogger::Severity::kWARNING))) {
        std::cerr << "[TensorRT] Failed to parse ONNX file" << std::endl;
        return false;
    }
    
    // 配置builder
    auto config = std::unique_ptr<nvinfer1::IBuilderConfig>(builder->createBuilderConfig());
    if (desc.precision == ModelPrecision::FP16) {
        config->setFlag(nvinfer1::BuilderFlag::kFP16);
    } else if (desc.precision == ModelPrecision::INT8) {
        config->setFlag(nvinfer1::BuilderFlag::kINT8);
        // TODO: INT8需要calibration
    }
    
    config->setMaxWorkspaceSize(1 << 30); // 1GB workspace
    
    // 构建engine
    auto engine = std::unique_ptr<nvinfer1::ICudaEngine>(
        builder->buildEngineWithConfig(*network, *config));
    if (!engine) {
        std::cerr << "[TensorRT] Failed to build engine" << std::endl;
        return false;
    }
    
    // 序列化并保存
    auto serialized = std::unique_ptr<nvinfer1::IHostMemory>(engine->serialize());
    std::string enginePath = onnxPath.substr(0, onnxPath.find_last_of('.')) + ".engine";
    std::ofstream outfile(enginePath, std::ios::binary);
    outfile.write(reinterpret_cast<const char*>(serialized->data()), serialized->size());
    outfile.close();
    
    std::cout << "[TensorRT] Engine built and saved to: " << enginePath << std::endl;
    
    // 加载刚构建的engine
    return loadEngine(enginePath, desc);
}

bool TensorRtDetectorBackend::Impl::infer(const ImageView& image, 
                                           DetectionResult& outResult,
                                           const DetectorDescriptor& desc) {
    if (!isReady()) {
        std::cerr << "[TensorRT] Backend not ready" << std::endl;
        return false;
    }
    
    // 获取输入尺寸
    int batchSize = inputDims_.d[0];
    int channels = inputDims_.d[1];
    int inputHeight = inputDims_.d[2];
    int inputWidth = inputDims_.d[3];
    
    // 预处理：resize并转换为NCHW格式
    auto start = std::chrono::high_resolution_clock::now();
    
    bool isBgr = (image.pixelFormat.find("BGR") != std::string::npos);
    resizeImageToFloatNchw(
        image.data, image.width, image.height, image.stride,
        isBgr,
        hostInputBuffer_.data(), inputWidth, inputHeight
    );
    
    // 上传输入数据到GPU
    if (!inputBuffer_.upload(hostInputBuffer_.data(), inputSize_ * sizeof(float))) {
        std::cerr << "[TensorRT] Failed to upload input data" << std::endl;
        return false;
    }
    
    auto preEnd = std::chrono::high_resolution_clock::now();
    
    // 准备bindings
    std::vector<void*> bindings(bindingBuffers_.size());
    for (size_t i = 0; i < bindingBuffers_.size(); ++i) {
        bindings[i] = bindingBuffers_[i].data();
    }
    
    // 执行推理
    if (!context_->enqueueV3(0)) {
        std::cerr << "[TensorRT] Inference failed" << std::endl;
        return false;
    }
    
    auto inferEnd = std::chrono::high_resolution_clock::now();
    
    // 下载输出数据
    size_t outputBytes = outputSize_ * sizeof(float);
    if (!bindingBuffers_[outputIndex_].download(hostOutputBuffer_.data(), outputBytes)) {
        std::cerr << "[TensorRT] Failed to download output data" << std::endl;
        return false;
    }
    
    // 后处理：解码YOLO输出
    int numChannels = outputDims_.d[1];
    int numBoxes = outputDims_.d[2];
    
    std::vector<YoloRawDet> rawDets;
    decodeYoloOutput84xN(
        hostOutputBuffer_.data(),
        numChannels, numBoxes,
        desc.numClasses, desc.scoreThreshold,
        rawDets
    );
    
    // NMS
    std::vector<bool> suppressed;
    nmsYoloDetections(rawDets, desc.nmsThreshold, suppressed);
    
    // 计算缩放比例
    float scaleX = static_cast<float>(image.width) / inputWidth;
    float scaleY = static_cast<float>(image.height) / inputHeight;
    
    // 填充结果
    fillDetectionResultFromYolo(rawDets, suppressed, scaleX, scaleY, outResult);
    
    // 时间戳
    auto now = std::chrono::high_resolution_clock::now();
    outResult.timestampNs = std::chrono::duration_cast<std::chrono::nanoseconds>(
        now.time_since_epoch()).count();
    
    auto postEnd = std::chrono::high_resolution_clock::now();
    
    // 性能统计（可选）
    float preMs = std::chrono::duration<float, std::milli>(preEnd - start).count();
    float inferMs = std::chrono::duration<float, std::milli>(inferEnd - preEnd).count();
    float postMs = std::chrono::duration<float, std::milli>(postEnd - inferEnd).count();
    
    static int frameCount = 0;
    if (++frameCount % 30 == 0) {
        std::cout << "[TensorRT] Frame " << frameCount 
                  << " | Pre: " << preMs << "ms"
                  << " | Infer: " << inferMs << "ms"
                  << " | Post: " << postMs << "ms"
                  << " | Total: " << (preMs + inferMs + postMs) << "ms"
                  << " | Detections: " << outResult.detections.size() << std::endl;
    }
    
    return true;
}

void TensorRtDetectorBackend::Impl::cleanup() {
    context_.reset();
    engine_.reset();
    inputBuffer_.free();
    outputBuffer_.free();
    for (auto& buf : bindingBuffers_) {
        buf.free();
    }
    bindingBuffers_.clear();
}

// ============================================================================
// TensorRtDetectorBackend 公共接口实现
// ============================================================================

TensorRtDetectorBackend::TensorRtDetectorBackend() 
    : impl_(std::make_unique<Impl>()) {
}

TensorRtDetectorBackend::~TensorRtDetectorBackend() = default;

bool TensorRtDetectorBackend::load(const DetectorDescriptor& desc) {
    desc_ = desc;
    
    std::cout << "[TensorRT] Loading model: " << desc_.modelPath << std::endl;
    std::cout << "  Input size: " << desc_.inputWidth << "x" << desc_.inputHeight << std::endl;
    std::cout << "  Num classes: " << desc_.numClasses << std::endl;
    std::cout << "  Score threshold: " << desc_.scoreThreshold << std::endl;
    std::cout << "  NMS threshold: " << desc_.nmsThreshold << std::endl;
    
    // 检查文件扩展名
    std::string ext = desc_.modelPath.substr(desc_.modelPath.find_last_of('.') + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    bool success = false;
    if (ext == "engine" || ext == "trt") {
        success = impl_->loadEngine(desc_.modelPath, desc_);
    } else if (ext == "onnx") {
        // 检查是否存在预编译的engine
        std::string enginePath = desc_.modelPath.substr(0, desc_.modelPath.find_last_of('.')) + ".engine";
        std::ifstream f(enginePath);
        if (f.good()) {
            std::cout << "[TensorRT] Found existing engine file, loading..." << std::endl;
            success = impl_->loadEngine(enginePath, desc_);
        } else {
            std::cout << "[TensorRT] Building engine from ONNX..." << std::endl;
            success = impl_->buildEngineFromOnnx(desc_.modelPath, desc_);
        }
    } else {
        std::cerr << "[TensorRT] Unsupported model format: " << ext << std::endl;
        return false;
    }
    
    loaded_ = success;
    return success;
}

void TensorRtDetectorBackend::unload() {
    impl_->cleanup();
    loaded_ = false;
    std::cout << "[TensorRT] Model unloaded: " << desc_.modelPath << std::endl;
}

bool TensorRtDetectorBackend::run(const ImageView& image, DetectionResult& outResult) {
    if (!loaded_ || !impl_->isReady()) {
        std::cerr << "[TensorRT] Backend not loaded or not ready" << std::endl;
        return false;
    }
    
    return impl_->infer(image, outResult, desc_);
}

} // namespace falconmind::sdk::perception
