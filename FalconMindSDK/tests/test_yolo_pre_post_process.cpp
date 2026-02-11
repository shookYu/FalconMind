// FalconMindSDK - YoloPrePostProcess 单元测试（decode / NMS / fill）
#include "falconmind/sdk/perception/YoloPrePostProcess.h"
#include "falconmind/sdk/perception/DetectionTypes.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>

using namespace falconmind::sdk::perception;

static void test_decode_empty_and_threshold() {
    std::vector<YoloRawDet> out;
    float data[5] = {10, 10, 8, 8, 0};  // 1 box, 1 class; sigmoid(0)=0.5
    decodeYoloOutput84xN(data, 5, 1, 1, 0.6f, out);
    assert(out.empty());  // 0.5 < 0.6
    decodeYoloOutput84xN(data, 5, 1, 1, 0.3f, out);
    assert(out.size() == 1);
    assert(std::abs(out[0].x - 6) < 1e-5f);   // cx - w/2
    assert(std::abs(out[0].y - 6) < 1e-5f);
    assert(std::abs(out[0].w - 8) < 1e-5f);
    assert(std::abs(out[0].h - 8) < 1e-5f);
    assert(out[0].score > 0.49f && out[0].score < 0.51f);
    assert(out[0].classId == 0);
}

static void test_decode_multiple_boxes_and_classes() {
    std::vector<YoloRawDet> out;
    // 2 boxes, 2 classes: layout [cx,cy,w,h, c0, c1] per dim, so 6 channels
    // box0: (10,10,8,8) class0 logit=0 -> 0.5, class1 logit=-100 -> ~0
    // box1: (20,20,4,4) class0 logit=-100, class1 logit=0 -> 0.5
    float data[12] = {
        10, 20,   // cx
        10, 20,   // cy
        8, 4,     // w
        8, 4,     // h
        0, -10,   // class0 logits
        -10, 0    // class1 logits
    };
    decodeYoloOutput84xN(data, 6, 2, 2, 0.3f, out);
    assert(out.size() == 2);
    assert(out[0].classId == 0 && out[0].score > 0.49f);
    assert(out[1].classId == 1 && out[1].score > 0.49f);
    assert(std::abs(out[0].x - 6) < 1e-5f && std::abs(out[0].y - 6) < 1e-5f);
    assert(std::abs(out[1].x - 18) < 1e-5f && std::abs(out[1].y - 18) < 1e-5f);
}

static void test_decode_invalid_inputs() {
    std::vector<YoloRawDet> out;
    float data[5] = {1, 1, 1, 1, 0};
    decodeYoloOutput84xN(data, 3, 1, 1, 0.1f, out);  // numChannels < 4+numClasses
    assert(out.empty());
    decodeYoloOutput84xN(data, 5, 0, 1, 0.1f, out);  // numBoxes == 0
    assert(out.empty());
}

static void test_nms_suppresses_overlap_same_class() {
    std::vector<YoloRawDet> raw = {
        {0, 0, 10, 10, 0.9f, 0},
        {1, 1, 10, 10, 0.8f, 0}
    };
    std::vector<bool> suppressed;
    nmsYoloDetections(raw, 0.5f, suppressed);
    assert(suppressed.size() == 2);
    assert(!suppressed[0]);   // higher score kept
    assert(suppressed[1]);    // lower score, high IoU with first -> suppressed
}

static void test_nms_keeps_different_classes() {
    std::vector<YoloRawDet> raw = {
        {0, 0, 10, 10, 0.9f, 0},
        {0, 0, 10, 10, 0.8f, 1}
    };
    std::vector<bool> suppressed;
    nmsYoloDetections(raw, 0.5f, suppressed);
    assert(suppressed.size() == 2);
    assert(!suppressed[0] && !suppressed[1]);
}

static void test_fill_detection_result() {
    std::vector<YoloRawDet> raw = {{10, 20, 30, 40, 0.95f, 2}};
    std::vector<bool> suppressed = {false};
    DetectionResult result;
    fillDetectionResultFromYolo(raw, suppressed, 2.0f, 0.5f, result);
    assert(result.detections.size() == 1);
    assert(std::abs(result.detections[0].bbox.x - 20) < 1e-5f);
    assert(std::abs(result.detections[0].bbox.y - 10) < 1e-5f);
    assert(std::abs(result.detections[0].bbox.width - 60) < 1e-5f);
    assert(std::abs(result.detections[0].bbox.height - 20) < 1e-5f);
    assert(result.detections[0].score == 0.95f);
    assert(result.detections[0].classId == 2);
}

static void test_fill_skips_suppressed() {
    std::vector<YoloRawDet> raw = {
        {0, 0, 10, 10, 0.9f, 0},
        {5, 5, 10, 10, 0.8f, 0}
    };
    std::vector<bool> suppressed = {false, true};
    DetectionResult result;
    fillDetectionResultFromYolo(raw, suppressed, 1.0f, 1.0f, result);
    assert(result.detections.size() == 1);
    assert(result.detections[0].score == 0.9f);
}

static void test_resize_image_to_float_nchw() {
    // 2x2 RGB image, stride=6
    std::uint8_t src[12] = {
        0, 0, 0, 255, 255, 255,
        255, 255, 255, 0, 0, 0
    };
    float dst[3 * 2 * 2];
    resizeImageToFloatNchw(src, 2, 2, 6, false, dst, 2, 2);
    assert(dst[0 * 4 + 0] < 0.01f);   // R top-left
    assert(dst[0 * 4 + 1] > 0.99f);   // R top-right
    assert(dst[0 * 4 + 2] > 0.99f);
    assert(dst[0 * 4 + 3] < 0.01f);
}

int main() {
    std::cout << "[yolo_pre_post_process_tests] Running..." << std::endl;
    test_decode_empty_and_threshold();
    test_decode_multiple_boxes_and_classes();
    test_decode_invalid_inputs();
    test_nms_suppresses_overlap_same_class();
    test_nms_keeps_different_classes();
    test_fill_detection_result();
    test_fill_skips_suppressed();
    test_resize_image_to_float_nchw();
    std::cout << "[yolo_pre_post_process_tests] All passed." << std::endl;
    return 0;
}
