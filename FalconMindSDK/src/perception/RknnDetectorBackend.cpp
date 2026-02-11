#include "falconmind/sdk/perception/RknnDetectorBackend.h"
#include "falconmind/sdk/perception/YoloPrePostProcess.h"

#include <iostream>
#include <vector>
#include <cstring>

#if defined(FALCONMINDSDK_RKNN_BACKEND_ENABLED) && FALCONMINDSDK_RKNN_BACKEND_ENABLED
#include <rknn_api.h>
#endif

namespace falconmind::sdk::perception {

#if defined(FALCONMINDSDK_RKNN_BACKEND_ENABLED) && FALCONMINDSDK_RKNN_BACKEND_ENABLED
namespace {

struct RknnState {
    rknn_context ctx{0};
    int inputW{0};
    int inputH{0};
    uint32_t inputSize{0};
    rknn_tensor_format inputFmt{RKNN_TENSOR_NCHW};
    rknn_tensor_type inputType{RKNN_TENSOR_FLOAT32};
};

} // namespace
#endif

bool RknnDetectorBackend::load(const DetectorDescriptor& desc) {
    desc_ = desc;
#if defined(FALCONMINDSDK_RKNN_BACKEND_ENABLED) && FALCONMINDSDK_RKNN_BACKEND_ENABLED
    if (rknnState_) {
        unload();
    }
    auto* state = new RknnState();
    state->inputW = desc_.inputWidth > 0 ? desc_.inputWidth : 640;
    state->inputH = desc_.inputHeight > 0 ? desc_.inputHeight : 640;
    state->inputSize = 1 * 3 * static_cast<uint32_t>(state->inputH) * static_cast<uint32_t>(state->inputW) * sizeof(float);

    // rknn_init: size=0 表示 model 为文件路径
    int ret = rknn_init(&state->ctx, (void*)desc_.modelPath.c_str(), 0, 0, nullptr);
    if (ret != RKNN_SUCC) {
        std::cerr << "[RknnDetectorBackend] rknn_init failed: " << ret << " path=" << desc_.modelPath << std::endl;
        delete state;
        return false;
    }

    rknn_input_output_num num;
    ret = rknn_query(state->ctx, RKNN_QUERY_IN_OUT_NUM, &num, sizeof(num));
    if (ret != RKNN_SUCC || num.n_input < 1) {
        std::cerr << "[RknnDetectorBackend] rknn_query IN_OUT_NUM failed or no input" << std::endl;
        rknn_destroy(state->ctx);
        delete state;
        return false;
    }

    rknn_tensor_attr attr;
    memset(&attr, 0, sizeof(attr));
    attr.index = 0;
    ret = rknn_query(state->ctx, RKNN_QUERY_INPUT_ATTR, &attr, sizeof(attr));
    if (ret == RKNN_SUCC && attr.n_dims >= 2) {
        // 常见 NCHW: dims = [1,3,H,W] 或 NHWC: [1,H,W,3]
        if (attr.fmt == RKNN_TENSOR_NCHW && attr.n_dims >= 4) {
            state->inputH = static_cast<int>(attr.dims[2]);
            state->inputW = static_cast<int>(attr.dims[3]);
        } else if (attr.fmt == RKNN_TENSOR_NHWC && attr.n_dims >= 4) {
            state->inputH = static_cast<int>(attr.dims[1]);
            state->inputW = static_cast<int>(attr.dims[2]);
        }
        state->inputFmt = attr.fmt;
        state->inputType = attr.type;
        state->inputSize = attr.size;
    }
    if (state->inputSize == 0)
        state->inputSize = 1 * 3 * static_cast<uint32_t>(state->inputH) * static_cast<uint32_t>(state->inputW) * sizeof(float);

    rknnState_ = state;
    loaded_ = true;
    std::cout << "[RknnDetectorBackend] loaded: " << desc_.modelPath
              << " " << state->inputW << "x" << state->inputH
              << " fmt=" << (state->inputFmt == RKNN_TENSOR_NCHW ? "NCHW" : "NHWC") << std::endl;
    return true;
#else
    loaded_ = true;
    std::cout << "[RknnDetectorBackend] load (stub): " << desc_.modelPath
              << " (id=" << desc_.detectorId << ")" << std::endl;
    return true;
#endif
}

void RknnDetectorBackend::unload() {
#if defined(FALCONMINDSDK_RKNN_BACKEND_ENABLED) && FALCONMINDSDK_RKNN_BACKEND_ENABLED
    if (rknnState_) {
        auto* state = static_cast<RknnState*>(rknnState_);
        if (state->ctx) {
            rknn_destroy(state->ctx);
            state->ctx = 0;
        }
        delete state;
        rknnState_ = nullptr;
    }
#endif
    if (loaded_) {
        std::cout << "[RknnDetectorBackend] unload: " << desc_.modelPath << std::endl;
    }
    loaded_ = false;
}

bool RknnDetectorBackend::run(const ImageView& image, DetectionResult& outResult) {
    if (!loaded_) {
        std::cerr << "[RknnDetectorBackend] run() called before load()" << std::endl;
        return false;
    }

#if defined(FALCONMINDSDK_RKNN_BACKEND_ENABLED) && FALCONMINDSDK_RKNN_BACKEND_ENABLED
    auto* state = static_cast<RknnState*>(rknnState_);
    if (!state || !state->ctx) return false;

    const int inputW = state->inputW;
    const int inputH = state->inputH;
    const int numClasses = desc_.numClasses > 0 ? desc_.numClasses : 80;
    const float scoreThr = desc_.scoreThreshold > 0 ? desc_.scoreThreshold : 0.25f;
    const float nmsThr = desc_.nmsThreshold >= 0 ? desc_.nmsThreshold : 0.45f;

    std::vector<float> inputBuf(1 * 3 * inputH * inputW);
    int srcStride = image.stride > 0 ? image.stride : (image.width * 3);
    bool bgr = (image.pixelFormat == "BGR8" || image.pixelFormat == "bgr8");
    resizeImageToFloatNchw(image.data, image.width, image.height, srcStride, bgr,
                           inputBuf.data(), inputW, inputH);

    rknn_input inputs[1];
    memset(inputs, 0, sizeof(inputs));
    inputs[0].index = 0;
    inputs[0].buf = inputBuf.data();
    inputs[0].size = static_cast<uint32_t>(inputBuf.size() * sizeof(float));
    inputs[0].pass_through = 0;
    inputs[0].type = RKNN_TENSOR_FLOAT32;
    inputs[0].fmt = RKNN_TENSOR_NCHW;

    int ret = rknn_inputs_set(state->ctx, 1, inputs);
    if (ret != RKNN_SUCC) {
        std::cerr << "[RknnDetectorBackend] rknn_inputs_set failed: " << ret << std::endl;
        return false;
    }
    ret = rknn_run(state->ctx, nullptr);
    if (ret != RKNN_SUCC) {
        std::cerr << "[RknnDetectorBackend] rknn_run failed: " << ret << std::endl;
        return false;
    }

    rknn_input_output_num num;
    ret = rknn_query(state->ctx, RKNN_QUERY_IN_OUT_NUM, &num, sizeof(num));
    if (ret != RKNN_SUCC || num.n_output < 1) {
        outResult.detections.clear();
        return true;
    }

    std::vector<rknn_output> outputs(num.n_output);
    for (uint32_t i = 0; i < num.n_output; ++i) {
        outputs[i].want_float = 1;
        outputs[i].is_prealloc = 0;
        outputs[i].index = i;
        outputs[i].buf = nullptr;
        outputs[i].size = 0;
    }
    ret = rknn_outputs_get(state->ctx, num.n_output, outputs.data(), nullptr);
    if (ret != RKNN_SUCC) {
        std::cerr << "[RknnDetectorBackend] rknn_outputs_get failed: " << ret << std::endl;
        return false;
    }

    float* outputData = static_cast<float*>(outputs[0].buf);
    uint32_t outputSize = outputs[0].size;
    if (!outputData || outputSize < 4 * sizeof(float)) {
        rknn_outputs_release(state->ctx, num.n_output, outputs.data());
        outResult.detections.clear();
        return true;
    }

    rknn_tensor_attr outAttr;
    memset(&outAttr, 0, sizeof(outAttr));
    outAttr.index = 0;
    rknn_query(state->ctx, RKNN_QUERY_OUTPUT_ATTR, &outAttr, sizeof(outAttr));
    size_t numChannels = (outAttr.n_dims >= 2) ? static_cast<size_t>(outAttr.dims[1]) : 84;
    size_t numBoxes = (outAttr.n_dims >= 3) ? static_cast<size_t>(outAttr.dims[2]) : 8400;

    std::vector<YoloRawDet> raw;
    decodeYoloOutput84xN(outputData, numChannels, numBoxes, numClasses, scoreThr, raw);
    rknn_outputs_release(state->ctx, num.n_output, outputs.data());

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
    std::cout << "[RknnDetectorBackend] run() (stub): " << image.width << "x" << image.height
              << " format=" << image.pixelFormat << " model=" << desc_.modelPath << std::endl;
    outResult.detections.clear();
    outResult.frameId.clear();
    outResult.frameIndex = 0;
    outResult.timestampNs = 0;
    return true;
#endif
}

} // namespace falconmind::sdk::perception
