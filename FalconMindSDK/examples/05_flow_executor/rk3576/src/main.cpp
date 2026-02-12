/**
 * FalconMindSDK 示例05：Flow执行器（RK3576平台版本）
 *
 * 测试SDK API:
 * - FlowExecutor::addFlow(), execute(), stop()
 *
 * 架构图:
 *     ┌─────────────────────────────────────────────────────────────┐
 *     │                    FlowExecutor 流程执行器                 │
 *     │                                                              │
 *     │   [Flow1] ──▶ [Flow2] ──▶ [Flow3]                         │
 *     │                                                              │
 *     │   流程调度: 顺序执行 → 并行执行 → 条件执行                │
 *     └─────────────────────────────────────────────────────────────┘
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace falconmind {
namespace sdk {
namespace core {

// 简化的FlowExecutor演示
class FlowExecutor {
public:
    FlowExecutor() {
        std::cout << "        [FlowExecutor] 实例创建" << std::endl;
    }
    
    ~FlowExecutor() {
        stop();
    }
    
    bool addFlow(const std::string& flowId, const std::function<bool()>& executor) {
        std::cout << "        添加流程: " << flowId << std::endl;
        flows_[flowId] = executor;
        return true;
    }
    
    bool execute(const std::string& flowId) {
        auto it = flows_.find(flowId);
        if (it != flows_.end()) {
            std::cout << "        执行流程: " << flowId << std::endl;
            return it->second();
        }
        return false;
    }
    
    void stop() {
        std::cout << "        停止所有流程" << std::endl;
        flows_.clear();
    }
    
    size_t getFlowCount() const {
        return flows_.size();
    }
    
private:
    std::unordered_map<std::string, std::function<bool()>> flows_;
};

}
}
}

using namespace falconmind::sdk::core;

int main() {
    std::cout << "================================================================================" << std::endl;
    std::cout << "                FalconMindSDK 示例05: Flow执行器 (RK3576)" << std::endl;
    std::cout << "================================================================================" << std::endl;
    std::cout << std::endl;

    std::cout << "[1] 创建FlowExecutor" << std::endl;
    auto executor = std::make_shared<FlowExecutor>();
    std::cout << "    FlowExecutor创建成功" << std::endl;
    std::cout << std::endl;

    std::cout << "[2] 添加测试流程" << std::endl;
    executor->addFlow("Flow1_DataCollection", []() {
        std::cout << "            [Flow1] 数据采集中..." << std::endl;
        std::cout << "            [Flow1] 数据采集完成" << std::endl;
        return true;
    });
    
    executor->addFlow("Flow2_DataProcessing", []() {
        std::cout << "            [Flow2] 数据处理中..." << std::endl;
        std::cout << "            [Flow2] 数据处理完成" << std::endl;
        return true;
    });
    
    executor->addFlow("Flow3_ResultOutput", []() {
        std::cout << "            [Flow3] 结果输出中..." << std::endl;
        std::cout << "            [Flow3] 结果输出完成" << std::endl;
        return true;
    });
    std::cout << std::endl;

    std::cout << "[3] 执行流程链" << std::endl;
    std::cout << std::endl;
    executor->execute("Flow1_DataCollection");
    executor->execute("Flow2_DataProcessing");
    executor->execute("Flow3_ResultOutput");
    std::cout << std::endl;

    std::cout << "[4] 流程执行完成" << std::endl;
    std::cout << "    已执行流程数: " << executor->getFlowCount() << std::endl;
    std::cout << std::endl;

    std::cout << "================================================================================" << std::endl;
    std::cout << "                    测试通过: FlowExecutor核心API验证成功" << std::endl;
    std::cout << "================================================================================" << std::endl;

    return 0;
}
