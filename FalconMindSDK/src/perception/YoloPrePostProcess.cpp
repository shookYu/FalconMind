#include "falconmind/sdk/perception/YoloPrePostProcess.h"

#include <algorithm>
#include <cmath>

namespace falconmind::sdk::perception {

namespace {

float sigmoid(float x) {
    return 1.0f / (1.0f + std::exp(-x));
}

float boxIou(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    float xa = std::max(x1, x2);
    float ya = std::max(y1, y2);
    float xb = std::min(x1 + w1, x2 + w2);
    float yb = std::min(y1 + h1, y2 + h2);
    float inter = std::max(0.0f, xb - xa) * std::max(0.0f, yb - ya);
    float a1 = w1 * h1;
    float a2 = w2 * h2;
    return (a1 > 0 && a2 > 0) ? (inter / (a1 + a2 - inter)) : 0.0f;
}

} // namespace

void resizeImageToFloatNchw(
    const std::uint8_t* src, int srcW, int srcH, int srcStride,
    bool bgr,
    float* dst, int dstW, int dstH)
{
    const int C = 3;
    for (int dy = 0; dy < dstH; ++dy) {
        float sy = (srcH > 1) ? (dy * (srcH - 1) / static_cast<float>(dstH - 1)) : 0;
        int iy0 = static_cast<int>(std::floor(sy));
        int iy1 = std::min(iy0 + 1, srcH - 1);
        float fy = sy - iy0;
        for (int dx = 0; dx < dstW; ++dx) {
            float sx = (srcW > 1) ? (dx * (srcW - 1) / static_cast<float>(dstW - 1)) : 0;
            int ix0 = static_cast<int>(std::floor(sx));
            int ix1 = std::min(ix0 + 1, srcW - 1);
            float fx = sx - ix0;
            for (int c = 0; c < C; ++c) {
                int ch = bgr ? (2 - c) : c;
                float v00 = src[iy0 * srcStride + ix0 * C + ch];
                float v10 = src[iy0 * srcStride + ix1 * C + ch];
                float v01 = src[iy1 * srcStride + ix0 * C + ch];
                float v11 = src[iy1 * srcStride + ix1 * C + ch];
                float v = (1 - fx) * (1 - fy) * v00 + fx * (1 - fy) * v10
                       + (1 - fx) * fy * v01 + fx * fy * v11;
                dst[c * (dstH * dstW) + dy * dstW + dx] = v / 255.0f;
            }
        }
    }
}

void decodeYoloOutput84xN(
    const float* outputData,
    std::size_t numChannels, std::size_t numBoxes,
    int numClasses, float scoreThr,
    std::vector<YoloRawDet>& out)
{
    out.clear();
    out.reserve(256);
    if (numChannels < 4u + static_cast<std::size_t>(numClasses) || numBoxes == 0)
        return;
    for (std::size_t j = 0; j < numBoxes; ++j) {
        float cx = outputData[0 * numBoxes + j];
        float cy = outputData[1 * numBoxes + j];
        float w = outputData[2 * numBoxes + j];
        float h = outputData[3 * numBoxes + j];
        int bestClass = 0;
        float bestScore = 0;
        for (int c = 0; c < numClasses; ++c) {
            float s = sigmoid(outputData[(4 + c) * numBoxes + j]);
            if (s > bestScore) {
                bestScore = s;
                bestClass = c;
            }
        }
        if (bestScore < scoreThr) continue;
        YoloRawDet d;
        d.x = cx - w / 2;
        d.y = cy - h / 2;
        d.w = w;
        d.h = h;
        d.score = bestScore;
        d.classId = bestClass;
        out.push_back(d);
    }
}

void nmsYoloDetections(
    const std::vector<YoloRawDet>& raw, float nmsThr,
    std::vector<bool>& suppressed)
{
    suppressed.assign(raw.size(), false);
    std::vector<size_t> order(raw.size());
    for (size_t i = 0; i < raw.size(); ++i) order[i] = i;
    std::sort(order.begin(), order.end(), [&raw](size_t a, size_t b) {
        return raw[a].score > raw[b].score;
    });
    for (size_t ii = 0; ii < order.size(); ++ii) {
        size_t i = order[ii];
        if (suppressed[i]) continue;
        const auto& ri = raw[i];
        for (size_t kk = ii + 1; kk < order.size(); ++kk) {
            size_t k = order[kk];
            if (suppressed[k]) continue;
            if (raw[k].classId != ri.classId) continue;
            float iou = boxIou(ri.x, ri.y, ri.w, ri.h,
                              raw[k].x, raw[k].y, raw[k].w, raw[k].h);
            if (iou > nmsThr) suppressed[k] = true;
        }
    }
}

void fillDetectionResultFromYolo(
    const std::vector<YoloRawDet>& raw,
    const std::vector<bool>& suppressed,
    float scaleX, float scaleY,
    DetectionResult& outResult)
{
    outResult.detections.clear();
    for (size_t i = 0; i < raw.size(); ++i) {
        if (suppressed[i]) continue;
        const auto& r = raw[i];
        Detection d;
        d.bbox.x = r.x * scaleX;
        d.bbox.y = r.y * scaleY;
        d.bbox.width = r.w * scaleX;
        d.bbox.height = r.h * scaleY;
        d.score = r.score;
        d.classId = r.classId;
        outResult.detections.push_back(d);
    }
    outResult.frameIndex = 0;
    outResult.timestampNs = 0;
}

} // namespace falconmind::sdk::perception
