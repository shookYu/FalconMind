#include "falconmind/sdk/perception/DetectorConfigLoader.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <sstream>

namespace falconmind::sdk::perception {

namespace {

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    if (start == s.size()) return {};
    size_t end = s.size() - 1;
    while (end > start && std::isspace(static_cast<unsigned char>(s[end]))) --end;
    return s.substr(start, end - start + 1);
}

std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 && ((s.front() == '"' && s.back() == '"') ||
                          (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

DetectionBackendType parseBackend(const std::string& v) {
    std::string s = v;
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (s == "onnxruntime" || s == "onnx") return DetectionBackendType::OnnxRuntime;
    if (s == "rknn") return DetectionBackendType::Rknn;
    if (s == "tensorrt" || s == "trt") return DetectionBackendType::TensorRt;
    if (s == "cpu" || s == "cpuref") return DetectionBackendType::CpuReference;
    return DetectionBackendType::Unknown;
}

DeviceType parseDevice(const std::string& v) {
    std::string s = v;
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (s == "cpu") return DeviceType::Cpu;
    if (s == "gpu") return DeviceType::Gpu;
    if (s == "npu") return DeviceType::Npu;
    if (s == "auto") return DeviceType::Auto;
    return DeviceType::Auto;
}

ModelPrecision parsePrecision(const std::string& v) {
    std::string s = v;
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (s == "fp16") return ModelPrecision::FP16;
    if (s == "int8") return ModelPrecision::INT8;
    return ModelPrecision::FP32;
}

bool parseInt(const std::string& s, int& out) {
    try {
        out = std::stoi(s);
        return true;
    } catch (...) {
        return false;
    }
}

bool parseFloat(const std::string& s, float& out) {
    try {
        out = std::stof(s);
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

bool loadDetectorsFromYamlFile(const std::string& path, PerceptionPluginManager& manager) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::cerr << "[DetectorConfigLoader] failed to open file: " << path << std::endl;
        return false;
    }

    bool inList = false;
    bool inItem = false;
    DetectorDescriptor current;
    int loadedCount = 0;

    std::string line;
    while (std::getline(ifs, line)) {
        std::string raw = line;
        std::string t = trim(raw);
        if (t.empty() || t[0] == '#') {
            continue;
        }

        if (!inList) {
            if (t == "detectors:" || t.rfind("detectors:", 0) == 0) {
                inList = true;
            }
            continue;
        }

        // 开始一个新的条目
        if (t.rfind("- ", 0) == 0) {
            // 如果前一个条目已填有 id，则提交
            if (inItem && !current.detectorId.empty()) {
                manager.registerDetectorDescriptor(current);
                ++loadedCount;
            }
            current = DetectorDescriptor{};
            inItem = true;

            // 处理可能的 "- id: xxx" 写法
            std::string rest = trim(t.substr(2));
            if (!rest.empty()) {
                auto pos = rest.find(':');
                if (pos != std::string::npos) {
                    std::string key = trim(rest.substr(0, pos));
                    std::string value = stripQuotes(trim(rest.substr(pos + 1)));
                    if (key == "id") current.detectorId = value;
                }
            }
            continue;
        }

        if (!inItem) {
            continue;
        }

        // 解析 "key: value" 行
        auto pos = t.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = trim(t.substr(0, pos));
        std::string value = stripQuotes(trim(t.substr(pos + 1)));

        if (key == "id") {
            current.detectorId = value;
        } else if (key == "name") {
            current.name = value;
        } else if (key == "model_path") {
            current.modelPath = value;
        } else if (key == "label_path") {
            current.labelPath = value;
        } else if (key == "backend") {
            current.backendType = parseBackend(value);
        } else if (key == "device") {
            current.deviceType = parseDevice(value);
        } else if (key == "device_index") {
            int v{};
            if (parseInt(value, v)) current.deviceIndex = v;
        } else if (key == "precision") {
            current.precision = parsePrecision(value);
        } else if (key == "input_width") {
            int v{};
            if (parseInt(value, v)) current.inputWidth = v;
        } else if (key == "input_height") {
            int v{};
            if (parseInt(value, v)) current.inputHeight = v;
        } else if (key == "num_classes") {
            int v{};
            if (parseInt(value, v)) current.numClasses = v;
        } else if (key == "score_threshold") {
            float v{};
            if (parseFloat(value, v)) current.scoreThreshold = v;
        } else if (key == "nms_threshold") {
            float v{};
            if (parseFloat(value, v)) current.nmsThreshold = v;
        }
    }

    // 提交最后一个条目
    if (inItem && !current.detectorId.empty()) {
        manager.registerDetectorDescriptor(current);
        ++loadedCount;
    }

    std::cout << "[DetectorConfigLoader] loaded " << loadedCount
              << " detector descriptors from " << path << std::endl;
    return loadedCount > 0;
}

} // namespace falconmind::sdk::perception

