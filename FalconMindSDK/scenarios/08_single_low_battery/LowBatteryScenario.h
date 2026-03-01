/**
 * @file LowBatteryScenario.h
 * @brief 场景2.3低电量返航 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class LowBatteryScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // LOW_BATTERY特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit LowBatteryScenario(const Config& config);
    virtual ~LowBatteryScenario() = default;
    
    /**
     * @brief 执行场景2.3低电量返航任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成LOW_BATTERY搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief LOW_BATTERY特有逻辑
     */
    bool executeLOW_BATTERYLogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
