/**
 * @file SectorScenario.h
 * @brief 场景1.4扇形搜索 - 真实飞控连接版本
 * 
 * ⚠️ 真实实现：通过真实MAVLink连接PX4飞控
 */

#pragma once

#include "../common/RealScenarioBase.h"

namespace falconmind {
namespace scenarios {

class SectorScenario : public RealScenarioBase {
public:
    struct Config : public RealScenarioConfig {
        // SECTOR特定配置 - 扇形参数
        double centerLat = 34.052200;   // 扇形中心纬度
        double centerLon = -118.243700; // 扇形中心经度
        float radius = 100.0f;          // 扇形半径(m)
        float startAngle = -45.0f;      // 起始角度(度，0为正东)
        float sweepAngle = 90.0f;       // 扫掠角度(度)
        int numArcs = 5;                // 弧线数量
    };
    
    explicit SectorScenario(const Config& config);
    virtual ~SectorScenario() = default;
    
    /**
     * @brief 执行场景1.4扇形搜索任务（真实飞控）
     */
    bool execute() override;

protected:
    /**
     * @brief 生成SECTOR搜索路径
     */
    std::vector<Waypoint> generateSearchPath();
    
    /**
     * @brief SECTOR特有逻辑
     */
    bool executeSECTORLogic();

private:
    Config config_;
};

} // namespace scenarios
} // namespace falconmind
