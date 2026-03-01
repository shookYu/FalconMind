# Viewer 未完成优化与整体工程待实现项

> **基于文档**: `VIEWER_OPTIMIZATION_RECOMMENDATIONS.md`、`OPTIMIZATION_PROGRESS.md`、工程现状  
> **更新日期**: 2025-02-06

本文档整理了两部分内容：  
**一、优化建议文档中尚未完成或未完全落地的优化项**  
**二、整体工程（Viewer + 项目需求）中尚未实现的功能。**

---

## 一、优化建议文档中尚未完成的优化

以下对照 `VIEWER_OPTIMIZATION_RECOMMENDATIONS.md` 逐条标注状态，**未完成**的单独列出。

### 1. 架构与代码组织

| 建议 | 状态 | 说明 |
|------|------|------|
| 1.1 后端模块化（models/services/routers/utils） | ✅ 已做 | 已有 models、services、routers、utils、config.py |
| 1.2 前端模块化（components/services/stores） | 🔶 部分 | 有 services、utils、components（部分为 .js 非 .vue）；未用 Pinia/stores 做集中状态 |
| 1.3 配置管理（pydantic-settings + 前端 config.js） | ✅ 已做 | 后端 config、前端 config.js 已有 |

**未完成：**

- **前端组件化与状态管理**：未按建议拆成 Vue 单文件组件（如 UavList.vue、MissionList.vue、LocationSelector.vue 等），未引入 Pinia 或等效的集中状态管理。
- **Cesium 独立服务**：Cesium 初始化与相机逻辑仍在 `cesium-manager.js`，未单独拆成 `services/cesium.js` 的轻量入口。

---

### 2. 性能优化

| 建议 | 状态 | 说明 |
|------|------|------|
| 2.1.1 WebSocket 广播队列、非阻塞 | ✅ 已做 | 见 OPTIMIZATION_PROGRESS |
| 2.1.2 数据变化检测再广播 | ✅ 已做 | telemetry_service 变化检测 |
| 2.1.3 连接数限制与心跳 | ✅ 已做 | websocket_manager |
| 2.2.1 相机节流、轨迹点数限制、实体批处理 | ✅ 已做 | 节流、memory-manager、entity-batcher |
| 2.2.2 不活跃 UAV 清理、检测数量限制 | ✅ 已做 | memory-manager |
| 2.2.3 瓦片缓存、preload、RequestScheduler、错误重试 | 🔶 未完全 | 文档中的 tileCacheSize/preloadSiblings/error 监听/RequestScheduler 未在代码中系统实现 |

**未完成：**

- **地图瓦片加载优化**：统一配置 `tileCacheSize`、`preloadSiblings`、`preloadAncestors`；为 imageryProvider 增加 error 监听与重试；按文档配置 `RequestScheduler.maximumRequests`（若 Cesium 版本支持）。

---

### 3. 错误处理与健壮性

| 建议 | 状态 | 说明 |
|------|------|------|
| 3.1 后端全局异常处理、请求验证 | ✅ 已做 | 全局 handler、验证 |
| 3.2 前端 WebSocket 重连（指数退避、最大重试）、事件化 | ✅ 已做 | websocket.js |
| 3.3 Pydantic 数据验证（坐标范围、时间戳合理性） | ✅ 已做 | models 中已有验证 |

**未完成：**

- **前端**：Cesium 初始化失败时的友好提示（如 Toast + 降级说明）可再加强。
- **后端**：若需严格限流，文档中的 **slowapi 限流**（如 `/ingress/telemetry` 100 次/分钟）尚未实现。

---

### 4. 用户体验

| 建议 | 状态 | 说明 |
|------|------|------|
| 4.1 加载状态、Toast、键盘快捷键、视图保存 | ✅ 已做 | 加载指示、toast、keyboard-shortcuts、view-manager |
| 4.2 响应式（移动/平板 media query） | ✅ 已做 | styles.css 等 |
| 4.3 可访问性（ARIA、键盘导航、高对比度、屏幕阅读器） | ❌ 未做 | 文档建议均未系统实现 |

**未完成：**

- **可访问性**：ARIA 标签、完整键盘导航、高对比度模式、屏幕阅读器支持。
- **交互**：文档中的「撤销/重做相机位置」未实现；中键拖拽已按相机需求禁用或自定义，若有其它中键语义可再明确。

---

### 5. 可维护性与扩展性

| 建议 | 状态 | 说明 |
|------|------|------|
| 5.1 日志系统（RotatingFileHandler、等级、文件） | ✅ 已做 | utils/logging.py |
| 5.2 单元测试（telemetry_service、models） | ✅ 已做 | tests/ 下已有；WebSocket/路由/集成测试未完全 |
| 5.3 API 文档（OpenAPI、说明与示例） | 🔶 部分 | FastAPI 自带 OpenAPI；详细说明与 Postman 集合未单独维护 |

**未完成：**

- **测试扩展**：WebSocket 管理器单测、路由层单测、集成测试（见 OPTIMIZATION_PROGRESS 待实施）。
- **API 文档**：接口说明、示例与 Postman/Insomnia 集合的维护与发布。

---

### 6. 安全性

| 建议 | 状态 | 说明 |
|------|------|------|
| 6.1 认证与授权（JWT、verify_token、权限） | ❌ 未做 | 文档中的 auth 与权限未实现 |
| 6.2 输入验证与限流（slowapi） | ❌ 未做 | 限流未实现 |
| 6.3 CORS 按环境/配置限制来源 | 🔶 部分 | 若生产需从 config 读取 allow_origins 并收紧 |

**未完成：**

- **认证与授权**：JWT 校验、角色与权限（如 ingest_telemetry、CONTROL_UAV 等）。
- **限流**：对关键接口（如遥测接入）做按 IP/用户的限流。
- **CORS**：生产环境使用配置化的 `allowed_origins`，避免 `*`。

---

### 7. 数据持久化与历史

| 建议 | 状态 | 说明 |
|------|------|------|
| 7.1 数据库集成（SQLite、TelemetryRecord） | ✅ 已做 | services/database.py、历史存储 |
| 7.2 历史查询接口（/telemetry/history） | ✅ 已做 | routers/history.py、前端数据查询面板 |

当前已实现；若需扩展，可增加按时间范围/聚合的统计接口。

---

### 8. 功能增强（第 8 节：无人机集群态势与指控）

文档 8.1～8.10 中的功能多为**未实现或仅预留**，按优先级归纳如下。

#### 🔴 高优先级（文档建议先做，当前均未实现）

- **8.1.1 可视化任务规划器**：地图上绘制/编辑任务路径、多种任务类型、实时预览；后端 `/missions/plan`、`/missions/validate`。
- **8.1.2 任务模板管理**：模板 CRUD、基于模板创建任务、参数化配置。
- **8.2.1 集群队形可视化**：队形类型（线形、V、菱形等）、队形线与信息展示。
- **8.2.2 集群状态总览面板**：在线数、任务数、健康度、集群列表（如 ClusterOverview.vue 描述）。
- **8.3.1 告警中心**：告警列表、分级、确认/处理；后端 Alert 模型与 `/alerts`、`/alerts/{id}/acknowledge`。
- **8.3.2 事件日志系统**：关键事件记录、搜索/过滤、导出；后端 EventLog、`/events`、`/events/export`。
- **8.4.1 飞行控制接口**：起飞/降落/悬停/飞往点/紧急停止/模式切换等；后端 `/uavs/{id}/commands/*`。
- **8.4.2 飞行控制面板 UI**：快速命令、位置/速度控制（如 FlightControlPanel.vue）。

#### 🟡 中优先级（文档建议其次，当前均未实现）

- **8.2.3 多机协同路径显示**：协同路径、冲突区域、覆盖热力图（CooperativePathVisualizer）。
- **8.5.1 高级历史回放**：多 UAV 同步、速度/时间轴、历史告警与事件叠加（PlaybackService 增强）。
- **8.5.2 统计分析**：飞行时间/距离/高度/速度、任务完成率、电池与通信质量曲线；后端 `/statistics/uav|cluster|mission`，前端 StatisticsPanel。
- **8.6.1 通信链路状态监控**：链路状态、质量可视化、拓扑图；后端 `/communication/status`、`/communication/topology`。
- **8.6.2 数据链质量监控**：质量历史、异常告警（DatalinkMonitor）。
- **8.7.1 禁飞区管理**：禁飞区显示/编辑、冲突检测；后端 NoFlyZone、`/no-fly-zones`、`/missions/check-no-fly-zone`，前端 NoFlyZoneManager。

#### 🟢 低优先级（文档建议长期，当前均未实现）

- **8.7.2 地形分析**：地形高度、剖面、障碍检测（TerrainAnalysis）。
- **8.7.3 气象信息叠加**：风速风向、云层、能见度；后端 `/weather/current`。
- **8.8.1 用户权限管理**：角色（Admin/Operator/Observer）、权限枚举、用户 CRUD 与权限更新。
- **8.8.2 系统配置管理**：系统参数、地图源、告警阈值等；后端 `/config`。
- **8.9 移动端支持**：更完整的响应式、触摸手势、简化 UI（文档中的移动端样式与 touch-action）。

---

### 9. 场景驱动设计（第 9 节：基于 SDK 与 20 个场景）

文档 9.1～9.7 为场景模板、Pipeline 可视化、场景切换与对比、回放、特殊环境、报告等，**当前均未实现**。

#### 🔴 高优先级（文档建议先做）

- **9.1.1 场景模板系统**：20 个测试场景抽象为 ScenarioTemplate、分类、Pipeline/节点配置；后端 `/scenario-templates`、`/scenarios/from-template`。
- **9.1.2 场景配置向导**：分步向导、动态表单项、地图绘制区域、预览（ScenarioWizard.vue）。
- **9.2.1 Pipeline 可视化编辑器**：节点图、拖拽、连线、节点属性面板、节点库（PipelineVisualizer.vue）。

#### 🟡 中优先级

- **9.2.2 节点状态实时监控**：PipelineMonitor、节点状态与数据流订阅。
- **9.3.1 场景快速切换面板**：运行中场景列表、暂停/停止、快速启动（ScenarioSwitcher.vue）。
- **9.3.2 场景对比视图**：双场景选择、指标对比表与图表（ScenarioComparison.vue）。
- **9.4.1 多场景同步回放**：MultiScenarioPlayback、统一时间轴与多场景状态更新。
- **9.4.2 场景性能分析**：`/scenarios/{id}/analysis`、`/scenarios/compare`。

#### 🟢 低优先级

- **9.5 特殊环境场景模板**：GPS 拒止/黑夜/室内等适配配置与自动切换节点；后端 `/scenarios/special-environment`，前端 SpecialEnvironmentConfig.vue。
- **9.6 场景报告生成**：`/scenarios/{id}/report`（pdf/html/json）。

---

### 10. 实施优先级（文档最后章节）

文档「高优先级」中的错误处理、WebSocket 重连、数据验证、日志已基本完成。  
**仍未完全落地的：**

- 限流（与 6.2 重复）。
- 单元测试扩展（WebSocket、路由、集成）。
- 配置管理已做；若需按环境（dev/prod）切换，可再检查 env 与配置项是否完整。

---

## 二、整体工程尚未实现的功能（含项目需求）

结合 `PROJECT_REQUIREMENTS_DOCUMENT.md` 与当前代码库，以下为**整体工程层面**尚未实现或未完全对齐需求的部分。

### 2.1 Viewer 与 Cluster Center 的集成

- **任务下发与执行闭环**：Viewer 侧任务规划/编辑、下发到 Cluster Center、执行状态回显与中止，端到端流程未完全打通（依赖 8.1、8.4 的实现）。
- **集群与多机协同在 Viewer 的展示**：集群列表、队形、协同路径、通信拓扑等（对应文档 8.2、8.6）。

### 2.2 Builder 与 Viewer 的联动

- **Builder 生成的任务/流程在 Viewer 中的加载与展示**：若需求中存在「从 Builder 导出任务并在 Viewer 中打开」，该链路未实现。
- **Pipeline 配置在 Viewer 中的可视化与监控**：对应文档 9.2 Pipeline 可视化与节点状态监控。

### 2.3 SDK / NodeAgent / Cluster Center 对接

- **Viewer 与 Cluster Center 的 API 对齐**：飞行控制、任务、告警、事件等接口需与 Cluster Center 实际 API 一致并对接。
- **实时告警与事件来源**：告警中心、事件日志需有后端或 Cluster Center 的数据源与推送机制。

### 2.4 测试与质量

- **端到端测试**：从任务创建 → 下发 → 执行 → 回放的自动化 E2E 测试未在文档或仓库中体现为稳定套件。
- **性能基准与回归**：Cesium 渲染、大量 UAV/实体下的帧率与内存未做系统化基准与回归。

### 2.5 部署与运维

- **生产部署说明**：生产环境的反向代理、HTTPS、环境变量、CORS、认证等部署清单未在 Viewer 文档中集中写明。
- **健康检查与监控**：后端与前端健康检查接口、与监控系统的集成（若有需求）未在优化建议中实现。

---

## 三、汇总表：建议优先落地的项

| 类别 | 未完成项 | 建议优先级 |
|------|----------|------------|
| 优化建议-性能 | 瓦片缓存与 RequestScheduler、imagery 错误重试 | 中 |
| 优化建议-体验 | 可访问性（ARIA、键盘、高对比度）；相机撤销/重做（可选） | 低 |
| 优化建议-测试 | WebSocket/路由/集成测试 | 中 |
| 优化建议-安全 | 认证授权、限流、CORS 收紧 | 按需（多用户/公网时高） |
| 功能增强 | 任务规划与编辑、任务模板、告警中心、飞行控制、集群态势 | 高 |
| 功能增强 | 高级回放、统计分析、通信监控、禁飞区 | 中 |
| 功能增强 | 地形/气象、权限与系统配置、移动端完善 | 低 |
| 场景驱动 | 场景模板、配置向导、Pipeline 可视化 | 高 |
| 场景驱动 | 场景切换与对比、多场景回放、场景分析、报告与特殊环境 | 中/低 |
| 工程整体 | Viewer–Cluster Center 闭环、Builder–Viewer 联动、E2E 测试、生产部署文档 | 高/中 |

---

## 四、与 OPTIMIZATION_PROGRESS.md 的对应关系

- **OPTIMIZATION_PROGRESS.md 中「待实施」**：单元测试扩展、Cesium 进一步优化、Toast/加载/错误提示——其中 Toast 与加载已实现；其余（WS/路由/集成测试、Cesium 瓦片与调度优化）已纳入上文。
- **OPTIMIZATION_PROGRESS.md 中「功能增强」**：任务规划、告警中心、飞行控制、集群态势、场景模板——与本文「一、8」「一、9」及「三、汇总表」一致，均为高优先级待实现。

建议后续在推进优化或需求迭代时，直接以本文档和 `VIEWER_OPTIMIZATION_RECOMMENDATIONS.md` 为检查清单，并定期同步更新 `OPTIMIZATION_PROGRESS.md`。
