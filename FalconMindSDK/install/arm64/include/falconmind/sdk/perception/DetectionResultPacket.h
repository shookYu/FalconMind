// FalconMindSDK - 检测结果在 Pad 上传递的二进制包格式（供 detection_out → 下游）
#pragma once

#include "falconmind/sdk/perception/DetectionTypes.h"

#include <cstdint>
#include <cstddef>
#include <vector>

namespace falconmind::sdk::perception {

/** 包魔数 "DRES" */
constexpr std::uint32_t DETECTION_RESULT_PACKET_MAGIC = 0x53455244u; // "DRES" LE

/** 包头：魔数 + 版本 + 预留 + frameIndex + timestampNs + numDetections */
struct DetectionResultPacketHeader {
    std::uint32_t magic{DETECTION_RESULT_PACKET_MAGIC};
    std::uint8_t  version{1};
    std::uint8_t  reserved[3]{0, 0, 0};
    std::uint32_t frameIndex{0};
    std::uint64_t timestampNs{0};
    std::uint32_t numDetections{0};
};

/** 单条检测的定长部分（不含 className） */
struct DetectionResultPacketItem {
    float     x{0.f};
    float     y{0.f};
    float     width{0.f};
    float     height{0.f};
    float     score{0.f};
    std::int32_t classId{-1};
};

/** 将 DetectionResult 序列化为连续内存，返回字节数；buffer 需已预留足够空间 */
std::size_t serializeDetectionResult(
    const DetectionResult& result,
    std::uint8_t* buffer,
    std::size_t bufferSize);

/** 计算序列化所需最大字节数（不含 className 字符串，仅定长字段） */
inline std::size_t detectionResultPacketSize(std::size_t numDetections) {
    return sizeof(DetectionResultPacketHeader) + numDetections * sizeof(DetectionResultPacketItem);
}

/** 从 buffer 解析出 numDetections；若格式无效返回 0 */
std::uint32_t parseDetectionResultPacketNumDetections(const void* buffer, std::size_t size);

} // namespace falconmind::sdk::perception
