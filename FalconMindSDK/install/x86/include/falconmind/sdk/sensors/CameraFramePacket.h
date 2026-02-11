// FalconMindSDK - 相机帧在 Pad 上传递的包格式（头 + 紧接的像素数据）
#pragma once

#include <cstdint>
#include <cstddef>

namespace falconmind::sdk::sensors {

/** 帧包头，紧接其后为 width*height*bytesPerPixel 的像素数据 */
struct CameraFramePacket {
    int32_t  width{0};
    int32_t  height{0};
    int32_t  stride{0};   // 每行字节数，≥ width*bytesPerPixel
    char     format[16];  // "RGB8"/"BGR8"/"YUYV" 等，以 \0 结尾
};

/** 整个包大小 = sizeof(CameraFramePacket) + 像素字节数 */
inline size_t cameraFramePacketTotalSize(const CameraFramePacket& h, int bytesPerPixel) {
    if (h.stride > 0 && h.height > 0)
        return sizeof(CameraFramePacket) + static_cast<size_t>(h.stride) * static_cast<size_t>(h.height);
    if (h.width > 0 && h.height > 0)
        return sizeof(CameraFramePacket) + static_cast<size_t>(h.width) * static_cast<size_t>(h.height) * bytesPerPixel;
    return sizeof(CameraFramePacket);
}

/** 包头之后即像素数据 */
inline const std::uint8_t* cameraFramePacketData(const CameraFramePacket* header) {
    return reinterpret_cast<const std::uint8_t*>(header + 1);
}

/** 包头之后可写像素数据（用于原地增强） */
inline std::uint8_t* cameraFramePacketDataWritable(CameraFramePacket* header) {
    return reinterpret_cast<std::uint8_t*>(header + 1);
}

} // namespace falconmind::sdk::sensors
