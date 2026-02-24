/**
 * Example 11: Camera Capture - V4L2 Camera Acquisition
 * 
 * This example demonstrates:
 * - Opening V4L2 camera device (e.g., /dev/video0)
 * - Configuring resolution and pixel format
 * - Real-time frame capture using MMAP
 * - YUYV to RGB conversion
 * - Frame data output via Pad callbacks
 * - File mode for testing without camera
 * 
 * Usage:
 *   ./11_camera_capture_x86 [device|file] [width] [height]
 *   
 *   Device Mode (default): ./11_camera_capture_x86 /dev/video0 640 480
 *   File Mode: ./11_camera_capture_x86 file:/path/to/video.rgb 640 480
 * 
 * Prerequisites:
 *   - Linux system with V4L2 support
 *   - Camera device connected (/dev/video0 or other)
 *   - SDK built with camera support
 */

#include <falconmind/sdk/core/Pipeline.h>
#include <falconmind/sdk/core/Node.h>
#include <falconmind/sdk/core/Pad.h>
#include <falconmind/sdk/sensors/CameraSourceNode.h>
#include <falconmind/sdk/sensors/VideoSourceConfig.h>
#include <falconmind/sdk/sensors/CameraFramePacket.h>

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <iomanip>
#include <atomic>

using namespace falconmind::sdk::core;
using namespace falconmind::sdk::sensors;

// Global counters for received frames
std::atomic<int> frameCount{0};
std::atomic<long long> totalBytes{0};

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "FalconMindSDK - Camera Capture Example" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    // Parse command line arguments
    std::string device = "/dev/video0";
    std::string fileUri;
    bool fileMode = false;
    unsigned width = 640;
    unsigned height = 480;

    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1.size() >= 5 && arg1.substr(0, 5) == "file:") {
            fileMode = true;
            fileUri = arg1;
            device.clear();
        } else {
            device = arg1;
        }
    }
    if (argc > 2) width = static_cast<unsigned>(std::stoul(argv[2]));
    if (argc > 3) height = static_cast<unsigned>(std::stoul(argv[3]));

    if (fileMode) {
        std::cout << "File mode: " << fileUri << std::endl;
    } else {
        std::cout << "Camera device: " << device << std::endl;
    }
    std::cout << "Resolution: " << width << "x" << height << std::endl;
    std::cout << std::endl;

    // Create camera source node
    VideoSourceConfig cfg;
    cfg.device = device;
    cfg.uri = fileUri;
    cfg.width = width;
    cfg.height = height;
    cfg.fps = 30;
    
    auto cameraSource = std::make_shared<CameraSourceNode>(cfg);
    
    // Setup frame callback
    auto videoOutPad = cameraSource->getPad("video_out");
    if (videoOutPad) {
        videoOutPad->setDataCallback([](const void* data, size_t size) {
            if (size >= sizeof(CameraFramePacket)) {
                const CameraFramePacket* header = static_cast<const CameraFramePacket*>(data);
                const size_t pixelDataSize = size - sizeof(CameraFramePacket);
                
                frameCount++;
                totalBytes += size;
                
                // Display every 30 frames (approx 1 second at 30fps)
                if (frameCount % 30 == 0) {
                    std::cout << "\r[Frame " << std::setw(5) << frameCount << "] "
                              << header->width << "x" << header->height 
                              << " format=" << header->format
                              << " stride=" << header->stride
                              << " bytes=" << pixelDataSize
                              << " total=" << (totalBytes / 1024 / 1024) << "MB"
                              << std::flush;
                }
            }
        });
    }
    
    // Start camera
    if (!cameraSource->start()) {
        std::cerr << "Failed to start camera. Possible reasons:" << std::endl;
        std::cerr << "  - Camera device not found: " << device << std::endl;
        std::cerr << "  - No permission to access camera (try sudo)" << std::endl;
        std::cerr << "  - Camera is being used by another process" << std::endl;
        std::cerr << "  - Requested resolution not supported" << std::endl;
        return 1;
    }
    
    std::cout << "Camera started successfully!" << std::endl;
    std::cout << "Capturing frames... Press Ctrl+C to stop." << std::endl;
    std::cout << std::endl;
    
    // Capture for 10 seconds at 30fps
    const int targetFrames = 300; // 10 seconds * 30fps
    for (int i = 0; i < targetFrames; ++i) {
        cameraSource->process();
        std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30fps
    }
    
    std::cout << std::endl << std::endl;
    std::cout << "Capture complete!" << std::endl;
    std::cout << "Total frames: " << frameCount << std::endl;
    std::cout << "Total data: " << (totalBytes / 1024 / 1024) << " MB" << std::endl;
    std::cout << "Average FPS: " << std::fixed << std::setprecision(1) 
              << (frameCount / 10.0) << std::endl;
    
    // Stop camera
    cameraSource->stop();
    
    std::cout << "Done!" << std::endl;
    
    return 0;
}
