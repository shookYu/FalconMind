// 单元测试：DetectionResultPacket 序列化与解析
#include "falconmind/sdk/perception/DetectionResultPacket.h"
#include "falconmind/sdk/perception/DetectionTypes.h"

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>

using namespace falconmind::sdk::perception;

static void test_empty_result() {
    DetectionResult result;
    result.frameIndex = 1;
    result.timestampNs = 1000;
    std::vector<std::uint8_t> buf(detectionResultPacketSize(0));
    std::size_t written = serializeDetectionResult(result, buf.data(), buf.size());
    assert(written == sizeof(DetectionResultPacketHeader));
    assert(written == detectionResultPacketSize(0));
    std::uint32_t n = parseDetectionResultPacketNumDetections(buf.data(), written);
    assert(n == 0);
    std::cout << "  test_empty_result passed\n";
}

static void test_single_detection() {
    DetectionResult result;
    result.frameIndex = 2;
    result.timestampNs = 2000;
    Detection d;
    d.bbox.x = 10.f;
    d.bbox.y = 20.f;
    d.bbox.width = 100.f;
    d.bbox.height = 80.f;
    d.score = 0.9f;
    d.classId = 1;
    result.detections.push_back(d);
    std::size_t cap = detectionResultPacketSize(1);
    std::vector<std::uint8_t> buf(cap);
    std::size_t written = serializeDetectionResult(result, buf.data(), buf.size());
    assert(written == cap);
    std::uint32_t n = parseDetectionResultPacketNumDetections(buf.data(), written);
    assert(n == 1);
    const auto* h = reinterpret_cast<const DetectionResultPacketHeader*>(buf.data());
    assert(h->magic == DETECTION_RESULT_PACKET_MAGIC);
    assert(h->version == 1);
    assert(h->frameIndex == 2);
    assert(h->timestampNs == 2000);
    assert(h->numDetections == 1);
    std::cout << "  test_single_detection passed\n";
}

static void test_multiple_detections() {
    DetectionResult result;
    result.frameIndex = 3;
    result.timestampNs = 3000;
    for (int i = 0; i < 5; ++i) {
        Detection d;
        d.bbox.x = static_cast<float>(i * 10);
        d.bbox.y = static_cast<float>(i * 20);
        d.bbox.width = 50.f;
        d.bbox.height = 50.f;
        d.score = 0.5f + i * 0.1f;
        d.classId = i;
        result.detections.push_back(d);
    }
    std::size_t cap = detectionResultPacketSize(5);
    std::vector<std::uint8_t> buf(cap);
    std::size_t written = serializeDetectionResult(result, buf.data(), buf.size());
    assert(written == cap);
    std::uint32_t n = parseDetectionResultPacketNumDetections(buf.data(), written);
    assert(n == 5);
    std::cout << "  test_multiple_detections passed\n";
}

static void test_parse_invalid() {
    std::uint8_t bad[4] = {0, 0, 0, 0};
    assert(parseDetectionResultPacketNumDetections(bad, 4) == 0);
    assert(parseDetectionResultPacketNumDetections(nullptr, 100) == 0);
    assert(parseDetectionResultPacketNumDetections(bad, 0) == 0);
    std::cout << "  test_parse_invalid passed\n";
}

int main() {
    std::cout << "Running DetectionResultPacket tests...\n";
    test_empty_result();
    test_single_detection();
    test_multiple_detections();
    test_parse_invalid();
    std::cout << "All DetectionResultPacket tests passed.\n";
    return 0;
}
