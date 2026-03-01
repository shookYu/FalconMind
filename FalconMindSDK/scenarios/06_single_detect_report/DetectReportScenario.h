/**
 * @file DetectReportScenario.h
 * @brief 场景2.1检测上报 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class DetectReportScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // DETECT_REPORT特定配置
        float param1 = 100.0f;
        float param2 = 50.0f;
    };
    
    explicit DetectReportScenario(const Config& config);
    virtual ~DetectReportScenario() = default;
    
    /**
     * @brief 执行场景2.1检测上报任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成DETECT_REPORT搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief DETECT_REPORT特有逻辑
     */
    bool executeDETECT_REPORTLogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
