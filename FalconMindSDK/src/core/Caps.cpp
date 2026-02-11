#include "falconmind/sdk/core/Caps.h"

namespace falconmind::sdk::core {

void Caps::set(const std::string& key, const std::string& value) {
    props_[key] = value;
}

} // namespace falconmind::sdk::core

