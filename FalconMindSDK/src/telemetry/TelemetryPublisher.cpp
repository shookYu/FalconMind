#include "falconmind/sdk/telemetry/TelemetryPublisher.h"

#include <algorithm>

namespace falconmind::sdk::telemetry {

int TelemetryPublisher::subscribe(const Handler& handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    int id = nextId_++;
    handlers_.emplace_back(id, handler);
    return id;
}

void TelemetryPublisher::unsubscribe(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    handlers_.erase(
        std::remove_if(handlers_.begin(), handlers_.end(),
                       [id](const auto& p) { return p.first == id; }),
        handlers_.end());
}

void TelemetryPublisher::publish(const TelemetryMessage& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, handler] : handlers_) {
        handler(msg);
    }
}

TelemetryPublisher& TelemetryPublisher::instance() {
    static TelemetryPublisher inst;
    return inst;
}

} // namespace falconmind::sdk::telemetry
