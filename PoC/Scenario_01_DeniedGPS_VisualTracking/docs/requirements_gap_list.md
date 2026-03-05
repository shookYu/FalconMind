# 四大工程需求缺口清单

## 说明

本文档基于**拒止环境区域侦查与视觉制导跟踪**场景，分析四大工程需要补充的能力。

**文档用途：**
1. 明确各工程需要开发的功能
2. 作为开发排期的输入
3. 作为验收测试的依据

---

## 1. FalconMindSDK 需求

### 1.1 Flow节点库（高优先级）

#### P0 - 场景必需节点

| ID | 节点类型 | 功能描述 | 验收标准 | 估算工时 |
|----|---------|---------|---------|---------|
| SDK-FLOW-01 | `VINSStatusCheck` | 检查VINS初始化状态 | 能检测VINS就绪状态，返回置信度 | 16h |
| SDK-FLOW-02 | `VINSInitializer` | VINS初始化流程 | 完成初始化流程<30s，支持配置参数 | 24h |
| SDK-FLOW-03 | `GPSDefenseActivator` | GPS欺骗检测激活 | 启动检测线程，1Hz检查频率 | 16h |
| SDK-FLOW-04 | `VisualDetector` | YOLO+DeepSORT检测 | 输出检测结果，支持跟踪ID | 32h |
| SDK-FLOW-05 | `VisualServoController` | IBVS视觉伺服 | 20Hz控制循环，PID参数可配置 | 40h |
| SDK-FLOW-06 | `SearchPatternGenerator` | 搜索航点生成 | 支持LAWN_MOWER/SPIRAL/ZIGZAG | 16h |
| SDK-FLOW-07 | `TargetDetectionChecker` | 目标发现检查 | 按类别和置信度过滤目标 | 8h |
| SDK-FLOW-08 | `TargetAwaiter` | 等待目标选择 | 接收外部指令，超时处理 | 8h |

**依赖模块：**
- VINS初始化管理模块（封装现有VINS-Fusion）
- GPS欺骗检测模块（RAIM + IMU一致性）
- DeepSORT跟踪（完善现有实现）
- IBVS控制器（新实现）

#### P1 - 增强体验节点

| ID | 节点类型 | 功能描述 | 验收标准 | 估算工时 |
|----|---------|---------|---------|---------|
| SDK-FLOW-09 | `TrackingConfigurator` | 跟踪参数配置 | 动态修改IBVS参数 | 8h |
| SDK-FLOW-10 | `TrackingMonitor` | 跟踪状态监控 | 检测丢失/超时/异常 | 8h |
| SDK-FLOW-11 | `EndConditionChecker` | 结束条件检查 | 可配置多种结束条件 | 8h |
| SDK-FLOW-12 | `DistanceEstimator` | 单目距离估计 | 基于目标尺寸估计距离 | 16h |

### 1.2 Mission Executor模块（Viewer方式必需）

| ID | 模块 | 功能描述 | 验收标准 | 估算工时 |
|----|-----|---------|---------|---------|
| SDK-MISSION-01 | MissionConfigParser | 解析Mission YAML | 完整解析denied_env_mission.yaml | 16h |
| SDK-MISSION-02 | MissionExecutor | Mission执行引擎 | 转换为内部执行计划并执行 | 32h |
| SDK-MISSION-03 | MissionStateMachine | Mission状态管理 | 状态转换正确，可查询 | 16h |
| SDK-MISSION-04 | MissionToFlowConverter | Mission转Flow | Mission正确转换为Flow JSON | 24h |

### 1.3 感知模块增强

| ID | 功能 | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| SDK-PER-01 | DeepSORT完善 | 完善现有跟踪实现 | ID保持率>95%，处理遮挡 | 40h |
| SDK-PER-02 | OSNet特征提取 | 128维外观特征 | 提取时间<10ms | 24h |
| SDK-PER-03 | 单目距离估计 | 基于已知尺寸估计距离 | 误差<10% @ 30m | 16h |

### 1.4 控制模块增强

| ID | 功能 | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| SDK-CTRL-01 | IBVS控制器 | 图像视觉伺服 | 20Hz，距离控制±2m | 40h |
| SDK-CTRL-02 | PID参数动态配置 | 运行时修改PID | 参数热更新 | 8h |

### 1.5 导航模块增强

| ID | 功能 | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| SDK-NAV-01 | VINS初始化管理 | 封装初始化流程 | <30s完成，可查询进度 | 24h |
| SDK-NAV-02 | RAIM检查 | GNSS一致性检查 | 欺骗检测率100% | 24h |
| SDK-NAV-03 | IMU一致性验证 | 速度差检测 | 异常检测灵敏度<3m/s | 16h |
| SDK-NAV-04 | 搜索航点生成 | 多种搜索模式 | 支持3种模式，航点连续 | 16h |

**SDK总估算工时:** ~450h (~11人周)

---

## 2. FalconMindBuilder 需求

### 2.1 预置Flow模板

| ID | 模板 | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| BUILDER-TPL-01 | 拒止环境搜索模板 | phase1_search.json模板 | 可一键创建，参数可配置 | 8h |
| BUILDER-TPL-02 | 拒止环境跟踪模板 | phase3_tracking.json模板 | 可一键创建，参数可配置 | 8h |
| BUILDER-TPL-03 | 完整拒止环境任务 | 三阶段完整Flow | 包含所有三个阶段 | 16h |

### 2.2 专用UI组件

| ID | 组件 | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| BUILDER-UI-01 | 搜索区域编辑器 | 地图绘制搜索区域 | 支持多边形绘制，坐标导出 | 16h |
| BUILDER-UI-02 | IBVS参数配置面板 | PID参数配置 | 滑块/输入框，实时验证 | 8h |
| BUILDER-UI-03 | 目标选择面板 | 显示检测目标 | 弱网模式，目标卡片 | 16h |
| BUILDER-UI-04 | 跟踪监控预览 | 实时跟踪显示 | 距离/高度曲线 | 16h |

### 2.3 配置导出功能

| ID | 功能 | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| BUILDER-EXP-01 | Mission YAML导出 | Flow转Mission配置 | 生成的YAML可被Viewer使用 | 16h |
| BUILDER-EXP-02 | 配置验证 | 验证配置完整性 | 错误提示和修复建议 | 8h |

**Builder总估算工时:** ~112h (~3人周)

---

## 3. FalconMindViewer 需求

### 3.1 Mission管理API

| ID | API | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| VIEWER-API-01 | `POST /missions/denied-env` | 创建Mission | 正确生成Mission YAML | 16h |
| VIEWER-API-02 | `POST /missions/{id}/deploy` | 部署Mission | 通过MQTT/HTTP下发到UAV | 16h |
| VIEWER-API-03 | `POST /missions/{id}/select-target` | 选择目标 | 发送目标ID到UAV | 8h |
| VIEWER-API-04 | `POST /missions/{id}/abort` | 中止任务 | 紧急中止指令下发 | 8h |
| VIEWER-API-05 | `GET /missions/{id}/telemetry` | 遥测流 | WebSocket 5Hz遥测 | 16h |

### 3.2 专用前端组件

| ID | 组件 | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| VIEWER-COMP-01 | 搜索区域编辑器 | 地图绘制 | 同Builder组件，可复用 | 8h |
| VIEWER-COMP-02 | 目标选择面板 | 目标列表 | 显示目标，支持选择确认 | 24h |
| VIEWER-COMP-03 | 跟踪监控面板 | 实时状态 | 距离/高度/质量显示 | 24h |
| VIEWER-COMP-04 | GPS状态指示器 | 欺骗状态 | 颜色/图标显示状态 | 8h |
| VIEWER-COMP-05 | Mission配置表单 | 参数配置 | 完整的Mission配置UI | 32h |

### 3.3 数据服务

| ID | 服务 | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| VIEWER-SVC-01 | MissionConfigService | Mission配置管理 | CRUD操作，验证 | 16h |
| VIEWER-SVC-02 | MissionDeploymentService | 部署服务 | MQTT/HTTP下发 | 16h |
| VIEWER-SVC-03 | TelemetryService | 遥测数据处理 | 解析存储，实时推送 | 24h |

**Viewer总估算工时:** ~216h (~5人周)

---

## 4. NodeAgent 需求

### 4.1 Mission执行支持

| ID | 功能 | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| NODE-MIS-01 | Mission接收 | 接收Viewer下发的Mission | MQTT订阅，存储到SQLite | 16h |
| NODE-MIS-02 | Mission存储 | 本地持久化 | SQLite表结构，查询接口 | 8h |
| NODE-MIS-03 | Mission执行 | 调用SDK MissionExecutor | 启动执行，状态上报 | 24h |
| NODE-MIS-04 | 离线Mission支持 | 预置Mission执行 | 离线时从本地加载 | 16h |
| NODE-MIS-05 | 执行结果上报 | 任务完成上报 | 断线重连后同步 | 16h |

### 4.2 人机交互指令

| ID | 功能 | 描述 | 验收标准 | 估算工时 |
|----|-----|------|---------|---------|
| NODE-CMD-01 | 目标选择指令 | 处理选择目标 | 转发到执行进程 | 8h |
| NODE-CMD-02 | 中止指令 | 紧急中止 | 立即生效 | 8h |
| NODE-CMD-03 | 暂停/恢复 | 任务控制 | 状态保持 | 8h |

**NodeAgent总估算工时:** ~104h (~2.5人周)

---

## 需求汇总

### 按工程汇总

| 工程 | P0需求 | P1需求 | 总工时 | 人周 |
|-----|-------|-------|-------|------|
| **SDK** | 8节点+Mission | 4节点+增强 | 450h | 11 |
| **Builder** | 模板+基础UI | 增强UI+导出 | 112h | 3 |
| **Viewer** | Mission API+组件 | 增强服务 | 216h | 5 |
| **NodeAgent** | Mission执行 | 扩展指令 | 104h | 2.5 |
| **总计** | - | - | **882h** | **22** |

### 按优先级汇总

**P0 - 场景必需（阻塞运行）:**
- SDK: 8个Flow节点 + Mission Executor
- Viewer: Mission管理API + 基础组件
- NodeAgent: Mission执行

**P1 - 增强体验:**
- SDK: 4个Flow节点 + 距离估计
- Builder: 专用模板和UI
- Viewer: 监控面板增强

### 建议开发顺序

```
第一阶段（4周）- SDK核心能力:
  Week 1-2: SDK-FLOW-01~04 (VINS/GPS/检测节点)
  Week 3-4: SDK-FLOW-05 (IBVS控制器)

第二阶段（3周）- SDK Mission + NodeAgent:
  Week 5-6: SDK-MISSION-01~04 (Mission Executor)
  Week 7: NODE-MIS-01~05 (NodeAgent Mission支持)

第三阶段（3周）- Viewer:
  Week 8-9: VIEWER-API-01~05 (Mission API)
  Week 10: VIEWER-COMP-01~05 (前端组件)

第四阶段（2周）- Builder增强:
  Week 11-12: BUILDER-TPL-01~03 + BUILDER-UI-01~04

总工期: 12周 (~3个月)
```

---

## 验收测试清单

### SDK验收测试

- [ ] Flow节点库: 所有P0节点可加载执行
- [ ] Mission Executor: 完整执行denied_env_mission.yaml
- [ ] VINS初始化: <30s完成，定位误差<1m
- [ ] GPS欺骗检测: 100%检测人工注入欺骗
- [ ] YOLO+DeepSORT: 检测率>90%，ID保持>95%
- [ ] IBVS控制: 20Hz频率，距离控制±2m

### Builder验收测试

- [ ] 模板: 可一键创建拒止环境任务
- [ ] Flow编排: 拖拽节点完成三阶段编排
- [ ] 部署: Flow JSON成功下发到UAV
- [ ] 弱网: 断网后任务继续执行

### Viewer验收测试

- [ ] Mission创建: UI生成正确YAML
- [ ] Mission部署: 成功下发到UAV
- [ ] 目标选择: 人工选择成功转发
- [ ] 遥测接收: 5Hz实时显示
- [ ] 监控面板: 距离/高度实时显示

### NodeAgent验收测试

- [ ] Mission接收: 正确接收存储
- [ ] Mission执行: 调用SDK成功
- [ ] 离线执行: 断网后可继续
- [ ] 结果上报: 重连后同步

---

## 备注

1. **工时估算基于:**
   - 熟练C++/Python开发者
   - 已有SDK基础框架
   - 包含单元测试时间
   - 不包含文档和集成测试

2. **风险项:**
   - IBVS控制器实现复杂度较高
   - VINS在拒止环境下的稳定性
   - DeepSORT在RK3588上的性能

3. **可复用组件:**
   - Builder和Viewer的地图编辑器可复用
   - 目标选择面板可复用
   - Mission配置解析逻辑可复用
