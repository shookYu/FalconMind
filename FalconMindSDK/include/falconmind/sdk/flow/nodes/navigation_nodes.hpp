/**
 * @file navigation_nodes.hpp
 * @brief 导航相关节点 - 使用真实SDK功能
 * 
 * 依赖:
 * - FlightConnectionService: MAVLink飞控通信
 */

#pragma once

#include "falconmind/sdk/flow/flow_node.hpp"
#include <cmath>
#include <vector>

namespace falconmind {
namespace sdk {
namespace flow {
namespace nodes {

/**
 * @brief 搜索航点生成节点
 * 
 * 生成搜索区域覆盖航点，支持多种模式：
 * - LAWN_MOWER: 割草机模式（往返扫描）
 * - SPIRAL: 螺旋模式（从中心向外）
 * - ZIGZAG: Z字形模式
 */
class SearchPatternGeneratorNode : public FlowNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "SearchPatternGenerator"; }
    
    std::vector<NodePort> getInputPorts() const override {
        return {
            {"area", "object", "搜索区域（bounds或center+radius）"},
            {"altitude", "float", "搜索高度（米）"},
            {"speed", "float", "搜索速度（米/秒）"}
        };
    }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"waypoints", "array", "生成的航点列表"},
            {"waypoint_count", "int", "航点数量"},
            {"estimated_duration_seconds", "float", "预计执行时间（秒）"}
        };
    }
    
    bool configure(const json& config) override;
    NodeResult execute(NodeContext& context) override;

private:
    std::string pattern_ = "LAWN_MOWER";
    double overlap_rate_ = 0.2;
    double lane_width_ = 20.0;  // meters
    double camera_fov_ = 60.0;  // degrees
    
    json generateWaypoints(const json& area, double altitude, double speed);
    
    json generateLawnMowerPattern(
        double min_lat, double min_lon, double max_lat, double max_lon,
        double altitude, double speed, double lane_width);
    
    json generateSpiralPattern(
        double min_lat, double min_lon, double max_lat, double max_lon,
        double altitude, double speed, double lane_width);
    
    json generateZigzagPattern(
        double min_lat, double min_lon, double max_lat, double max_lon,
        double altitude, double speed, double lane_width);
    
    double estimateDuration(const json& waypoints, double speed);
};

REGISTER_NODE(SearchPatternGeneratorNode)

/**
 * @brief 航点执行节点
 * 
 * 通过MAVLink上传并执行航点任务
 */
class ExecuteWaypointsNode : public FlowNode {
public:
    NodeType getType() const override { return NodeType::ACTION; }
    std::string getName() const override { return "ExecuteWaypoints"; }
    
    std::vector<NodePort> getInputPorts() const override {
        return {
            {"waypoints", "array", "航点列表"},
            {"speed", "float", "飞行速度（米/秒）"}
        };
    }
    
    std::vector<NodePort> getOutputPorts() const override {
        return {
            {"completed", "bool", "是否完成"},
            {"current_waypoint", "int", "当前航点索引"},
            {"current_waypoint_id", "int", "当前航点ID"}
        };
    }
    
    bool configure(const json& config) override;
    NodeResult execute(NodeContext& context) override;

private:
    std::string connection_string_ = "udp://127.0.0.1:14540";
    
    bool uploadWaypointsToFlightController(const json& waypoints);
};

REGISTER_NODE(ExecuteWaypointsNode)

} // namespace nodes
} // namespace flow
} // namespace sdk
} // namespace falconmind
