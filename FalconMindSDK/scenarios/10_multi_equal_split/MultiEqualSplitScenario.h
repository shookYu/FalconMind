/**
 * @file MultiEqualSplitScenario.h
 * @brief 场景3.1多机等分 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class MultiEqualSplitScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // EQUAL_SPLIT特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit MultiEqualSplitScenario(const Config& config);
    virtual ~MultiEqualSplitScenario() = default;
    
    /**
     * @brief 执行场景3.1多机等分任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成EQUAL_SPLIT搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief EQUAL_SPLIT特有逻辑
     */
    bool executeEQUAL_SPLITLogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
