/**
 * @file MultiAgriSprayingScenario.h
 * @brief 场景3.3农业喷洒 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class MultiAgriSprayingScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // AGRI_SPRAYING特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit MultiAgriSprayingScenario(const Config& config);
    virtual ~MultiAgriSprayingScenario() = default;
    
    /**
     * @brief 执行场景3.3农业喷洒任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成AGRI_SPRAYING搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief AGRI_SPRAYING特有逻辑
     */
    bool executeAGRI_SPRAYINGLogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
