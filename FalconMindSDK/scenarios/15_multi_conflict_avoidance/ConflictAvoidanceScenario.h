/**
 * @file ConflictAvoidanceScenario.h
 * @brief 场景4.2冲突避免 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class ConflictAvoidanceScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // CONFLICT_AVOID特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit ConflictAvoidanceScenario(const Config& config);
    virtual ~ConflictAvoidanceScenario() = default;
    
    /**
     * @brief 执行场景4.2冲突避免任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成CONFLICT_AVOID搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief CONFLICT_AVOID特有逻辑
     */
    bool executeCONFLICT_AVOIDLogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
