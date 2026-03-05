/**
 * @file IBVSController.h
 * @brief Image-Based Visual Servoing (IBVS) 控制器
 * 
 * 基于图像的视觉伺服控制，实现无人机对目标的跟踪控制：
 * - 图像空间误差计算
 * - PID距离控制
 * - 自适应增益调整
 * - 速度指令生成
 * 
 * @author FalconMind SDK Team
 * @version 1.0.0
 * @date 2026-03-04
 */

#pragma once

#include <memory>
#include <chrono>
#include <falconmind/sdk/core/Node.h>
#include <falconmind/sdk/flight/FlightTypes.h>

namespace falconmind {
namespace sdk {
namespace control {

/**
 * @brief 相机参数
 */
struct CameraParameters {
    int width{1920};        /// 图像宽度
    int height{1080};       /// 图像高度
    double fx{1000.0};      /// x方向焦距 (像素)
    double fy{1000.0};      /// y方向焦距 (像素)
    double cx{960.0};       /// x方向光心
    double cy{540.0};       /// y方向光心
    
    /**
     * @brief 获取图像中心
     */
    std::pair<double, double> getImageCenter() const {
        return {cx, cy};
    }
};

/**
 * @brief 图像空间目标位置
 */
struct ImageSpaceTarget {
    double u{0.0};          /// 归一化x坐标 (-1 到 1)
    double v{0.0};          /// 归一化y坐标 (-1 到 1)
    double area_ratio{0.0}; /// 目标占图像面积比例 (0 到 1)
    
    /**
     * @brief 从像素坐标创建
     * @param pixel_x 像素x坐标
     * @param pixel_y 像素y坐标
     * @param camera 相机参数
     */
    static ImageSpaceTarget fromPixel(
        double pixel_x, double pixel_y, 
        const CameraParameters& camera
    ) {
        ImageSpaceTarget target;
        target.u = (pixel_x - camera.cx) / (camera.width / 2.0);
        target.v = (pixel_y - camera.cy) / (camera.height / 2.0);
        return target;
    }
};

/**
 * @brief 速度指令
 */
struct VelocityCommand {
    double vx{0.0};         /// 前向速度 (m/s，正为前进)
    double vy{0.0};         /// 右向速度 (m/s，正为右移)
    double vz{0.0};         /// 下向速度 (m/s，正为下降)
    double yaw_rate{0.0};   /// 偏航角速度 (rad/s)
    std::chrono::steady_clock::time_point timestamp;
    
    /**
     * @brief 限制速度大小
     */
    void saturate(double max_v_xy, double max_v_z, double max_yaw_rate) {
        double v_xy = std::sqrt(vx * vx + vy * vy);
        if (v_xy > max_v_xy) {
            double scale = max_v_xy / v_xy;
            vx *= scale;
            vy *= scale;
        }
        vz = std::max(-max_v_z, std::min(max_v_z, vz));
        yaw_rate = std::max(-max_yaw_rate, std::min(max_yaw_rate, yaw_rate));
    }
    
    /**
     * @brief 转换为MAVLink速度指令
     */
    flight::VelocityNED toMAVLinkVelocity() const {
        return flight::VelocityNED{vx, vy, vz, yaw_rate};
    }
};

/**
 * @brief IBVS控制器配置
 */
struct IBVSConfig {
    // 目标参数
    double desired_distance{30.0};      /// 期望距离 (m)
    double desired_height{10.0};        /// 期望高度 (m)
    double distance_tolerance{2.0};     /// 距离容差 (m)
    double height_tolerance{1.0};       /// 高度容差 (m)
    
    // 速度限制
    double max_speed{8.0};              /// 最大水平速度 (m/s)
    double max_vertical_speed{3.0};     /// 最大垂直速度 (m/s)
    double max_yaw_rate{1.0};           /// 最大偏航角速度 (rad/s)
    
    // PID参数 - 距离控制
    double kp_distance{0.5};            /// 距离比例增益
    double ki_distance{0.1};            /// 距离积分增益
    double kd_distance{0.2};            /// 距离微分增益
    
    // PID参数 - 位置控制
    double kp_position{0.01};           /// 位置比例增益
    double kp_height{0.1};              /// 高度比例增益
    double kp_yaw{0.2};                 /// 偏航比例增益
    
    // 自适应参数
    bool enable_adaptive_gain{true};    /// 启用自适应增益
    double min_distance_for_full_speed{50.0}; /// 全速最小距离
    
    // 相机参数
    CameraParameters camera;
};

/**
 * @brief 跟踪质量
 */
struct TrackingQuality {
    double distance_error{0.0};         /// 距离误差 (m)
    double position_error_u{0.0};       /// 水平位置误差
    double position_error_v{0.0};       /// 垂直位置误差
    double quality_score{1.0};          /// 质量评分 0-1
    bool is_distance_good{false};       /// 距离是否达标
    bool is_position_good{false};       /// 位置是否达标
};

/**
 * @brief Image-Based Visual Servoing 控制器
 * 
 * IBVS视觉伺服控制器，基于图像空间误差计算控制指令，
 * 实现无人机对目标的精确跟踪。
 * 
 * 控制原理：
 * - 距离误差 → 前后运动 (vx)
 * - 水平图像误差 → 左右运动 (vy) + 偏航控制 (yaw_rate)
 * - 垂直图像误差 → 上下运动 (vz)
 * 
 * 使用示例：
 * @code
 * IBVSController controller(config);
 * 
 * // 目标在图像中心偏右
 * ImageSpaceTarget target{0.1, -0.05, 0.05};  // u, v, area_ratio
 * double current_distance = 32.5;  // 估计距离
 * double current_height = 9.8;     // 估计高度
 * 
 * // 计算控制指令
 * auto cmd = controller.computeControl(target, current_distance, current_height);
 * 
 * // 发送到飞控
 * mavlink.sendVelocity(cmd.vx, cmd.vy, cmd.vz, cmd.yaw_rate);
 * @endcode
 */
class IBVSController : public core::Node {
public:
    /**
     * @brief 构造函数
     * @param config 控制器配置
     */
    explicit IBVSController(const IBVSConfig& config = IBVSConfig{});
    
    ~IBVSController() override = default;
    
    /**
     * @brief 初始化控制器
     */
    bool initialize() override;
    
    /**
     * @brief 计算控制指令
     * 
     * @param target 图像空间目标位置
     * @param current_distance 当前估计距离 (m)
     * @param current_height 当前估计高度 (m)
     * @return 速度控制指令
     */
    VelocityCommand computeControl(
        const ImageSpaceTarget& target,
        double current_distance,
        double current_height
    );
    
    /**
     * @brief 计算跟踪质量
     * @param current_distance 当前距离
     * @param current_height 当前高度
     * @param target 目标位置
     * @return 跟踪质量
     */
    TrackingQuality computeQuality(
        double current_distance,
        double current_height,
        const ImageSpaceTarget& target
    ) const;
    
    /**
     * @brief 设置目标距离
     */
    void setDesiredDistance(double distance);
    
    /**
     * @brief 设置目标高度
     */
    void setDesiredHeight(double height);
    
    /**
     * @brief 获取当前配置
     */
    IBVSConfig getConfig() const;
    
    /**
     * @brief 更新配置
     */
    void updateConfig(const IBVSConfig& config);
    
    /**
     * @brief 重置控制器状态
     */
    void reset();
    
    /**
     * @brief 是否到达目标状态
     */
    bool isAtTarget(
        double current_distance,
        double current_height,
        const ImageSpaceTarget& target
    ) const;

protected:
    /**
     * @brief 计算距离控制 (PID)
     * @param error 距离误差
     * @param dt 时间差
     * @return 控制输出
     */
    double computeDistanceControl(double error, double dt);
    
    /**
     * @brief 计算自适应增益
     * @param distance 当前距离
     * @return 增益系数
     */
    double computeAdaptiveGain(double distance) const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * @brief 便捷函数：创建标准IBVS控制器
 */
std::shared_ptr<IBVSController> createIBVSController(
    const IBVSConfig& config = IBVSConfig{}
);

/**
 * @brief 便捷函数：创建保守模式控制器（更稳定但响应慢）
 */
std::shared_ptr<IBVSController> createConservativeIBVSController();

/**
 * @brief 便捷函数：创建激进模式控制器（响应快但可能超调）
 */
std::shared_ptr<IBVSController> createAggressiveIBVSController();

} // namespace control
} // namespace sdk
} // namespace falconmind
