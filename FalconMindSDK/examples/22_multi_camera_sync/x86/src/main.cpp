#include <iostream>
#include <iomanip>
#include <vector>
#include <chrono>
#include <thread>

// SDK includes

struct CameraFrame {
    int cameraId;
    uint64_t timestamp;
    uint64_t hardwareTimestamp;
    bool synced;
};

class MultiCameraSync {
public:
    static const int MAX_CAMERAS = 4;
    
    bool initialize() {
        std::cout << "[Sync] Initializing multi-camera synchronization..." << std::endl;
        std::cout << "  Cameras: " << MAX_CAMERAS << std::endl;
        std::cout << "  Sync method: Hardware trigger (GPIO)" << std::endl;
        std::cout << "  Sync tolerance: 1ms" << std::endl;
        return true;
    }
    
    std::vector<CameraFrame> captureSyncFrame() {
        std::vector<CameraFrame> frames;
        uint64_t baseTime = getCurrentTime();
        
        for (int i = 0; i < MAX_CAMERAS; ++i) {
            CameraFrame frame;
            frame.cameraId = i;
            frame.timestamp = baseTime;
            frame.hardwareTimestamp = baseTime + (i * 100);
            frame.synced = (i < 2);
            frames.push_back(frame);
        }
        
        return frames;
    }
    
    void printSyncStatus(const std::vector<CameraFrame>& frames) {
        std::cout << "  Camera timestamps: ";
        for (const auto& f : frames) {
            std::cout << "Cam" << f.cameraId << "=" << f.hardwareTimestamp;
            if (!f.synced) std::cout << "[ASYNC]";
            std::cout << " ";
        }
        std::cout << std::endl;
    }

private:
    uint64_t getCurrentTime() {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
    }
};

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Example 22: Multi-Camera Synchronization" << std::endl;
    std::cout << "========================================" << std::endl << std::endl;
    
    MultiCameraSync sync;
    if (!sync.initialize()) {
        std::cerr << "Failed to initialize camera sync" << std::endl;
        return 1;
    }
    
    std::cout << std::endl << "Capturing synchronized frames..." << std::endl;
    
    for (int i = 0; i < 10; ++i) {
        auto frames = sync.captureSyncFrame();
        std::cout << "Frame " << std::setw(2) << i << ": ";
        sync.printSyncStatus(frames);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << std::endl << "Multi-camera sync demo complete!" << std::endl;
    return 0;
}
