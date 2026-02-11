// FalconMindSDK - Caps (capabilities) skeleton
#pragma once

#include <string>
#include <unordered_map>

namespace falconmind::sdk::core {

class Caps {
public:
    Caps() = default;

    void set(const std::string& key, const std::string& value);
    const std::unordered_map<std::string, std::string>& properties() const noexcept { return props_; }

private:
    std::unordered_map<std::string, std::string> props_;
};

} // namespace falconmind::sdk::core

