/**
 * @file gps_defense_nodes.hpp
 * @brief GPS欺骗防护节点 - 使用真实SDK功能
 * 
 * 依赖:
 * - GPSDefender: GPS欺骗检测与防护系统
 */

#pragma once

#include "falconmind/sdk/flow/flow_node.hpp"
#include "falconmind/sdk/navigation/GPSDefender.h"
#include <memory>

namespace falconmind {
namespace sdk {
namespace flow {
namespace nodes {

// Forward declaration of simple GPS defender (defined in .cpp due to SDK PIMPL issue)
class SimpleGPSDefender;

/**
 * @brief GPS欺骗检测节点
 * 
 * 使用GPSDefender进行多源融合欺骗检测：
 * - RAIM一致性检查
 * - IMU速度一致性验证
 * - VINS位置交叉验证
 */
class GPSDefenseActivatorNode : public BackgroundNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "GPSDefenseActivator"; }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"defense_active", "bool", "防护是否激活"},
            {"detection_mode", "string", "检测模式"}
        };
    }
    
    bool configure(const json& config) override;
    NodeResult execute(NodeContext& context) override;
    void runBackground(NodeContext& context) override;

private:
    bool raim_check_ = true;
    bool imu_consistency_check_ = true;
    bool vins_consistency_check_ = true;
    double check_interval_ = 1.0;
    double raim_threshold_ = 5.0;
    double velocity_diff_threshold_ = 3.0;
    double position_diff_threshold_ = 10.0;
    
    // GPSDefender instance (using stub due to SDK PIMPL issue)
    std::unique_ptr<SimpleGPSDefender> simple_defender_;
    
    
    // Helper methods to get sensor data
    navigation::GNSSMeasurement getLatestGNSS(NodeContext& context);
    navigation::IMUMeasurement getLatestIMU(NodeContext& context);
    navigation::VisualPosition getLatestVINSPosition(NodeContext& context);
};

REGISTER_NODE(GPSDefenseActivatorNode)

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
