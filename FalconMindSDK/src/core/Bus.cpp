#include "falconmind/sdk/core/Bus.h"

namespace falconmind::sdk::core {

int Bus::subscribe(const Handler& handler) {
    int id = nextId_++;
    handlers_.emplace(id, handler);
    return id;
}

void Bus::unsubscribe(int id) {
    handlers_.erase(id);
}

void Bus::post(const BusMessage& msg) {
    for (auto& [id, h] : handlers_) {
        (void)id;
        if (h) h(msg);
    }
}

} // namespace falconmind::sdk::core

