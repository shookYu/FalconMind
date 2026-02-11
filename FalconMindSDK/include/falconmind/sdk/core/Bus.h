// FalconMindSDK - Bus skeleton for events/logs between nodes and pipeline
#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace falconmind::sdk::core {

struct BusMessage {
    std::string category;
    std::string text;
};

class Bus {
public:
    using Handler = std::function<void(const BusMessage&)>;

    int subscribe(const Handler& handler);
    void unsubscribe(int id);

    void post(const BusMessage& msg);

private:
    int nextId_{1};
    std::unordered_map<int, Handler> handlers_;
};

} // namespace falconmind::sdk::core

