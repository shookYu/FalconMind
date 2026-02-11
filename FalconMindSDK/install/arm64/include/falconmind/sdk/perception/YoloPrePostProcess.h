// FalconMindSDK - 通用 YOLO 前处理/后处理，供 ONNXRuntime / RKNN 等检测后端复用
#pragma once

#include "falconmind/sdk/perception/DetectionTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace falconmind::sdk::perception {

// 解码后的单条检测（未做 NMS、未缩放回原图）
struct YoloRawDet {
    float x{0.f};
    float y{0.f};
    float w{0.f};
    float h{0.f};
    float score{0.f};
    int   classId{-1};
};

/** 将图像 resize 并转为 NCHW float [0,1]，供模型输入 */
void resizeImageToFloatNchw(
    const std::uint8_t* src, int srcW, int srcH, int srcStride,
    bool bgr,
    float* dst, int dstW, int dstH);

/**
 * 解码 YOLO 输出 (1, numChannels, numBoxes)，layout: outputData[c*numBoxes + j]。
 * 假定前 4 维为 cx,cy,w,h，随后 numClasses 维为类别 logits（内部做 sigmoid）。
 */
void decodeYoloOutput84xN(
    const float* outputData,
    std::size_t numChannels, std::size_t numBoxes,
    int numClasses, float scoreThr,
    std::vector<YoloRawDet>& out);

/** 按类别做 NMS，suppressed[i]==true 表示被抑制 */
void nmsYoloDetections(
    const std::vector<YoloRawDet>& raw, float nmsThr,
    std::vector<bool>& suppressed);

/** 将 raw + suppressed 缩放回原图坐标并写入 DetectionResult */
void fillDetectionResultFromYolo(
    const std::vector<YoloRawDet>& raw,
    const std::vector<bool>& suppressed,
    float scaleX, float scaleY,
    DetectionResult& outResult);

} // namespace falconmind::sdk::perception
