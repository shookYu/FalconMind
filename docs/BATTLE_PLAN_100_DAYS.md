# FalconMind 四大工程百日攻坚行动计划

## 目标

**让拒止环境视觉跟踪场景的三种方式都达到100%功能完整和工程化可用**

- ✅ Builder方式：100% - 预置模板 + 专用UI，一键部署
- ✅ Viewer方式：100% - Mission管理API + 前端组件，集群可用
- ✅ SDK方式：100% - 完整节点库 + Mission Executor，编译通过可运行

**工期：100天（约14周）**
**投入：4名全职工程师**

---

## 阶段划分

```
第1阶段：Foundation（第1-3周）- SDK核心基础
第2阶段：Flow Nodes（第4-7周）- 完整节点库  
第3阶段：Mission System（第8-10周）- Mission执行体系
第4阶段：Integration（第11-12周）- 集成测试
第5阶段：Polish（第13-14周）- 工程化完善
```

---

## 第1阶段：Foundation（第1-3周）

### 目标
建立SDK基础框架，确保编译系统和基础模块可用。

### Week 1-2: SDK基础架构完善

#### 任务1.1: 构建系统优化
- [ ] 完善CMake配置，支持模块化编译
- [ ] 添加可选组件开关（-DFALCONMIND_BUILD_FLOW_NODES=ON）
- [ ] 支持交叉编译（RK3588/RK3576）
- [ ] CI/CD流水线搭建（GitHub Actions）

**验收标准：**
```bash
mkdir build && cd build
cmake .. -DFALCONMIND_BUILD_ALL=ON
make -j4  # 编译成功
ctest     # 基础测试通过
```

#### 任务1.2: 基础模块接口定义
- [ ] 定义FlowNode基类接口
- [ ] 定义MissionConfig数据结构
- [ ] 定义进程间通信接口（ZeroMQ封装）

**关键文件：**
```
include/falconmind/sdk/
├── flow/
│   ├── flow_node.hpp           # 节点基类
│   ├── flow_executor.hpp       # 执行器
│   └── flow_graph.hpp          # 执行图
├── mission/
│   ├── mission_config.hpp      # 配置结构
│   └── mission_types.hpp       # 类型定义
└── ipc/
    ├── zmq_wrapper.hpp         # ZeroMQ封装
    └── shared_memory.hpp       # 共享内存
```

#### 任务1.3: 基础算法模块封装
- [ ] VINS-Fusion接口封装
- [ ] YOLO推理接口封装
- [ ] MAVLink通信接口封装

**产出物：**
- SDK基础框架编译通过
- 基础接口单元测试通过

### Week 3: Builder/Viewer基础准备

#### 任务1.4: Builder工程结构优化
- [ ] Flow模板管理机制
- [ ] 自定义节点注册机制
- [ ] 配置验证系统

#### 任务1.5: Viewer工程结构优化
- [ ] Mission API框架
- [ ] WebSocket遥测系统
- [ ] 前端组件库搭建

**产出物：**
- Builder/Viewer工程目录结构优化
- 基础API框架搭建

### 阶段1里程碑
```
✅ SDK基础框架编译通过
✅ 基础接口定义完成
✅ CI/CD流水线搭建
✅ Builder/Viewer工程结构优化
```

---

## 第2阶段：Flow Nodes（第4-7周）

### 目标
实现所有P0级别的Flow节点，Builder方式达到可用状态。

### Week 4-5: 导航相关节点

#### 任务2.1: VINSStatusCheck节点
```cpp
// include/falconmind/sdk/flow/nodes/vins_status_check_node.hpp
class VINSStatusCheckNode : public FlowNode {
public:
    struct Config {
        double min_confidence = 0.8;
        double timeout_seconds = 5.0;
    };
    
    bool execute() override;
    
private:
    std::shared_ptr<VINSInterface> vins_;
};
```

**实现内容：**
- VINS状态查询接口
- 置信度检查逻辑
- 超时处理

**验收：**
```bash
# 单元测试
./test_vins_status_check_node
# 通过: VINS就绪返回true，未就绪返回false
```

#### 任务2.2: VINSInitializer节点
```cpp
class VINSInitializerNode : public FlowNode {
public:
    struct Config {
        double init_timeout = 30.0;
        int required_features = 150;
    };
    
    bool execute() override;  // 阻塞式初始化
    double getProgress() const;  // 0.0 ~ 1.0
};
```

**实现内容：**
- VINS初始化流程封装
- 进度报告
- 超时和错误处理

#### 任务2.3: GPSDefenseActivator节点
```cpp
class GPSDefenseActivatorNode : public FlowNode {
public:
    struct Config {
        bool raim_check = true;
        bool imu_consistency_check = true;
        double check_interval = 1.0;
    };
    
    bool execute() override;
    void runBackground();  // 后台持续检测
};
```

**实现内容：**
- RAIM算法实现
- IMU一致性验证
- 欺骗检测告警

**验收：**
```bash
# SDR模拟GPS欺骗信号
./test_gps_defense_node
# 通过: 100%检测欺骗信号
```

#### 任务2.4: SearchPatternGenerator节点
```cpp
class SearchPatternGeneratorNode : public FlowNode {
public:
    enum Pattern { LAWN_MOWER, SPIRAL, ZIGZAG };
    
    struct Config {
        Pattern pattern = LAWN_MOWER;
        std::vector<GeodeticPosition> area;
        double altitude = 50.0;
        double overlap_rate = 0.2;
    };
    
    std::vector<Waypoint> generateWaypoints();
};
```

### Week 6: 感知相关节点

#### 任务2.5: VisualDetector节点
```cpp
class VisualDetectorNode : public FlowNode {
public:
    struct Config {
        std::string model = "yolov8n.rknn";
        std::vector<std::string> classes = {"person"};
        float confidence_threshold = 0.6;
        bool enable_tracking = true;
    };
    
    bool execute() override;
    std::vector<Detection> getDetections();
    
private:
    std::shared_ptr<YOLODetector> yolo_;
    std::shared_ptr<DeepSORTTracker> tracker_;
};
```

**实现内容：**
- YOLO检测器封装
- DeepSORT跟踪器集成
- 检测+跟踪一体化

**验收：**
```bash
# 使用测试视频
./test_visual_detector_node
# 通过: 检测率>90%，ID保持>95%
```

#### 任务2.6: TargetDetectionChecker节点
```cpp
class TargetDetectionCheckerNode : public FlowNode {
public:
    struct Config {
        std::vector<std::string> target_classes;
        float min_confidence = 0.7;
        int min_detection_frames = 3;
    };
    
    bool check();  // 返回是否发现目标
    TargetInfo getBestTarget();
};
```

### Week 7: 控制相关节点

#### 任务2.7: VisualServoController节点（核心）
```cpp
class VisualServoControllerNode : public FlowNode {
public:
    struct Config {
        float desired_distance = 30.0;
        float desired_height = 10.0;
        float max_speed = 8.0;
        int control_frequency = 20;  // Hz
        
        struct PIDParams {
            float kp_distance, ki_distance, kd_distance;
            float kp_position, kp_yaw;
        } pid;
    };
    
    bool initialize() override;
    void runBackground() override;  // 20Hz控制循环
    void stop() override;
    
private:
    IBVSController ibvs_controller_;
    std::atomic<bool> running_{false};
    std::thread control_thread_;
};
```

**实现内容：**
- IBVS控制律实现
- 20Hz实时控制循环
- PID参数动态配置
- 安全限幅

**关键算法：**
```cpp
VelocityCommand IBVSController::compute(
    const ImageSpaceTarget& target,
    float current_distance) 
{
    // 图像误差
    float ex = target.u - image_center_u_;  // 水平
    float ey = target.v - image_center_v_;  // 垂直
    float ez = current_distance - desired_distance_;
    
    // IBVS控制律
    float vx = -(pid_.kp_distance * ez + 
                 pid_.ki_distance * integral_error_ +
                 pid_.kd_distance * derivative_error_);
    float vy = -pid_.kp_position * ex * current_distance;
    float vz = -pid_.kp_position * ey * current_distance;
    float yaw_rate = -pid_.kp_yaw * ex;
    
    // 饱和限制
    vx = std::clamp(vx, -max_speed_, max_speed_);
    
    return {vx, vy, vz, yaw_rate};
}
```

#### 任务2.8: TargetAwaiter节点
```cpp
class TargetAwaiterNode : public FlowNode {
public:
    struct Config {
        float timeout_seconds = 300.0;
        std::vector<std::string> target_classes;
    };
    
    bool execute() override;  // 阻塞等待
    int getSelectedTrackId();
};
```

**实现内容：**
- 接收外部指令（目标选择）
- 超时处理
- 目标确认

### 阶段2里程碑
```
✅ 8个P0 Flow节点全部实现
✅ 单元测试全部通过
✅ Builder可以使用这些节点编排Flow
✅ Builder方式达到80%可用度
```

---

## 第3阶段：Mission System（第8-10周）

### 目标
实现Mission执行体系，Viewer和SDK方式达到可用状态。

### Week 8: Mission配置与解析

#### 任务3.1: MissionConfig数据结构
```cpp
// include/falconmind/sdk/mission/mission_config.hpp
struct DeniedEnvMissionConfig {
    struct SearchConfig {
        std::vector<GeodeticPosition> area;
        float altitude = 50.0;
        float speed = 5.0;
        std::string pattern = "LAWN_MOWER";
    } search;
    
    struct TrackingConfig {
        std::string target_class = "person";
        float desired_distance = 30.0;
        float distance_tolerance = 2.0;
        float desired_height = 10.0;
        float height_tolerance = 1.0;
        float max_speed = 8.0;
        int control_frequency = 20;
        
        struct IBVSParams {
            float kp_distance = 0.5;
            float ki_distance = 0.1;
            float kd_distance = 0.2;
        } pid_params;
    } tracking;
    
    struct VINSConfig {
        float init_timeout = 30.0;
        int required_features = 150;
    } vins;
    
    struct GPSDefenseConfig {
        bool enabled = true;
        float check_interval = 1.0;
    } gps_defense;
};
```

#### 任务3.2: Mission YAML解析器
```cpp
class MissionConfigParser {
public:
    static DeniedEnvMissionConfig parseFromYAML(const std::string& yaml_path);
    static bool validate(const DeniedEnvMissionConfig& config);
};
```

**验收：**
```bash
./test_mission_parser ../PoC/configs/viewer/denied_env_mission.yaml
# 通过: 正确解析所有字段
```

### Week 9: Mission执行引擎

#### 任务3.3: MissionExecutor核心
```cpp
class MissionExecutor {
public:
    bool loadMission(const DeniedEnvMissionConfig& config);
    bool start();
    bool pause();
    bool resume();
    bool abort();
    
    MissionState getState() const;
    
private:
    std::unique_ptr<FlowGraph> buildFlowGraph();
    std::shared_ptr<FlowExecutor> flow_executor_;
    DeniedEnvMissionConfig config_;
    std::atomic<MissionState> state_{MissionState::IDLE};
};
```

**实现内容：**
- Mission配置转Flow图
- 状态机管理
- 生命周期控制

#### 任务3.4: Mission到Flow转换器
```cpp
class MissionToFlowConverter {
public:
    FlowGraph convert(const DeniedEnvMissionConfig& mission);
    
private:
    FlowGraph buildSearchPhase(const SearchConfig& config);
    FlowGraph buildTrackingPhase(const TrackingConfig& config);
};
```

**转换逻辑：**
```
Mission YAML
    │
    ▼
SearchConfig ──▶ SearchPatternGenerator + VisualDetector
    │
TrackingConfig ──▶ VisualServoController (20Hz)
    │
    ▼
FlowGraph
    │
    ▼
FlowExecutor执行
```

### Week 10: NodeAgent集成

#### 任务3.5: NodeAgent Mission支持
```cpp
// NodeAgent/src/mission/mission_handler.hpp
class MissionHandler {
public:
    void onMissionReceived(const std::string& mission_yaml);
    void onTargetSelected(int track_id);
    void onAbortCommand();
    
private:
    std::shared_ptr<MissionExecutor> executor_;
    MissionStorage storage_;
};
```

**实现内容：**
- Mission接收（MQTT订阅）
- Mission本地存储
- Mission执行调用
- 执行结果上报

#### 任务3.6: Viewer Mission API
```python
# Viewer/backend/app/api/denied_env_mission.py

@router.post("/missions/denied-env")
async def create_mission(config: DeniedEnvMissionConfig):
    """创建Mission配置"""
    mission_id = await mission_service.create(config)
    return {"mission_id": mission_id}

@router.post("/missions/{mission_id}/deploy")
async def deploy_mission(mission_id: str, uav_id: str):
    """部署Mission到UAV"""
    await mission_service.deploy(mission_id, uav_id)
    return {"status": "deployed"}

@router.post("/missions/{mission_id}/select-target")
async def select_target(mission_id: str, track_id: int):
    """选择跟踪目标"""
    await mission_service.select_target(mission_id, track_id)
    return {"status": "selected"}
```

### 阶段3里程碑
```
✅ Mission YAML解析完成
✅ MissionExecutor可执行
✅ NodeAgent支持Mission接收和执行
✅ Viewer Mission API可用
✅ SDK支持Mission方式运行
```

---

## 第4阶段：Integration（第11-12周）

### 目标
三种方式端到端集成测试，确保可运行。

### Week 11: 端到端集成

#### 任务4.1: Builder端到端测试
```bash
# 1. 启动Builder
cd FalconMindBuilder && docker-compose up -d

# 2. 导入PoC Flow配置
curl -X POST http://localhost:8080/api/flows/import \
  -F "file=@PoC/02_Builder_Approach/flow_definitions/phase1_search.json"

# 3. 创建并部署任务
curl -X POST http://localhost:8080/api/flows/deploy \
  -d '{"flow_id": "denied_env_search_phase", "uav_id": "uav_001"}'

# 4. 验证执行
# - VINS初始化
# - 起飞
# - 搜索
# - 检测到目标
```

**验收标准：**
- Flow部署成功
- 节点执行正常
- 遥测数据正确

#### 任务4.2: Viewer端到端测试
```bash
# 1. 启动Viewer
cd FalconMindViewer && ./start-dev.sh

# 2. 创建Mission
curl -X POST http://localhost:8080/api/v1/missions/denied-env \
  -H "Content-Type: application/json" \
  -d @PoC/01_Viewer_Approach/configs/denied_env_mission.yaml

# 3. 部署到UAV
curl -X POST http://localhost:8080/api/v1/missions/me_001/deploy \
  -d '{"uav_id": "uav_001"}'

# 4. 验证执行
# - Mission下发成功
# - UAV开始执行
# - 遥测上报正常
```

#### 任务4.3: SDK端到端测试
```bash
# 1. 编译SDK方式
cd FalconMindSDK/build
make denied_env_tracking

# 2. 运行测试
./bin/launcher.sh \
  ../PoC/03_SDK_Native_Approach/configs/denied_env_mission.yaml \
  uav_001

# 3. 交互式选择目标
echo "s 5" > /tmp/falconmind_cmd  # 选择track_id=5
echo "c" > /tmp/falconmind_cmd   # 确认选择

# 4. 验证执行
# - 启动正常
# - VINS初始化成功
# - 搜索并检测目标
# - 跟踪控制正常（20Hz）
```

### Week 12: 问题修复与优化

#### 任务4.4: 性能优化
- [ ] IBVS控制频率稳定性优化（确保20Hz）
- [ ] 内存使用优化（避免内存泄漏）
- [ ] CPU占用优化（RK3588上<60%）

#### 任务4.5: 稳定性修复
- [ ] 边界条件处理（目标丢失、通信中断）
- [ ] 错误恢复机制
- [ ] 异常处理完善

### 阶段4里程碑
```
✅ Builder方式端到端测试通过
✅ Viewer方式端到端测试通过
✅ SDK方式端到端测试通过
✅ 性能指标达标（20Hz控制，<100ms延迟）
```

---

## 第5阶段：Polish（第13-14周）

### 目标
工程化完善，达到生产可用标准。

### Week 13: 测试与文档

#### 任务5.1: 完整测试套件
```
tests/
├── unit/                           # 单元测试
│   ├── test_flow_nodes.cpp         # 节点测试
│   ├── test_mission_parser.cpp     # 解析测试
│   └── test_ibvs_controller.cpp    # 控制测试
├── integration/                    # 集成测试
│   ├── test_builder_flow.py        # Builder测试
│   ├── test_viewer_api.py          # Viewer测试
│   └── test_sdk_mission.py         # SDK测试
└── e2e/                           # 端到端测试
    └── test_denied_env_mission.py  # 场景测试
```

**测试覆盖率目标：>80%**

#### 任务5.2: 完善文档
- [ ] API文档（Doxygen/Swagger）
- [ ] 用户手册（三种方式使用指南）
- [ ] 部署文档（Docker/K8s）
- [ ] 故障排查手册

#### 任务5.3: Builder专用UI
- [ ] 拒止环境任务模板
- [ ] 专用参数配置面板
- [ ] 实时跟踪监控

#### 任务5.4: Viewer专用组件
- [ ] 搜索区域地图编辑器
- [ ] 目标选择面板
- [ ] 跟踪状态监控

### Week 14: CI/CD与发布

#### 任务5.5: CI/CD流水线
```yaml
# .github/workflows/release.yml
name: Release

on:
  push:
    tags:
      - 'v*'

jobs:
  build:
    runs-on: ubuntu-20.04
    steps:
      - uses: actions/checkout@v2
      
      - name: Build SDK
        run: |
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release
          make -j4
          
      - name: Run Tests
        run: ctest --output-on-failure
        
      - name: Package
        run: make package
        
      - name: Release
        uses: softprops/action-gh-release@v1
        with:
          files: FalconMind-*.deb
```

#### 任务5.6: 预编译包发布
- [ ] Ubuntu 20.04/22.04 deb包
- [ ] Docker镜像
- [ ] RK3588交叉编译版本

#### 任务5.7: 场景验证最终测试
```bash
# 完整场景验证
./scripts/validate_scenario_01.sh

# 输出:
# ✅ Builder方式: PASS
# ✅ Viewer方式: PASS
# ✅ SDK方式: PASS
# 
# 性能指标:
# - 控制频率: 20Hz ✓
# - 延迟: 79ms ✓
# - 检测率: 92% ✓
# - ID保持: 96% ✓
```

### 阶段5里程碑
```
✅ 测试覆盖率>80%
✅ 文档完整
✅ CI/CD流水线搭建
✅ 预编译包发布
✅ 三种方式100%可用
```

---

## 资源分配

### 人员分工

| 人员 | 主要职责 | 投入阶段 |
|-----|---------|---------|
| **工程师A** | SDK Flow节点实现（导航/控制） | 第1-7周 |
| **工程师B** | SDK感知节点 + Mission系统 | 第1-10周 |
| **工程师C** | Viewer开发（API + 前端） | 第1-10周 |
| **工程师D** | Builder优化 + 测试 + 集成 | 第3-14周 |

### 时间投入

```
总工期: 14周
总人力: 4人 × 14周 = 56人周

第1-3周: 全员投入基础架构
第4-10周: 并行开发（SDK/Viewer/Builder）
第11-14周: 全员投入集成测试
```

---

## 风险管理

### 高风险项

| 风险 | 影响 | 缓解措施 |
|-----|------|---------|
| IBVS控制稳定性 | 场景核心功能 | 第7周完成，预留2周调优 |
| DeepSORT性能 | 跟踪质量 | 使用轻量模型，NPU优化 |
| VINS拒止环境 | 导航精度 | 提前测试，准备降级方案 |
| 多进程通信 | SDK Native稳定性 | 使用成熟IPC方案 |

### 应急预案

```
如果第7周IBVS未达标:
  → 简化控制律（纯PID，去掉图像伺服）
  → 保证基础功能可用，后续版本优化

如果第10周Mission系统未达标:
  → SDK方式优先（硬编码任务）
  → Builder/Viewer后续版本支持
```

---

## 验收标准

### 功能验收

- [ ] **Builder方式**
  - [ ] 可一键创建拒止环境任务
  - [ ] 拖拽编排三阶段Flow
  - [ ] 部署后自动执行
  - [ ] 遥测数据实时显示

- [ ] **Viewer方式**
  - [ ] 可创建Mission配置
  - [ ] 可部署到UAV
  - [ ] 目标选择功能正常
  - [ ] 遥测监控实时更新

- [ ] **SDK方式**
  - [ ] 编译通过无警告
  - [ ] 配置文件驱动执行
  - [ ] 多进程架构稳定
  - [ ] 交互式控制正常

### 性能验收

| 指标 | 目标 | 测试方法 |
|-----|------|---------|
| 控制频率 | 20Hz | 示波器测量 |
| 控制延迟 | <100ms | 端到端测量 |
| 检测帧率 | 20Hz | 日志统计 |
| 检测率 | >90% | 测试视频 |
| ID保持率 | >95% | 移动目标测试 |
| 距离控制 | ±2m | RTK测量 |
| CPU占用 | <60% | top监控 |
| 内存占用 | <512MB | free监控 |

### 工程化验收

- [ ] 代码覆盖率>80%
- [ ] 文档完整度100%
- [ ] CI/CD流水线通过
- [ ] 无P0/P1级别Bug
- [ ] 代码Review通过

---

## 项目时间表

```
Week  1  2  3  4  5  6  7  8  9  10 11 12 13 14
      |--Foundation--|-----Flow Nodes-----|--Mission--|Integration|Polish|

SDK:  [===============================][==========][======][====]
         基础架构    节点实现        Mission    集成   测试

Viewer:[=================][========================][======][====]
         架构准备         API + 前端             集成   测试

Builder:[  ][====================================][======][====]
          准备   模板 + UI优化                 集成   测试

里程碑:  M1        M2              M3              M4      M5
```

---

## 成功指标

```
✅ 三种方式全部100%可用
✅ 编译通过，无警告
✅ 测试覆盖率>80%
✅ 性能指标全部达标
✅ 文档完整
✅ PoC配置文件可直接运行

最终验证:
PoC/Scenario_01_DeniedGPS_VisualTracking/
├── 01_Viewer_Approach/configs/  # Viewer配置
├── 02_Builder_Approach/flow_definitions/  # Builder配置
├── 03_SDK_Native_Approach/configs/  # SDK配置
└── README.md  # 说明如何运行

./scripts/run_all_modes.sh
# ✅ Builder方式运行成功
# ✅ Viewer方式运行成功  
# ✅ SDK方式运行成功
```

---

## 附录：每日站会模板

```
昨日完成:
- 

今日计划:
- 

阻塞/风险:
- 

需要帮助:
- 
```

## 附录：周报模板

```
第X周周报

完成内容:
1. 
2. 

进度状态:
- 整体进度: X% (计划Y%)
- 风险项: 

下周计划:
1. 
2. 

需要决策:
- 
```
