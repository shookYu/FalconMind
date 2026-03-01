/**
 * @file MultiVoronoiScenario.h
 * @brief 场景3.2Voronoi分割 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class MultiVoronoiScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // VORONOI特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit MultiVoronoiScenario(const Config& config);
    virtual ~MultiVoronoiScenario() = default;
    
    /**
     * @brief 执行场景3.2Voronoi分割任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成VORONOI搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief VORONOI特有逻辑
     */
    bool executeVORONOILogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
