# PoC共性能力提炼分析

## 分析原则

**PoC中不应该存在的代码：**
- ❌ 业务逻辑实现代码
- ❌ 算法实现代码
- ❌ 自定义节点实现（应该在SDK中）
- ❌ Viewer后端API实现（应该在Viewer工程中）

**PoC中应该保留的内容：**
- ✅ 配置文件（Flow JSON、Mission YAML）
- ✅ 文档（架构说明、使用指南）
- ✅ 示例脚本（启动、测试）
- ✅ 需求文档（提炼到四大工程的规范）

---

## 共性能力识别

### 1. SDK（FalconMindSDK）需要添加的能力

#### 1.1 Flow节点库扩展
当前PoC中出现的自定义节点应该成为SDK的标准节点：

```
FalconMindSDK/src/core/flow_nodes/ （新增目录）
├── perception_nodes/          # 感知节点
│   ├── yolo_detector_node.cpp/.hpp
│   ├── deepsort_tracker_node.cpp/.hpp
│   └── distance_estimator_node.cpp/.hpp
├── navigation_nodes/          # 导航节点
│   ├── vins_initializer_node.cpp/.hpp
│   ├── gps_defender_node.cpp/.hpp
│   └── position_fusion_node.cpp/.hpp
├── control_nodes/             # 控制节点
│   ├── visual_servo_node.cpp/.hpp      # IBVS控制
│   ├── position_controller_node.cpp/.hpp
│   └── mavlink_command_node.cpp/.hpp
└── mission_nodes/             # 任务节点
    ├── target_selector_node.cpp/.hpp
    └── search_pattern_node.cpp/.hpp
```

**从PoC提炼的代码：**
- `02_Builder_Approach/custom_nodes/visual_servoing.py` → `visual_servo_node`
- `02_Builder_Approach/custom_nodes/gps_defense.py` → `gps_defender_node`
- `02_Builder_Approach/custom_nodes/vins_initialization.py` → `vins_initializer_node`
- `01_Viewer_Approach/services/visual_tracker.py` → 算法实现提炼到SDK

#### 1.2 Mission Executor（新增模块）
用于Viewer方式的Mission YAML解析和执行：

```
FalconMindSDK/src/mission/ （新增或扩展）
├── mission_executor.cpp/.hpp        # Mission执行器
├── mission_parser.cpp/.hpp          # Mission配置解析
├── mission_config.hpp               # Mission配置结构
└── denied_env_mission.hpp           # 拒止环境任务专用配置
```

**配置示例：**
```cpp
// SDK/include/falconmind/sdk/mission/mission_config.hpp
struct DeniedEnvMissionConfig {
    struct SearchConfig {
        std::vector<GeodeticPosition> area;
        double altitude;
        std::string pattern;  // "LAWN_MOWER", "SPIRAL"
    } search;
    
    struct TrackingConfig {
        double desired_distance;
        double desired_height;
        double max_speed;
        struct IBVSParams {
            double kp_distance, ki_distance, kd_distance;
            double kp_position;
        } pid;
    } tracking;
    
    struct VINSConfig {
        double init_timeout;
        int required_features;
    } vins;
    
    struct GPSDefenseConfig {
        bool enabled;
        double check_interval;
        double raim_threshold;
    } gps_defense;
};
```

#### 1.3 感知模块增强
```
FalconMindSDK/src/perception/ （扩展）
├── yolo/
│   └── ... （已有）
├── tracking/
│   ├── deepsort_tracker.cpp/.hpp    # 需要完善
│   └── target_selector.cpp/.hpp     # 新增
└── estimation/
    └── distance_estimator.cpp/.hpp  # 单目距离估计
```

#### 1.4 控制模块增强
```
FalconMindSDK/src/control/ （扩展）
├── visual_servo/
│   ├── ibvs_controller.cpp/.hpp     # 图像视觉伺服
│   └── pbvs_controller.cpp/.hpp     # 位置视觉伺服（预留）
└── position_controller.cpp/.hpp     # 已有，可能需要扩展
```

#### 1.5 导航模块增强
```
FalconMindSDK/src/navigation/ （扩展）
├── vins/
│   └── vins_initializer.cpp/.hpp    # VINS初始化管理
├── gps_defense/
│   ├── gps_defender.cpp/.hpp        # GPS欺骗检测
│   ├── raim_checker.cpp/.hpp        # RAIM算法
│   └── imu_consistency.cpp/.hpp     # IMU一致性检查
└── search_pattern/
    └── pattern_generator.cpp/.hpp   # 搜索航点生成
```

---

### 2. Builder（FalconMindBuilder）需要添加的能力

#### 2.1 预置Flow模板
```
FalconMindBuilder/backend/app/flows/templates/ （新增）
├── denied_env_search.json           # 拒止环境搜索模板
├── denied_env_tracking.json         # 拒止环境跟踪模板
└── denied_env_full_mission.json     # 完整任务模板
```

**这些模板就是PoC中的flow_definitions！**

#### 2.2 专用UI组件
```
FalconMindBuilder/frontend/src/components/mission/ （新增）
├── DeniedEnvConfigPanel.vue         # 拒止环境配置面板
├── SearchAreaEditor.vue             # 搜索区域编辑器
├── TrackingParamConfig.vue          # 跟踪参数配置
└── TargetSelectionPanel.vue         # 目标选择面板（弱网模式）
```

#### 2.3 配置文件生成器
```
FalconMindBuilder/backend/app/services/
└── mission_config_service.py        # 将UI配置转换为Mission YAML
```

**功能：**
- 将Builder的Flow配置导出为Mission YAML（供Viewer使用）
- 支持Flow配置和Mission配置的双向转换

---

### 3. Viewer（FalconMindViewer）需要添加的能力

#### 3.1 Mission配置管理
```
FalconMindViewer/backend/app/services/
├── mission_config_service.py        # Mission配置解析
├── mission_deployment_service.py    # Mission部署到UAV
└── denied_env_mission_service.py    # 拒止环境任务专用服务
```

#### 3.2 专用API
```
FalconMindViewer/backend/app/api/
└── denied_env_mission.py            # 拒止环境任务API
```

**API设计（只配置，不控制）：**
```python
# POST /api/v1/missions/denied-env
# 创建任务配置，生成Mission YAML

# POST /api/v1/missions/{id}/deploy
# 部署任务到指定UAV（下发Mission YAML）

# POST /api/v1/missions/{id}/select-target
# 人工选择目标（发送目标ID给UAV，UAV本地执行跟踪）

# POST /api/v1/missions/{id}/abort
# 中止任务（发送中止指令）

# GET /api/v1/missions/{id}/telemetry
# 接收遥测（被动接收，不主动控制）
```

#### 3.3 专用前端组件
```
FalconMindViewer/frontend/src/components/missions/
└── denied_env/
    ├── SearchAreaEditor.vue         # 搜索区域编辑
    ├── TrackingParamConfig.vue      # 跟踪参数配置
    ├── TargetSelectionPanel.vue     # 目标选择（弱网支持）
    └── VisualTrackingMonitor.vue    # 视觉跟踪监控
```

---

### 4. NodeAgent 需要添加的能力

#### 4.1 Mission执行支持
```
FalconMindSDK/NodeAgent/src/mission/ （新增）
├── mission_executor.cpp/.hpp        # Mission执行器
├── mission_config_loader.cpp/.hpp   # Mission配置加载
└── denied_env_mission_handler.cpp/.hpp  # 拒止环境任务处理器
```

**说明：**
- NodeAgent接收来自Viewer的Mission YAML
- NodeAgent调用SDK的Mission Executor执行任务
- 离线时NodeAgent可以独立执行预置的Mission

#### 4.2 离线任务存储
```
FalconMindSDK/NodeAgent/src/storage/
└── mission_storage.cpp/.hpp         # 任务配置本地存储
```

**功能：**
- 存储接收到的Mission配置
- 离线时从本地加载执行
- 重连后同步执行结果

---

## PoC重构后结构

重构后的PoC应该只包含：

```
PoC/Scenario_01_DeniedGPS_VisualTracking/
├── README.md                        # 场景说明
├── ARCHITECTURE.md                  # 架构文档（数据流图）
├── configs/                         # 配置文件（核心！）
│   ├── builder/                     # Builder方式配置
│   │   ├── denied_env_search.json   # 阶段1: 搜索Flow
│   │   ├── denied_env_lock.json     # 阶段2: 锁定Flow
│   │   └── denied_env_tracking.json # 阶段3: 跟踪Flow
│   │
│   ├── viewer/                      # Viewer方式配置
│   │   └── denied_env_mission.yaml  # Mission配置
│   │
│   └── sdk/                         # SDK方式配置
│       ├── mission.yaml             # Mission配置
│       ├── ibvs_params.yaml         # IBVS参数
│       └── launcher.sh              # 启动脚本
│
├── docs/                            # 文档
│   ├── data_flow.md                 # 三种方式数据流
│   ├── requirements_to_sdk.md       # SDK需求文档
│   ├── requirements_to_builder.md   # Builder需求文档
│   ├── requirements_to_viewer.md    # Viewer需求文档
│   └── requirements_to_nodeagent.md # NodeAgent需求文档
│
└── validation/                      # 验证测试
    ├── test_scenarios.md            # 测试场景
    └── validation_checklist.md      # 验证清单
```

**关键变化：**
- ❌ 删除所有services/、api/、src/代码目录
- ❌ 删除custom_nodes/（移到SDK）
- ✅ 保留configs/（纯配置）
- ✅ 保留docs/（需求文档）
- ✅ 新增requirements_to_*.md（四大工程需求）

---

## 数据流验证

### Builder方式数据流
```
操作员 ──▶ Builder UI ──▶ configs/builder/*.json ──▶ SDK FlowExecutor
                                                              │
                                                              ▼
                                                    ┌──────────────────┐
                                                    │  飞控 (PX4)      │
                                                    └──────────────────┘
```

### Viewer方式数据流
```
操作员 ──▶ Viewer UI ──▶ configs/viewer/*.yaml ──▶ NodeAgent/SDK MissionExecutor
                                                             │
                                                             ▼
                                                    ┌──────────────────┐
                                                    │  飞控 (PX4)      │
                                                    └──────────────────┘
                                                              │
                                                              ▼
                                                    ┌──────────────────┐
                                                    │  遥测 (5Hz)      │
                                                    └──────────────────┘
                                                              │
                                                              ▼
                                                           Viewer
```

### SDK方式数据流
```
launcher.sh ──▶ configs/sdk/*.yaml ──▶ denied_env_tracking (可执行)
                                                │
                                                ├──▶ perception进程
                                                ├──▶ control进程
                                                └──▶ navigation进程
                                                              │
                                                              ▼
                                                    ┌──────────────────┐
                                                    │  飞控 (PX4)      │
                                                    └──────────────────┘
```

所有方式的核心执行逻辑都在SDK中，PoC只提供配置！
