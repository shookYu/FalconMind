/**
 * @file E2EMultiScenario.h
 * @brief 场景6.2多机端到端 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class E2EMultiScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // E2E_MULTI特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit E2EMultiScenario(const Config& config);
    virtual ~E2EMultiScenario() = default;
    
    /**
     * @brief 执行场景6.2多机端到端任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成E2E_MULTI搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief E2E_MULTI特有逻辑
     */
    bool executeE2E_MULTILogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
