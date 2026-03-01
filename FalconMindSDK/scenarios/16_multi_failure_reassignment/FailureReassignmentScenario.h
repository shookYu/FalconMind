/**
 * @file FailureReassignmentScenario.h
 * @brief 场景4.3故障重分配 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class FailureReassignmentScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // FAILURE_REASSIGN特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit FailureReassignmentScenario(const Config& config);
    virtual ~FailureReassignmentScenario() = default;
    
    /**
     * @brief 执行场景4.3故障重分配任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成FAILURE_REASSIGN搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief FAILURE_REASSIGN特有逻辑
     */
    bool executeFAILURE_REASSIGNLogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
