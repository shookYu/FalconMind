// FalconMindSDK - Node base interface (week1 skeleton)
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace falconmind::sdk::core {

class Pad;

class Node {
public:
    explicit Node(std::string id);
    virtual ~Node();

    const std::string& id() const noexcept { return id_; }
    void setId(const std::string& newId) { id_ = newId; }

    virtual bool configure(const std::unordered_map<std::string, std::string>& params);

    virtual bool start();
    virtual void stop();

    // 简化：先只提供一个“无类型”处理入口，后续再按 Caps/Buffer 抽象细化
    virtual void process();

    std::shared_ptr<Pad> getPad(const std::string& name);
    const std::unordered_map<std::string, std::shared_ptr<Pad>>& pads() const noexcept { return pads_; }

protected:
    void addPad(const std::shared_ptr<Pad>& pad);

private:
    std::string id_;
    std::unordered_map<std::string, std::shared_ptr<Pad>> pads_;
};

} // namespace falconmind::sdk::core

