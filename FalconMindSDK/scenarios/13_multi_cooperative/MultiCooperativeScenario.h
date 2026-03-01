/**
 * @file MultiCooperativeScenario.h
 * @brief 场景3.4协同发现 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class MultiCooperativeScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // COOPERATIVE特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit MultiCooperativeScenario(const Config& config);
    virtual ~MultiCooperativeScenario() = default;
    
    /**
     * @brief 执行场景3.4协同发现任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成COOPERATIVE搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief COOPERATIVE特有逻辑
     */
    bool executeCOOPERATIVELogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
