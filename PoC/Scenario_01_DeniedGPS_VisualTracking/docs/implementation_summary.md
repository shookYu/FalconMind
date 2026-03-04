# PoC Scenario 01 - 功能推演总结

## 业务场景

**任务**：在拒止和GPS欺骗环境下实现区域侦查与视觉制导跟踪

**流程**：
1. VINS初始化（无GPS定位）
2. 区域侦查（YOLO检测 + 航点巡逻）
3. 目标发现 → 人工选择 → 确认
4. 视觉跟踪（IBVS控制，30米距离，10米高度）
5. 任务结束 → 返航

## 通过业务推演发现的问题

### 1. FalconMindViewer 需要完善的功能

#### A. VINS初始化界面
**发现**：拒止环境必须先完成VINS初始化才能起飞，但当前Viewer没有专门的VINS初始化界面。

**需求**：
- VINS状态实时监控面板
- 特征点数量显示
- 初始化进度条
- 相机画面显示（用于检查特征点分布）
- 初始化完成提示

**实现**：`01_Viewer_Approach/frontend/components/VINSInitPanel.vue`

#### B. 弱网通信机制
**发现**：拒止环境下通信可能中断，但跟踪任务不能中断。

**需求**：
- 命令队列本地缓存
- 断线重连自动恢复
- 降级模式切换
- 关键数据本地存储

**实现**：`01_Viewer_Approach/services/low_bandwidth_link.py`

#### C. 目标选择界面
**发现**：当前Viewer没有实时目标选择功能。

**需求**：
- 视频流叠加检测框
- 点击选择目标
- 目标列表展示
- 距离/置信度显示
- 确认/取消按钮

**实现**：`01_Viewer_Approach/frontend/components/TargetSelector.vue`

#### D. GPS欺骗状态显示
**发现**：需要实时显示GPS防护状态。

**需求**：
- GNSS状态指示器
- RAIM检查结果
- 欺骗警报显示
- 导航源切换提示

**实现**：`01_Viewer_Approach/services/gps_spoofing_guard.py`

### 2. FalconMindBuilder 需要完善的功能

#### A. 自定义节点系统
**发现**：Builder当前缺少视觉伺服、GPS防护等节点。

**新增节点**：

| 节点名称 | 功能 | 类别 | 是否背景 |
|---------|------|------|---------|
| `VINSStatusCheck` | 检查VINS初始化状态 | 条件 | 否 |
| `GPSDefenseActivator` | 启动GPS欺骗防护 | 动作 | 是 |
| `VisualDetector` | 启动YOLO检测 | 动作 | 是 |
| `TargetSelectionAwaiter` | 等待人工选择目标 | 动作 | 否 |
| `TargetLockInitializer` | 初始化目标锁定 | 动作 | 否 |
| `VisualServoController` | 视觉伺服控制器 | 动作 | 是 |
| `IBVSController` | 计算IBVS控制 | 动作 | 否 |
| `DistanceQualityChecker` | 检查距离控制质量 | 条件 | 否 |

**实现**：`02_Builder_Approach/custom_nodes/`

#### B. Flow编排模板
**发现**：拒止环境任务复杂，需要预置模板。

**新增模板**：
- "拒止环境区域侦查"（三阶段Flow）
- "VINS导航任务"
- "视觉跟踪任务"
- "GPS防护配置"

**实现**：`02_Builder_Approach/flow_definitions/`

#### C. 人机交互界面
**发现**：Builder需要地面人员介入目标选择。

**需求**：
- 目标选择面板（类似Viewer）
- 简化的确认界面
- 跟踪状态显示
- 距离/高度实时曲线

**实现**：`02_Builder_Approach/web_interface/TargetSelectionPanel.vue`

#### D. 降级模式支持
**发现**：Builder完全运行在边缘，但通信中断时需要降级。

**需求**：
- 通信状态监控
- 自主决策模式
- 任务继续执行
- 恢复后数据同步

**配置**：`phase3_tracking.json`中的`degraded_modes`

### 3. FalconMindSDK 需要完善的功能

#### A. VINS-Fusion封装
**发现**：当前SDK缺少易用的VINS初始化API。

**新增API**：
```cpp
class VINSInitializer {
    bool initialize();  // 一键初始化
    double getProgress() const;
    bool isReady() const;
};
```

**实现**：`03_SDK_Native_Approach/src/navigation/vins_initializer.cpp`

#### B. GPS欺骗检测
**发现**：SDK缺少GPS防护模块。

**新增模块**：
```cpp
class GPSDefender {
    SpoofingReport processGNSS(const GNSSMeasurement& gnss);
    void processIMU(const IMUData& imu);
    GNSSStatus getStatus() const;
};
```

**算法**：
- RAIM一致性检查
- IMU速度一致性验证
- 多源交叉验证

**实现**：`03_SDK_Native_Approach/src/navigation/gps_defender.cpp`

#### C. DeepSORT跟踪器
**发现**：SDK有YOLO但缺少跟踪。

**新增类**：
```cpp
class DeepSORTTracker {
    std::vector<Track> update(const std::vector<TargetDetection>& detections);
    std::optional<Track> getTrackById(int track_id) const;
};
```

**算法**：
- OSNet特征提取
- 级联匹配（马氏+余弦）
- Kalman滤波

**实现**：`03_SDK_Native_Approach/src/perception/deepsort_tracker.cpp`

#### D. IBVS控制器
**发现**：SDK缺少视觉伺服控制。

**新增类**：
```cpp
class IBVSController {
    VelocityCommand computeControl(
        const ImageSpaceTarget& target,
        double current_distance,
        double current_height
    );
};
```

**算法**：
- 图像误差计算
- PID距离控制
- 自适应增益

**实现**：`03_SDK_Native_Approach/src/control/ibvs_controller.cpp`

#### E. 距离估计器
**发现**：单目相机需要距离估计。

**新增类**：
```cpp
class MonocularDistanceEstimator {
    double estimateDistance(const BoundingBox& bbox) const;
};
```

**方法**：
- 已知目标尺寸法
- 深度神经网络
- 双目视差（可选）

**实现**：`03_SDK_Native_Approach/src/perception/distance_estimator.cpp`

### 4. 系统架构问题与改进

#### 问题1：Viewer-Builder功能重叠
**发现**：两者都有人机交互界面，但定位不同。

**解决方案**：
- Viewer：复杂交互、数据分析、集群管理
- Builder：简化交互、现场调试、快速部署
- 统一通信协议，可互相切换

#### 问题2：SDK-Builder集成
**发现**：Builder节点调用SDK能力不够直接。

**改进**：
- Builder节点直接映射SDK API
- 支持C++节点扩展
- 自动生成Python绑定

#### 问题3：任务状态同步
**发现**：Viewer、Builder、SDK各自维护状态。

**改进**：
- 统一状态机定义
- 状态变更事件广播
- 持久化状态存储

### 5. 性能优化建议

#### Viewer方式
- **问题**：通信延迟高
- **优化**：
  - H.265编码降低带宽
  - 关键帧降低频率
  - 本地缓存命令队列

#### Builder方式
- **问题**：解释执行开销
- **优化**：
  - 关键节点编译为C++
  - 背景任务多线程
  - 内存池管理

#### SDK方式
- **问题**：开发效率低
- **优化**：
  - 提供模板代码生成
  - 可视化调试工具
  - 仿真测试框架

## 实施优先级

### P0 (必须)
1. ✅ Builder自定义节点系统
2. ✅ VINS初始化封装
3. ✅ DeepSORT跟踪器
4. ✅ IBVS控制器

### P1 (重要)
1. ✅ Viewer目标选择界面
2. ✅ GPS欺骗检测
3. ✅ Builder Flow模板
4. ✅ 距离估计器

### P2 (优化)
1. 弱网通信机制
2. 降级模式支持
3. 性能分析工具
4. 仿真测试框架

## 代码统计

```
PoC/Scenario_01_DeniedGPS_VisualTracking/
├── 01_Viewer_Approach/          ~1,800行
│   ├── mission_models.py          ~410行
│   ├── services/                  ~1,200行
│   ├── api/mission_api.py         ~400行
│   └── README.md                  ~340行
├── 02_Builder_Approach/         ~1,200行
│   ├── flow_definitions/          ~990行 (JSON)
│   ├── custom_nodes/              ~375行
│   └── README.md                  ~440行
├── 03_SDK_Native_Approach/      ~1,700行
│   ├── include/                   ~964行
│   ├── src/main.cpp               ~307行
│   └── README.md                  ~464行
├── docs/                          (待补充)
└── README.md                      ~330行

总计：~5,000行代码+文档
```

## 后续工作

1. **功能完善**
   - 实现所有P0功能
   - 补充单元测试
   - 集成测试验证

2. **文档完善**
   - API文档
   - 用户手册
   - 部署指南

3. **示例程序**
   - 完整可运行示例
   - SITL仿真测试
   - 实机测试

4. **性能基准**
   - 延迟测试
   - 精度测试
   - 资源占用测试
