/**
 * @file WaypointListScenario.h
 * @brief 场景1.5航点列表 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class WaypointListScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // WAYPOINT_LIST特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit WaypointListScenario(const Config& config);
    virtual ~WaypointListScenario() = default;
    
    /**
     * @brief 执行场景1.5航点列表任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成WAYPOINT_LIST搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief WAYPOINT_LIST特有逻辑
     */
    bool executeWAYPOINT_LISTLogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
