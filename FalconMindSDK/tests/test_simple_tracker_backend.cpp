// FalconMindSDK - SimpleTrackerBackend 与 SortTrackerBackend 单元测试
#include "falconmind/sdk/perception/SimpleTrackerBackend.h"
#include "falconmind/sdk/perception/SortTrackerBackend.h"
#include "falconmind/sdk/perception/DetectionTypes.h"
#include "falconmind/sdk/perception/TrackingTypes.h"

#include <cassert>
#include <iostream>

using namespace falconmind::sdk::perception;

static DetectionResult makeDetections(std::uint64_t ts, std::uint32_t frameIdx,
                                       const std::vector<std::array<float, 4>>& boxes) {
    DetectionResult r;
    r.timestampNs = ts;
    r.frameIndex = frameIdx;
    for (const auto& b : boxes) {
        Detection d;
        d.bbox.x = b[0];
        d.bbox.y = b[1];
        d.bbox.width = b[2];
        d.bbox.height = b[3];
        d.score = 0.9f;
        d.classId = 0;
        r.detections.push_back(d);
    }
    return r;
}

static void test_load_unload() {
    SimpleTrackerBackend backend;
    assert(!backend.isLoaded());
    assert(backend.load());
    assert(backend.isLoaded());
    backend.unload();
    assert(!backend.isLoaded());
}

static void test_run_without_load_returns_false() {
    SimpleTrackerBackend backend;
    DetectionResult det = makeDetections(0, 0, {{0, 0, 10, 10}});
    TrackingResult out;
    assert(!backend.run(det, out));
}

static void test_single_detection_gets_track_id() {
    SimpleTrackerBackend backend;
    assert(backend.load());
    DetectionResult det = makeDetections(1000, 0, {{10, 20, 30, 40}});
    TrackingResult out;
    assert(backend.run(det, out));
    assert(out.tracks.size() == 1);
    assert(out.tracks[0].trackId == 1);
    assert(out.tracks[0].status == "ACTIVE");
    assert(det.detections[0].trackId == 1);
}

static void test_same_bbox_matches_same_track() {
    SimpleTrackerBackend backend;
    backend.setIouThreshold(0.3f);
    assert(backend.load());
    DetectionResult det1 = makeDetections(1000, 0, {{0, 0, 10, 10}});
    TrackingResult out1;
    assert(backend.run(det1, out1));
    assert(out1.tracks.size() == 1 && out1.tracks[0].trackId == 1);
    // 第二帧：框略移动，IoU 仍高，应匹配同一 track
    DetectionResult det2 = makeDetections(2000, 1, {{1, 1, 10, 10}});
    TrackingResult out2;
    assert(backend.run(det2, out2));
    assert(det2.detections[0].trackId == 1);
    assert(out2.tracks.size() == 1);
    assert(out2.tracks[0].trackId == 1);
    assert(out2.tracks[0].status == "ACTIVE");
}

static void test_lost_after_max_missed_frames() {
    SimpleTrackerBackend backend;
    backend.setMaxMissedFrames(3);
    assert(backend.load());
    DetectionResult det = makeDetections(1000, 0, {{0, 0, 10, 10}});
    TrackingResult out;
    assert(backend.run(det, out));
    assert(out.tracks.size() == 1 && out.tracks[0].status == "ACTIVE");
    // 连续 4 帧无检测
    for (uint32_t i = 1; i <= 4; ++i) {
        DetectionResult empty = makeDetections(1000 + i * 100, i, {});
        out.tracks.clear();
        assert(backend.run(empty, out));
    }
    // 第 4 帧后该 track 应被标记 LOST 并移除，out 中应出现一次 LOST（在移除前写入）
    // 实际逻辑：每帧都先 missedFrames++，再处理；若某 track missedFrames > 3 则标记 LOST 并 toRemove
    // 帧1: 1 det -> 1 track ACTIVE, missedFrames=0
    // 帧2: 0 det -> 该 track missedFrames=1 -> ACTIVE
    // 帧3: 0 det -> missedFrames=2 -> ACTIVE
    // 帧4: 0 det -> missedFrames=3 -> ACTIVE (3 is not > 3)
    // 帧5: 0 det -> missedFrames=4 -> LOST, 移除
    // 所以第5次 run(empty) 时 out 里有 1 个 track 且 status=="LOST"
    assert(out.tracks.size() == 1);
    assert(out.tracks[0].status == "LOST");
}

static void test_two_distant_boxes_two_tracks() {
    SimpleTrackerBackend backend;
    assert(backend.load());
    DetectionResult det = makeDetections(1000, 0, {
        {0, 0, 10, 10},
        {100, 100, 10, 10}
    });
    TrackingResult out;
    assert(backend.run(det, out));
    assert(out.tracks.size() == 2);
    assert(det.detections[0].trackId != det.detections[1].trackId);
    assert(det.detections[0].trackId == 1);
    assert(det.detections[1].trackId == 2);
}

static void test_output_frame_metadata() {
    SimpleTrackerBackend backend;
    assert(backend.load());
    DetectionResult det = makeDetections(12345, 7, {{0, 0, 5, 5}});
    det.frameId = "f1";
    TrackingResult out;
    assert(backend.run(det, out));
    assert(out.frameIndex == 7);
    assert(out.timestampNs == 12345);
    assert(out.frameId == "f1");
}

static void test_sort_tracker_backend_delegate() {
    SortTrackerBackend backend;
    assert(!backend.isLoaded());
    assert(backend.load());
    assert(backend.isLoaded());
    DetectionResult det = makeDetections(1000, 0, {{5, 5, 20, 20}});
    TrackingResult out;
    assert(backend.run(det, out));
    assert(out.tracks.size() == 1);
    assert(out.tracks[0].status == "ACTIVE");
    assert(det.detections[0].trackId > 0);
    backend.unload();
    assert(!backend.isLoaded());
}

int main() {
    std::cout << "[simple_tracker_backend_tests] Running..." << std::endl;
    test_load_unload();
    test_run_without_load_returns_false();
    test_single_detection_gets_track_id();
    test_same_bbox_matches_same_track();
    test_lost_after_max_missed_frames();
    test_two_distant_boxes_two_tracks();
    test_output_frame_metadata();
    test_sort_tracker_backend_delegate();
    std::cout << "[simple_tracker_backend_tests] All passed." << std::endl;
    return 0;
}
