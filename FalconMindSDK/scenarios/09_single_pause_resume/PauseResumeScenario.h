/**
 * @file PauseResumeScenario.h
 * @brief 场景2.4暂停恢复 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class PauseResumeScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // PAUSE_RESUME特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit PauseResumeScenario(const Config& config);
    virtual ~PauseResumeScenario() = default;
    
    /**
     * @brief 执行场景2.4暂停恢复任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成PAUSE_RESUME搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief PAUSE_RESUME特有逻辑
     */
    bool executePAUSE_RESUMELogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
