/**
 * @file E2ESingleScenario.h
 * @brief 场景6.1单机端到端 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class E2ESingleScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // E2E_SINGLE特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit E2ESingleScenario(const Config& config);
    virtual ~E2ESingleScenario() = default;
    
    /**
     * @brief 执行场景6.1单机端到端任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成E2E_SINGLE搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief E2E_SINGLE特有逻辑
     */
    bool executeE2E_SINGLELogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
