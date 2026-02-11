#include "falconmind/sdk/perception/DetectionResultPacket.h"

#include <cstring>
#include <algorithm>

namespace falconmind::sdk::perception {

std::size_t serializeDetectionResult(
    const DetectionResult& result,
    std::uint8_t* buffer,
    std::size_t bufferSize)
{
    const std::size_t headerSize = sizeof(DetectionResultPacketHeader);
    const std::size_t itemSize = sizeof(DetectionResultPacketItem);
    const std::size_t n = result.detections.size();
    const std::size_t total = headerSize + n * itemSize;
    if (!buffer || bufferSize < total) return 0;

    DetectionResultPacketHeader* h = reinterpret_cast<DetectionResultPacketHeader*>(buffer);
    h->magic = DETECTION_RESULT_PACKET_MAGIC;
    h->version = 1;
    h->frameIndex = result.frameIndex;
    h->timestampNs = result.timestampNs;
    h->numDetections = static_cast<std::uint32_t>(n);

    DetectionResultPacketItem* items = reinterpret_cast<DetectionResultPacketItem*>(buffer + headerSize);
    for (size_t i = 0; i < n; ++i) {
        const auto& d = result.detections[i];
        items[i].x = d.bbox.x;
        items[i].y = d.bbox.y;
        items[i].width = d.bbox.width;
        items[i].height = d.bbox.height;
        items[i].score = d.score;
        items[i].classId = static_cast<std::int32_t>(d.classId);
    }
    return total;
}

std::uint32_t parseDetectionResultPacketNumDetections(const void* buffer, std::size_t size) {
    if (!buffer || size < sizeof(DetectionResultPacketHeader)) return 0;
    const auto* h = static_cast<const DetectionResultPacketHeader*>(buffer);
    if (h->magic != DETECTION_RESULT_PACKET_MAGIC || h->version != 1) return 0;
    return h->numDetections;
}

} // namespace falconmind::sdk::perception
