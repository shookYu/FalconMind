// FalconMindSDK - DetectorConfigLoader
// 简单的 YAML/JSON 风格模型配置加载器（当前实现解析一个约定好的 YAML 子集）。
#pragma once

#include "falconmind/sdk/perception/PerceptionPluginManager.h"

#include <string>

namespace falconmind::sdk::perception {

// 从一个简单的 detectors.yaml 配置文件加载检测模型描述，并批量注册到 PerceptionPluginManager。
//
// 约定的 YAML 结构示例：
//
// detectors:
//   - id: yolo_v26_640_onnx
//     name: YOLOv26 640 ONNX
//     model_path: /opt/models/yolo_v26_640.onnx
//     label_path: /opt/models/coco80.txt
//     backend: onnxruntime        # onnxruntime / rknn / tensorrt / cpu
//     device: cpu                 # cpu / gpu / npu / auto
//     device_index: 0
//     precision: fp32             # fp32 / fp16 / int8
//     input_width: 640
//     input_height: 640
//     num_classes: 80
//     score_threshold: 0.25
//     nms_threshold: 0.45
//
// 仅解析如上 key，忽略未知字段；不依赖外部 YAML/JSON 库。
bool loadDetectorsFromYamlFile(const std::string& path, PerceptionPluginManager& manager);

} // namespace falconmind::sdk::perception

