/**
 * @file AdvancedVoronoiScenario.h
 * @brief 场景4.1高级Voronoi - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class AdvancedVoronoiScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // ADV_VORONOI特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit AdvancedVoronoiScenario(const Config& config);
    virtual ~AdvancedVoronoiScenario() = default;
    
    /**
     * @brief 执行场景4.1高级Voronoi任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成ADV_VORONOI搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief ADV_VORONOI特有逻辑
     */
    bool executeADV_VORONOILogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
