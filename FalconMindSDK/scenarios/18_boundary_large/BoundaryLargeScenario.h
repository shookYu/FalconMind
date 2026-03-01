/**
 * @file BoundaryLargeScenario.h
 * @brief 场景5.2极大区域 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class BoundaryLargeScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // LARGE_AREA特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit BoundaryLargeScenario(const Config& config);
    virtual ~BoundaryLargeScenario() = default;
    
    /**
     * @brief 执行场景5.2极大区域任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成LARGE_AREA搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief LARGE_AREA特有逻辑
     */
    bool executeLARGE_AREALogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
