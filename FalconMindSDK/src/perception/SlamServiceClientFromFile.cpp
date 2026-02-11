#include "falconmind/sdk/perception/ISlamServiceClient.h"

#include <fstream>

namespace falconmind::sdk::perception {

bool SlamServiceClientFromFile::isAvailable() const {
    if (poseFilePath_.empty()) return false;
    std::ifstream f(poseFilePath_, std::ios::binary);
    return f.is_open();
}

bool SlamServiceClientFromFile::getPose(Pose3D& pose) {
    if (poseFilePath_.empty()) return false;
    std::ifstream f(poseFilePath_, std::ios::binary);
    if (!f.is_open()) return false;
    f.read(reinterpret_cast<char*>(&pose), sizeof(Pose3D));
    return f.gcount() == static_cast<std::streamsize>(sizeof(Pose3D));
}

} // namespace falconmind::sdk::perception
