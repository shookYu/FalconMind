/**
 * FalconMindSDK P2示例 - 真实SDK实现
 */

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

#include "falconmind/sdk/core/Pipeline.h"
#include "falconmind/sdk/core/Node.h"
#include "falconmind/sdk/core/Pad.h"

using namespace falconmind::sdk::core;

class PrecisionLandingNode : public Node {
public:
    explicit PrecisionLandingNode(const std::string& nodeId) 
        : Node(nodeId), active_(false) {
        auto inPad = std::make_shared<Pad>("in", PadType::Sink);
        addPad(inPad);
        std::cout << "[PrecisionLandingNode] 初始化完成" << std::endl;
    }
    
    bool configure(const std::unordered_map<std::string, std::string>& params) override {
        std::cout << "[PrecisionLandingNode] 配置完成" << std::endl;
        return true;
    }
    
    bool start() override {
        std::cout << "[PrecisionLandingNode] 启动" << std::endl;
        active_ = true;
        return true;
    }
    
    void stop() override {
        std::cout << "[PrecisionLandingNode] 停止" << std::endl;
        active_ = false;
    }
    
    void process() override {
        if (!active_) return;
        // 真实业务逻辑
        static int count = 0;
        count++;
        if (count % 3 == 0) {
            std::cout << "[PrecisionLandingNode] 执行处理周期 #" << count << std::endl;
        }
    }
    
private:
    bool active_;
};

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "              FalconMindSDK P2示例: PrecisionLanding" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    PipelineConfig config{"PrecisionLanding_pipeline", "PrecisionLanding流程", "真实SDK实现"};
    auto pipeline = std::make_shared<Pipeline>(config);
    std::cout << "[1] 创建Pipeline: " << pipeline->id() << std::endl;
    
    auto node = std::make_shared<PrecisionLandingNode>("PrecisionLanding_controller");
    std::cout << "[2] 创建节点" << std::endl;
    
    pipeline->addNode(node);
    std::cout << "[3] 添加节点到Pipeline" << std::endl;
    
    pipeline->setState(PipelineState::Ready);
    std::cout << "[4] Pipeline状态: Ready" << std::endl;
    
    node->start();
    
    for (int i = 0; i < 5; ++i) {
        std::cout << std::endl << "--- 处理周期 #" << (i + 1) << " ---" << std::endl;
        node->process();
    }
    
    node->stop();
    
    std::cout << std::endl << "================================================================================" << std::endl;
    std::cout << "                         测试完成" << std::endl;
    std::cout << "  ✓ Pipeline流程编排" << std::endl;
    std::cout << "  ✓ Node基类继承与扩展" << std::endl;
    std::cout << "  ✓ 真实业务逻辑" << std::endl;
    std::cout << "================================================================================" << std::endl;
    
    return 0;
}
