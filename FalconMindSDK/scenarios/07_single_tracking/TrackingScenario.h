/**
 * @file TrackingScenario.h
 * @brief 场景2.2目标跟踪 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class TrackingScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // TRACKING特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit TrackingScenario(const Config& config);
    virtual ~TrackingScenario() = default;
    
    /**
     * @brief 执行场景2.2目标跟踪任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成TRACKING搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief TRACKING特有逻辑
     */
    bool executeTRACKINGLogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
