#include "falconmind/sdk/sensors/CameraSourceNode.h"
#include "falconmind/sdk/sensors/CameraFramePacket.h"

#include <iostream>
#include <cstring>
#include <algorithm>
#include <fstream>

#ifdef __linux__
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

namespace falconmind::sdk::sensors {

using namespace falconmind::sdk::core;

CameraSourceNode::CameraSourceNode(const VideoSourceConfig& cfg)
    : Node("camera_source"), config_(cfg) {
    addPad(std::make_shared<Pad>("video_out", PadType::Source));
}

bool CameraSourceNode::configure(const std::unordered_map<std::string, std::string>& params) {
    auto itDev = params.find("device");
    if (itDev != params.end()) config_.device = itDev->second;
    auto itUri = params.find("uri");
    if (itUri != params.end()) config_.uri = itUri->second;
    auto itW = params.find("width");
    if (itW != params.end()) config_.width = static_cast<unsigned>(std::stoul(itW->second));
    auto itH = params.find("height");
    if (itH != params.end()) config_.height = static_cast<unsigned>(std::stoul(itH->second));
    return true;
}

#ifdef __linux__
static bool trySetFormat(int fd, int width, int height, uint32_t* outPixelFormat, int* outStride) {
    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == 0) {
        *outPixelFormat = fmt.fmt.pix.pixelformat;
        *outStride = static_cast<int>(fmt.fmt.pix.bytesperline);
        return true;
    }
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) == 0) {
        *outPixelFormat = fmt.fmt.pix.pixelformat;
        *outStride = static_cast<int>(fmt.fmt.pix.bytesperline);
        return true;
    }
    return false;
}

bool CameraSourceNode::initV4L2() {
    if (config_.device.empty()) return false;
    v4l2Fd_ = open(config_.device.c_str(), O_RDWR);
    if (v4l2Fd_ < 0) {
        std::cerr << "[CameraSourceNode] open " << config_.device << " failed: " << errno << std::endl;
        return false;
    }
    v4l2_capability cap{};
    if (ioctl(v4l2Fd_, VIDIOC_QUERYCAP, &cap) != 0 ||
        !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
        !(cap.capabilities & V4L2_CAP_STREAMING)) {
        std::cerr << "[CameraSourceNode] device not a capture/streaming device" << std::endl;
        close(v4l2Fd_);
        v4l2Fd_ = -1;
        return false;
    }
    int w = config_.width > 0 ? static_cast<int>(config_.width) : 640;
    int h = config_.height > 0 ? static_cast<int>(config_.height) : 480;
    uint32_t pixFmt = 0;
    if (!trySetFormat(v4l2Fd_, w, h, &pixFmt, &v4l2Stride_)) {
        std::cerr << "[CameraSourceNode] SET_FMT failed" << std::endl;
        close(v4l2Fd_);
        v4l2Fd_ = -1;
        return false;
    }
    v4l2Width_ = w;
    v4l2Height_ = h;
    if (v4l2Stride_ <= 0) v4l2Stride_ = (pixFmt == V4L2_PIX_FMT_RGB24) ? (w * 3) : (w * 2);

    v4l2_requestbuffers req{};
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(v4l2Fd_, VIDIOC_REQBUFS, &req) != 0 || req.count == 0) {
        std::cerr << "[CameraSourceNode] REQBUFS failed" << std::endl;
        close(v4l2Fd_);
        v4l2Fd_ = -1;
        return false;
    }
    v4l2NumBuffers_ = req.count;
    v4l2MapPtrs_.resize(v4l2NumBuffers_, nullptr);
    v4l2MapLens_.resize(v4l2NumBuffers_, 0);
    for (unsigned i = 0; i < v4l2NumBuffers_; ++i) {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(v4l2Fd_, VIDIOC_QUERYBUF, &buf) != 0) {
            shutdownV4L2();
            return false;
        }
        void* ptr = mmap(nullptr, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, v4l2Fd_, buf.m.offset);
        if (ptr == MAP_FAILED) {
            shutdownV4L2();
            return false;
        }
        v4l2MapPtrs_[i] = ptr;
        v4l2MapLens_[i] = buf.length;
    }
    for (unsigned i = 0; i < v4l2NumBuffers_; ++i) {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(v4l2Fd_, VIDIOC_QBUF, &buf) != 0) {
            shutdownV4L2();
            return false;
        }
    }
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(v4l2Fd_, VIDIOC_STREAMON, &type) != 0) {
        shutdownV4L2();
        return false;
    }

    size_t pixelSize = static_cast<size_t>(v4l2Stride_) * static_cast<size_t>(v4l2Height_);
    size_t rgbSize = static_cast<size_t>(v4l2Width_) * static_cast<size_t>(v4l2Height_) * 3u;
    frameBuffer_.resize(sizeof(CameraFramePacket) + std::max(pixelSize, rgbSize));
    CameraFramePacket* header = reinterpret_cast<CameraFramePacket*>(frameBuffer_.data());
    header->width = v4l2Width_;
    header->height = v4l2Height_;
    header->stride = v4l2Stride_;
    std::strncpy(header->format, (pixFmt == V4L2_PIX_FMT_RGB24) ? "RGB8" : "YUYV", sizeof(header->format) - 1);
    header->format[sizeof(header->format) - 1] = '\0';
    return true;
}

void CameraSourceNode::shutdownV4L2() {
    if (v4l2Fd_ >= 0) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(v4l2Fd_, VIDIOC_STREAMOFF, &type);
        for (unsigned i = 0; i < v4l2MapPtrs_.size(); ++i) {
            if (v4l2MapPtrs_[i])
                munmap(v4l2MapPtrs_[i], v4l2MapLens_[i]);
        }
        v4l2MapPtrs_.clear();
        v4l2MapLens_.clear();
        close(v4l2Fd_);
        v4l2Fd_ = -1;
    }
    v4l2NumBuffers_ = 0;
    v4l2Ready_ = false;
}
#else
bool CameraSourceNode::initV4L2() { (void)config_; return false; }
void CameraSourceNode::shutdownV4L2() {}
#endif

bool CameraSourceNode::initFileMode() {
    if (filePath_.empty()) return false;
    fileStream_.open(filePath_, std::ios::binary);
    if (!fileStream_.is_open()) {
        std::cerr << "[CameraSourceNode] file open failed: " << filePath_ << std::endl;
        return false;
    }
    fileWidth_ = config_.width > 0 ? config_.width : 640;
    fileHeight_ = config_.height > 0 ? config_.height : 480;
    fileFrameBytes_ = static_cast<size_t>(fileWidth_) * static_cast<size_t>(fileHeight_) * 3u;
    frameBuffer_.resize(sizeof(CameraFramePacket) + fileFrameBytes_);
    CameraFramePacket* header = reinterpret_cast<CameraFramePacket*>(frameBuffer_.data());
    header->width = static_cast<int32_t>(fileWidth_);
    header->height = static_cast<int32_t>(fileHeight_);
    header->stride = static_cast<int32_t>(fileWidth_ * 3);
    std::strncpy(header->format, "RGB8", sizeof(header->format) - 1);
    header->format[sizeof(header->format) - 1] = '\0';
    std::cout << "[CameraSourceNode] file mode: " << filePath_ << " " << fileWidth_ << "x" << fileHeight_ << std::endl;
    return true;
}

void CameraSourceNode::shutdownFileMode() {
    if (fileStream_.is_open()) fileStream_.close();
    fileMode_ = false;
}

bool CameraSourceNode::start() {
    started_ = true;
#ifdef __linux__
    if (!config_.device.empty()) {
        v4l2Ready_ = initV4L2();
        if (v4l2Ready_ && !frameBuffer_.empty()) {
            auto* hp = reinterpret_cast<CameraFramePacket*>(frameBuffer_.data());
            std::cout << "[CameraSourceNode] V4L2 started: " << config_.device
                      << " " << v4l2Width_ << "x" << v4l2Height_ << " " << hp->format << std::endl;
        }
    }
#endif
    if (!v4l2Ready_) {
        if (config_.uri.size() >= 5 && config_.uri.substr(0, 5) == "file:") {
            filePath_ = config_.uri.substr(5);
            fileMode_ = initFileMode();
        }
        if (!fileMode_)
            std::cout << "[CameraSourceNode] start (stub): device=" << config_.device
                      << " uri=" << config_.uri << " width=" << config_.width
                      << " height=" << config_.height << " fps=" << config_.fps << std::endl;
    }
    return true;
}

void CameraSourceNode::stop() {
#ifdef __linux__
    shutdownV4L2();
#endif
    shutdownFileMode();
    started_ = false;
}

void CameraSourceNode::process() {
    if (!started_) return;

#ifdef __linux__
    if (v4l2Ready_ && v4l2Fd_ >= 0) {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        if (ioctl(v4l2Fd_, VIDIOC_DQBUF, &buf) != 0)
            return;
        CameraFramePacket* header = reinterpret_cast<CameraFramePacket*>(frameBuffer_.data());
        std::uint8_t* dst = frameBuffer_.data() + sizeof(CameraFramePacket);
        const std::uint8_t* src = static_cast<const std::uint8_t*>(v4l2MapPtrs_[buf.index]);
        size_t srcStride = static_cast<size_t>(v4l2Stride_);
        int w = v4l2Width_, hh = v4l2Height_;
        bool isYuyv = (std::strcmp(header->format, "YUYV") == 0);
        if (isYuyv && srcStride >= static_cast<size_t>(w * 2)) {
            for (int y = 0; y < hh; ++y) {
                const std::uint8_t* row = src + y * srcStride;
                for (int x = 0; x < w; ++x) {
                    int u = row[x << 1];
                    int y0 = row[x << 1 | 1];
                    int v = (x + 1) < w ? row[(x + 1) << 1] : u;
                    int r = y0 + (351 * (v - 128)) / 256;
                    int g = y0 - (179 * (v - 128) + 86 * (u - 128)) / 256;
                    int b = y0 + (443 * (u - 128)) / 256;
                    dst[(y * w + x) * 3 + 0] = static_cast<std::uint8_t>(std::max(0, std::min(255, r)));
                    dst[(y * w + x) * 3 + 1] = static_cast<std::uint8_t>(std::max(0, std::min(255, g)));
                    dst[(y * w + x) * 3 + 2] = static_cast<std::uint8_t>(std::max(0, std::min(255, b)));
                }
            }
            header->stride = w * 3;
            std::strncpy(header->format, "RGB8", sizeof(header->format) - 1);
            header->format[sizeof(header->format) - 1] = '\0';
        } else {
            size_t pixelSize = srcStride * static_cast<size_t>(hh);
            std::memcpy(dst, src, std::min(pixelSize, v4l2MapLens_[buf.index]));
        }
        size_t pixelSize = static_cast<size_t>(header->stride) * static_cast<size_t>(header->height);
        ioctl(v4l2Fd_, VIDIOC_QBUF, &buf);

        auto pad = getPad("video_out");
        if (pad)
            pad->pushToConnections(frameBuffer_.data(), sizeof(CameraFramePacket) + pixelSize);
        return;
    }
#endif

    if (fileMode_ && fileStream_.is_open()) {
        std::uint8_t* dst = frameBuffer_.data() + sizeof(CameraFramePacket);
        fileStream_.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(fileFrameBytes_));
        if (fileStream_.gcount() == static_cast<std::streamsize>(fileFrameBytes_)) {
            auto pad = getPad("video_out");
            if (pad)
                pad->pushToConnections(frameBuffer_.data(), sizeof(CameraFramePacket) + fileFrameBytes_);
        }
        if (fileStream_.eof()) {
            fileStream_.clear();
            fileStream_.seekg(0);
        }
        return;
    }

    std::cout << "[CameraSourceNode] process: stub (no frame) from "
              << (config_.device.empty() ? config_.uri : config_.device) << std::endl;
}

} // namespace falconmind::sdk::sensors
