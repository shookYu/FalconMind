# PoC Scenario 01: 拒止环境区域侦查与视觉制导跟踪

## 📋 概述

本PoC是**验证四大工程能力的测试场景**，用于检验Builder、Viewer、SDK、NodeAgent是否能支撑**拒止环境视觉跟踪**业务场景。

**核心原则:**
- ✅ **配置驱动**: Builder/Viewer只生成配置，不生成代码
- ✅ **边缘执行**: 所有实时控制都在UAV本地闭环（20Hz）
- ✅ **能力验证**: 通过场景暴露四大工程的能力缺口

---

## 🎯 场景描述

### 任务背景
在GPS拒止（Denied）和GPS欺骗（Spoofing）环境下，无人机执行区域侦查，发现目标后由地面人员选定，自动跟踪目标保持30米水平距离、10米高度进行随动跟踪。

### 任务流程

```
Phase 1: 区域侦查
├── VINS初始化（视觉惯性导航）
├── 起飞到搜索高度
├── GPS欺骗防护启动
├── 沿航点巡逻搜索
└── YOLO实时检测

Phase 2: 目标确认
├── 发现目标后悬停
├── 高清图传回地面
├── 操作员人工选择目标
└── 确认后开始跟踪

Phase 3: 视觉跟踪
├── DeepSORT锁定目标
├── IBVS视觉伺服控制（20Hz）
├── 距离保持30m±2m
├── 高度保持10m±1m
└── 实时随动跟踪

Phase 4: 任务结束
├── 目标丢失/手动取消
└── VINS导航返航
```

### 技术指标

| 指标 | 要求 | 验证方式 |
|------|------|----------|
| 定位精度 | 水平<1m, 高度<0.5m | VINS实测 |
| GPS欺骗检测 | <1s识别 | SDR模拟欺骗 |
| 目标检测 | mAP>0.85 | 实地测试 |
| 跟踪稳定性 | ID保持>95% | 移动目标测试 |
| 距离控制 | 30m±2m | RTK测量 |
| 控制频率 | 20Hz | 示波器测量 |

---

## 📁 目录结构

```
PoC/Scenario_01_DeniedGPS_VisualTracking/
├── README.md                                    # 本文件
├── ARCHITECTURE.md                              # 三种方式架构对比
│
├── 01_Viewer_Approach/                          # Viewer方式
│   └── README.md                                # 配置说明
│
├── 02_Builder_Approach/                         # Builder方式
│   ├── README.md                                # 配置说明
│   └── flow_definitions/                        # Flow配置（核心）
│       ├── phase1_search.json                   # 阶段1: 区域侦查
│       ├── phase2_target_lock.json              # 阶段2: 目标锁定
│       └── phase3_tracking.json                 # 阶段3: 跟踪控制
│
├── 03_SDK_Native_Approach/                      # SDK方式
│   └── README.md                                # 配置说明
│
└── docs/                                        # 文档
    ├── capability_extraction_analysis.md        # 共性能力提炼分析
    ├── capability_verification_checklist.md     # 能力验证清单
    └── requirements_gap_list.md                 # 需求缺口清单
```

**说明:**
- ❌ 删除所有代码文件（services/, api/, src/, custom_nodes/）
- ✅ 只保留配置和文档
- 📋 本PoC是**需求验证工具**，不是完整实现

---

## 🔍 三种方式对比

### 方式一：Viewer方式（Mission配置驱动）

```
操作员 ──▶ Viewer UI ──▶ Mission YAML ──▶ NodeAgent/SDK ──▶ 飞控
         (地图绘制)      (配置)            (边缘执行)        (20Hz闭环)
                    
监控 ◀── 遥测 (5Hz) ◀──────────────────────────────────────────┘

产出物: configs/viewer/denied_env_mission.yaml
特点: 地面站规划，边缘执行，适合集群管理
```

### 方式二：Builder方式（Flow配置驱动）

```
操作员 ──▶ Builder UI ──▶ Flow JSON ──▶ SDK FlowExecutor ──▶ 飞控
         (拖拽编排)        (配置)         (解释执行)         (20Hz闭环)

产出物: configs/builder/*.json
特点: 可视化编排，即时部署，适合现场调试
```

### 方式三：SDK方式（可执行程序）

```
launcher.sh ──▶ Mission YAML ──▶ mission_launcher ──▶ 多进程 ──▶ 飞控
               (配置)             (可执行程序)         (20Hz闭环)

产出物: configs/sdk/*.yaml + launcher.sh
特点: 最高性能，完全定制，适合量产部署
```

### 能力对比

| 维度 | Viewer | Builder | SDK |
|------|--------|---------|-----|
| **产出物** | Mission YAML | Flow JSON | YAML + 可执行程序 |
| **实时控制位置** | UAV边缘 | UAV边缘 | UAV边缘 |
| **Viewer参与** | 任务规划+监控 | 可选监控 | 可选监控 |
| **配置复杂度** | 低（表单） | 中（拖拽） | 高（YAML） |
| **灵活性** | 中 | 中 | 高 |
| **性能** | 高 | 高 | 最高 |

---

## ⚠️ 重要：当前状态

### 四大工程能力缺口

| 工程 | 满足度 | 状态 |
|-----|-------|------|
| **SDK** | 30% | 🔴 不满足 - 缺少8个Flow节点、Mission Executor |
| **Builder** | 70% | 🟡 部分满足 - 可用通用节点，缺少专用模板 |
| **Viewer** | 20% | 🔴 不满足 - 缺少Mission管理API |
| **NodeAgent** | 30% | 🔴 不满足 - 缺少Mission执行能力 |

### 当前可运行性

```
✅ Builder方式: 可以运行（使用通用Flow节点手动搭建）
❌ Viewer方式: 无法运行（缺少Mission管理API）
⚠️  SDK方式: 可以运行（需要手撸代码，缺少配置支持）

当四大工程完成P0能力后，本PoC的配置文件可以直接运行！
```

---

## 📋 需求缺口清单

### P0 - 场景必需（阻塞运行）

**SDK需实现:**
1. `VINSStatusCheck` 节点 - 检查VINS状态
2. `GPSDefenseActivator` 节点 - GPS欺骗检测
3. `VisualDetector` 节点 - YOLO+DeepSORT
4. `VisualServoController` 节点 - IBVS控制
5. `SearchPatternGenerator` 节点 - 航点生成
6. `TargetAwaiter` 节点 - 等待目标选择
7. Mission Executor模块 - 解析执行Mission YAML

**Viewer需实现:**
1. Mission创建API
2. Mission部署API
3. 目标选择API

**NodeAgent需实现:**
1. Mission接收与存储
2. Mission执行（调用SDK）

### 完整需求清单

详见: [docs/requirements_gap_list.md](docs/requirements_gap_list.md)

- 总需求: 30+功能点
- 总工时: ~900h (~22人周)
- 建议工期: 12周（3个月）

---

## 🚀 如何使用本PoC

### 1. 作为需求文档

```bash
# 查看SDK需要实现的能力
cat docs/requirements_gap_list.md | grep "SDK-"

# 查看Viewer需要实现的能力  
cat docs/requirements_gap_list.md | grep "VIEWER-"

# 查看能力验证清单
cat docs/capability_verification_checklist.md
```

### 2. 作为配置模板

```bash
# Builder方式配置
cat 02_Builder_Approach/flow_definitions/*.json

# Viewer方式配置参考
cat 01_Viewer_Approach/README.md | grep -A 100 "Mission配置文件"

# SDK方式配置参考
cat 03_SDK_Native_Approach/README.md | grep -A 100 "Mission配置文件"
```

### 3. 作为验收测试

当四大工程实现P0能力后：

```bash
# 测试Builder方式
cd FalconMindBuilder
docker-compose up -d
# 导入 02_Builder_Approach/flow_definitions/*.json
# 部署并验证执行

# 测试Viewer方式
cd FalconMindViewer
./start-dev.sh
# 创建Mission，使用 01_Viewer_Approach/README.md 中的配置
# 部署到UAV并验证

# 测试SDK方式
cd FalconMindSDK/build
make denied_env_tracking
./launcher.sh ../PoC/configs/sdk/denied_env_mission.yaml
```

---

## 📊 本PoC的价值

### 1. 需求明确化

- 提供了30+明确的功能需求
- 定义了验收标准和测试方法
- 确定了优先级（P0/P1/P2）

### 2. 架构验证

- 验证了"配置驱动"架构的可行性
- 明确了三种方式的边界和协作关系
- 验证了数据流设计的合理性

### 3. 能力缺口暴露

- 暴露了四大工程的能力不足
- 为开发排期提供了输入
- 避免了盲目开发

### 4. 配置模板

- 提供了可直接使用的配置模板
- 当工程能力完善后可直接运行
- 作为其他场景的参考模板

---

## 📚 相关文档

| 文档 | 内容 |
|-----|------|
| [ARCHITECTURE.md](ARCHITECTURE.md) | 三种方式架构详细对比 |
| [01_Viewer_Approach/README.md](01_Viewer_Approach/README.md) | Viewer方式配置说明 |
| [02_Builder_Approach/README.md](02_Builder_Approach/README.md) | Builder方式配置说明 |
| [03_SDK_Native_Approach/README.md](03_SDK_Native_Approach/README.md) | SDK方式配置说明 |
| [docs/capability_extraction_analysis.md](docs/capability_extraction_analysis.md) | 共性能力提炼分析 |
| [docs/capability_verification_checklist.md](docs/capability_verification_checklist.md) | 能力验证清单 |
| [docs/requirements_gap_list.md](docs/requirements_gap_list.md) | 需求缺口清单（开发输入） |

---

## ✅ 验收标准

本PoC完成的标准：

- [x] 分析共性能力，提炼到四大工程
- [x] 删除所有不应存在的代码文件
- [x] 创建完整的配置模板
- [x] 创建需求缺口清单（开发输入）
- [x] 创建能力验证清单（测试输入）
- [x] 三种方式数据流设计完成
- [ ] 四大工程实现P0能力
- [ ] 本PoC配置可直接运行
- [ ] 端到端功能验证通过

---

## 📝 变更记录

| 日期 | 版本 | 变更内容 |
|-----|------|---------|
| 2026-03-04 | v1.0 | 初始版本，包含代码实现 |
| 2026-03-05 | v2.0 | 重构为纯配置+验证结构，删除所有代码 |

---

## 👥 作者

FalconMind Team

**用途:** 本PoC用于验证FalconMind四大工程的业务支撑能力，为开发排期和架构设计提供输入。
