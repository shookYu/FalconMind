// FalconMindSDK - Pad interface (week1 skeleton)
#pragma once

#include <string>
#include <memory>
#include <functional>
#include <vector>

namespace falconmind::sdk::core {

enum class PadType {
    Source,
    Sink
};

// 前向声明
class Pad;

// Pad连接信息
struct PadConnection {
    std::weak_ptr<Pad> targetPad;  // 目标Pad（弱引用，避免循环引用）
    std::string targetNodeId;      // 目标节点ID
    std::string targetPadName;     // 目标Pad名称
};

class Pad {
public:
    Pad(std::string name, PadType type);

    const std::string& name() const noexcept { return name_; }
    PadType type() const noexcept { return type_; }
    
    // 连接管理
    bool connectTo(std::shared_ptr<Pad> targetPad, const std::string& targetNodeId, const std::string& targetPadName);
    bool disconnect();
    const std::vector<PadConnection>& connections() const noexcept { return connections_; }
    bool isConnected() const noexcept { return !connections_.empty(); }
    
    // 数据传递回调（Sink Pad 设置，Source Pad 通过 pushToConnections 调用）
    using DataCallback = std::function<void(const void* data, size_t size)>;
    void setDataCallback(DataCallback callback) { dataCallback_ = callback; }
    DataCallback getDataCallback() const { return dataCallback_; }

    /** Source Pad：将 data/size 推送到所有已连接的 Sink Pad 的 DataCallback；非 Source 或 data 为空则忽略 */
    void pushToConnections(const void* data, size_t size) const;

private:
    std::string name_;
    PadType type_;
    std::vector<PadConnection> connections_;  // 连接列表（Source Pad可以有多个连接）
    DataCallback dataCallback_;  // 数据传递回调
};

} // namespace falconmind::sdk::core

