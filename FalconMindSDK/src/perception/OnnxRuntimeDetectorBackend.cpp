#include "falconmind/sdk/perception/OnnxRuntimeDetectorBackend.h"
#include "falconmind/sdk/perception/YoloPrePostProcess.h"

#include <iostream>
#include <vector>

#if defined(FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED) && FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED
#include <onnxruntime_cxx_api.h>
#endif

namespace falconmind::sdk::perception {

#if defined(FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED) && FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED
namespace {

struct OnnxRuntimeState {
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "FalconMindSDK"};
    Ort::Session* session{nullptr};
    Ort::MemoryInfo memoryInfo{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};
    std::string inputName;
    std::string outputName;
    int inputW{0};
    int inputH{0};
};

} // namespace
#endif

bool OnnxRuntimeDetectorBackend::load(const DetectorDescriptor& desc) {
    desc_ = desc;
#if defined(FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED) && FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED
    if (onnxState_) {
        unload();
    }
    auto* state = new OnnxRuntimeState();
    state->inputW = desc_.inputWidth > 0 ? desc_.inputWidth : 640;
    state->inputH = desc_.inputHeight > 0 ? desc_.inputHeight : 640;

    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    try {
        state->session = new Ort::Session(state->env, desc_.modelPath.c_str(), options);
    } catch (const Ort::Exception& e) {
        std::cerr << "[OnnxRuntimeDetectorBackend] Ort::Session failed: " << e.what() << std::endl;
        delete state;
        return false;
    }

    Ort::AllocatorWithDefaultOptions allocator;
    state->inputName = state->session->GetInputNameAllocated(0, allocator).get();
    state->outputName = state->session->GetOutputNameAllocated(0, allocator).get();

    onnxState_ = state;
    loaded_ = true;
    std::cout << "[OnnxRuntimeDetectorBackend] loaded: " << desc_.modelPath
              << " input=" << state->inputName << " output=" << state->outputName
              << " " << state->inputW << "x" << state->inputH << std::endl;
    return true;
#else
    loaded_ = true;
    std::cout << "[OnnxRuntimeDetectorBackend] load (stub): " << desc_.modelPath
              << " (id=" << desc_.detectorId << ")" << std::endl;
    return true;
#endif
}

void OnnxRuntimeDetectorBackend::unload() {
#if defined(FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED) && FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED
    if (onnxState_) {
        auto* state = static_cast<OnnxRuntimeState*>(onnxState_);
        if (state->session) {
            delete state->session;
            state->session = nullptr;
        }
        delete state;
        onnxState_ = nullptr;
    }
#endif
    if (loaded_) {
        std::cout << "[OnnxRuntimeDetectorBackend] unload: " << desc_.modelPath << std::endl;
    }
    loaded_ = false;
}

bool OnnxRuntimeDetectorBackend::run(const ImageView& image, DetectionResult& outResult) {
    if (!loaded_) {
        std::cerr << "[OnnxRuntimeDetectorBackend] run() called before load()" << std::endl;
        return false;
    }

#if defined(FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED) && FALCONMINDSDK_ONNXRUNTIME_BACKEND_ENABLED
    auto* state = static_cast<OnnxRuntimeState*>(onnxState_);
    if (!state || !state->session) return false;

    const int inputW = state->inputW;
    const int inputH = state->inputH;
    const int numClasses = desc_.numClasses > 0 ? desc_.numClasses : 80;
    const float scoreThr = desc_.scoreThreshold > 0 ? desc_.scoreThreshold : 0.25f;
    const float nmsThr = desc_.nmsThreshold >= 0 ? desc_.nmsThreshold : 0.45f;

    std::vector<float> inputTensor(1 * 3 * inputH * inputW);
    int srcStride = image.stride > 0 ? image.stride : (image.width * 3);
    bool bgr = (image.pixelFormat == "BGR8" || image.pixelFormat == "bgr8");
    resizeImageToFloatNchw(image.data, image.width, image.height, srcStride, bgr,
                           inputTensor.data(), inputW, inputH);

    std::vector<int64_t> inputShape = {1, 3, inputH, inputW};
    Ort::Value inputOrt = Ort::Value::CreateTensor<float>(
        state->memoryInfo, inputTensor.data(), inputTensor.size(),
        inputShape.data(), inputShape.size());

    auto outputTensors = state->session->Run(Ort::RunOptions{nullptr},
        state->inputName.c_str(), &inputOrt, 1,
        state->outputName.c_str(), 1);

    float* outputData = outputTensors[0].GetTensorMutableData<float>();
    auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
    // YOLOv8/v11: (1, 84, 8400) 或 (1, 4+numClasses, numBoxes)
    size_t numChannels = outputShape.size() >= 2 ? static_cast<size_t>(outputShape[1]) : 84;
    size_t numBoxes = outputShape.size() >= 3 ? static_cast<size_t>(outputShape[2]) : 8400;
    std::vector<YoloRawDet> raw;
    decodeYoloOutput84xN(outputData, numChannels, numBoxes, numClasses, scoreThr, raw);
    if (raw.empty()) {
        outResult.detections.clear();
        outResult.frameIndex = 0;
        outResult.timestampNs = 0;
        return true;
    }

    std::vector<bool> suppressed;
    nmsYoloDetections(raw, nmsThr, suppressed);
    float scaleX = (image.width > 0 && inputW > 0) ? (image.width / static_cast<float>(inputW)) : 1.0f;
    float scaleY = (image.height > 0 && inputH > 0) ? (image.height / static_cast<float>(inputH)) : 1.0f;
    fillDetectionResultFromYolo(raw, suppressed, scaleX, scaleY, outResult);
    return true;
#else
    std::cout << "[OnnxRuntimeDetectorBackend] run() (stub): " << image.width << "x" << image.height
              << " format=" << image.pixelFormat << " model=" << desc_.modelPath << std::endl;
    outResult.detections.clear();
    outResult.frameId.clear();
    outResult.frameIndex = 0;
    outResult.timestampNs = 0;
    return true;
#endif
}

} // namespace falconmind::sdk::perception
