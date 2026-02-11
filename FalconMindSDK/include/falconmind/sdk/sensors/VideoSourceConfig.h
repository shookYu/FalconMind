// FalconMindSDK - VideoSourceConfig (week3 skeleton)
#pragma once

#include <string>

namespace falconmind::sdk::sensors {

enum class VideoSourceType {
    Unknown,
    UsbCamera,
    MipiCamera,
    RtspStream,
    UdpStream
};

struct VideoSourceConfig {
    std::string   sensorId;
    VideoSourceType sourceType{VideoSourceType::UsbCamera};
    std::string   device;       // /dev/video0, mipi0 等
    std::string   uri;          // rtsp/udp 地址
    unsigned int  width{0};     // 0 表示使用设备默认
    unsigned int  height{0};
    double        fps{0.0};
    std::string   pixelFormat;  // NV12/RGB8/...
    std::string   decoder;      // RK_HW/NVDEC/SW_FFMPEG/...
};

} // namespace falconmind::sdk::sensors

