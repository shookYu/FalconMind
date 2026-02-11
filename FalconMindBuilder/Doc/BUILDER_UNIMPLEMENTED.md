# FalconMindBuilder 未实现功能清单

> **说明**：Builder 已实现最小可用版（工程/流程 CRUD、拖拽连线、代码生成 main.cpp + CMakeLists.txt、Flow 导出与版本管理）。本文档对照设计文档（05_FalconMindBuilder_Design.md）、需求文档与优化建议，列出**尚未实现**的功能。  
> **更新日期**：2025-02-06

---

## 1. 工程与流程管理

| 功能 | 现状 | 待实现 |
|------|------|--------|
| 工程更新 | 无 `PUT /projects/{project_id}` | 支持修改工程名称、描述等元数据 |
| 工程删除 | 无 `DELETE /projects/{project_id}` | 删除工程及其下所有流程，并清理持久化 |
| 流程克隆 | 无 `POST /projects/{project_id}/flows/{flow_id}/clone` | 复制流程为新 flow_id，便于在现有流程上改一版 |
| 流程打开/导入 | 仅有「从当前工程选流程」 | 从文件或 JSON 导入流程定义（打开本地/导出文件） |
| 流程验证 | 未在保存或生成前做结构化校验 | 校验节点连接合法性（端口类型匹配）、必填参数、环路检测等，并返回错误列表 |

---

## 2. 前端编辑器增强

| 功能 | 现状 | 待实现 |
|------|------|--------|
| 节点对齐 | 无 | 多选节点后对齐（左/右/上/下/水平居中/垂直居中） |
| 节点分组 | 无 | 将若干节点归为一组，可折叠/展开或整体移动 |
| 画布注释 | 无 | 在画布上添加文字/形状注释，便于说明流程 |
| 全局变量 | FlowDefinition 无 `globals` 字段 | 定义流程级变量，并在节点参数中绑定（设计文档 3.2 IR） |
| 多选与批量操作 | 部分存在 | 框选多节点、批量删除、批量移动、批量对齐 |
| 连接线选中删除 | 已支持双击删除边 | 可选：单击选中边再按 Delete，或边上的删除按钮 |

*说明：节点删除、边删除、撤销（Undo）已在当前前端实现。*

---

## 3. 节点库与模板

| 功能 | 现状 | 待实现 |
|------|------|--------|
| 模板来源 | 仅后端硬编码 8 个模板 | **从 SDK 导入**：解析 SDK 接口描述（Proto/IDL/JSON），自动生成 NodeTemplate，与设计中的 `NodeTemplateService.importFromSdk` 对应 |
| 自定义模板 | 无 | `POST /templates` 创建、`DELETE /templates/{id}` 删除；前端「保存为模板」将当前节点配置存为可复用模板 |
| 模板分类扩展 | 现有 FLIGHT/SENSORS/PERCEPTION/MISSION 等 | 设计中的更多类型：逻辑节点（条件/分支/循环）、集群节点（任务分配/多机同步）、环境适应节点（GPS 拒止/低照度）、SLAM、图像预处理等，需在 SDK 有对应节点或占位后再在 Builder 增加模板 |
| 节点类型覆盖 | 8 个模板（flight_state_source, flight_command_sink, camera_source, dummy_detection, tracking, search_path_planner, event_reporter） | 设计 3.1 中的大量类型未覆盖：激光雷达/IMU/GPS/UWB 源、图像预处理/低照度、SLAM、多源融合导航、环境适应、逻辑节点、日志 Sink、集群 Sink 等 |

---

## 4. 代码生成与构建部署

| 功能 | 现状 | 待实现 |
|------|------|--------|
| 生成预览 | 无 | `previewGeneratedFiles`：生成前返回将要生成的文件列表与内容预览，不落盘（设计 5.3 CodeGenerationService） |
| 生成目标 | 仅 C++ + CMake | **多语言**：如 Python 目标；**目标平台**：板端/仿真/云端（设计 CodeGenConfig.language、targetPlatform） |
| 部署模式 | 未区分为单机/多机/集群 | CodeGenConfig.deploymentMode（SINGLE_UAV / MULTI_UAV / CLUSTER_CENTER），影响生成内容与部署描述 |
| Dockerfile | 未生成 | 需求 4.2.2：生成 Dockerfile，便于容器化构建 |
| docker-compose | 未生成 | 需求 4.2.2：生成 docker-compose，一键起多服务（设计 DeploymentDescriptorService.generateDockerCompose） |
| K8s 编排 | 未生成 | 设计：generateK8sManifests，生成 K8s Deployment/Service 等 |
| 构建与打包 | 未实现 | BuildService.buildImages、packageArtifacts（镜像构建、制品打包），设计中有接口大纲 |

---

## 5. 部署拓扑与多机

| 功能 | 现状 | 待实现 |
|------|------|--------|
| 部署拓扑 | FlowDefinition 无 `deploymentTopology` | 设计 3.2：DeploymentTopology（nodes[] 部署单元如 UAV1/UAV2/Center，assignments[] 流程节点到部署单元的映射），用于多机场景下「哪些节点跑在哪台机」 |
| 单机/集群场景 | 设计标注「支持单机与集群场景」为计划中 | 流程中可配置多机角色、节点与部署单元绑定，代码生成与部署描述据此区分 |

---

## 6. 仿真与调试

| 功能 | 现状 | 待实现 |
|------|------|--------|
| 逻辑仿真 | 无 | SimulationEngine：start/step/runToEnd/stop/getState；SimulationEventPublisher 订阅事件（设计 5.4）。用于不跑真实硬件时单步/连续执行流程逻辑 |
| 仿真与预演 | 设计「待实现」中列出 | 前端「仿真」按钮触发后端仿真，并展示当前节点、变量、事件流 |

---

## 7. 行业模板与复用

| 功能 | 现状 | 待实现 |
|------|------|--------|
| 行业模板 | 无 | IndustryTemplateService：按行业（农业/工业/消防/军事等）列出模板、基于模板创建流程、可配置参数（设计 6.1） |
| 子流程/子图 | 无 | 设计 3.2 IR：子流程、复用节点（子图引用），即一个流程可引用另一流程为「子图」节点 |

---

## 8. 与 Viewer / Cluster Center 的集成

| 功能 | 现状 | 待实现 |
|------|------|--------|
| 任务配置同步到 Viewer | 设计「生成的工程可附带 Viewer 识别的任务元数据」「Builder 可将生成的任务配置同步到 Viewer 所使用的后端」 | 生成时或单独接口：将任务名称、参数、UAV 列表等推送到 Viewer 后端，便于在 Viewer 中直接展示与下发 |
| Builder ↔ Cluster Center | 需求 8.2：Builder 与 Cluster Center 通过 REST 交互 | 具体接口（如上传流程、查询任务状态）未在 Builder 中实现 |

---

## 9. 后端架构与接口

| 功能 | 现状 | 待实现 |
|------|------|--------|
| 模块拆分 | 单文件 main.py 含模型、存储、模板、代码生成、版本、对比等 | 按设计拆分为：FlowRepository、FlowVersionService、NodeTemplateService、CodeGenerationService、BuildService、DeploymentDescriptorService、SimulationEngine 等，便于测试与扩展 |
| 版本 diff 展示 | 已有 compare 接口返回节点/边增删改 | 可选：返回可读的 diff 文本或前端可视化 diff 视图 |
| 生成结果落盘 | 当前仅返回 main.cpp/CMakeLists.txt 字符串 | 可选：服务端配置输出目录，生成后直接写入并返回路径或压缩包下载 |

---

## 10. 汇总表

| 类别 | 未实现项 | 优先级建议 |
|------|----------|------------|
| 工程/流程 | 工程更新与删除、流程克隆、流程导入、流程验证 | 高 |
| 编辑器 | 节点对齐/分组/注释、全局变量、多选批量操作 | 中 |
| 节点库 | 从 SDK 导入模板、自定义模板、更多节点类型（逻辑/集群/SLAM/环境等） | 高（依赖 SDK 能力） |
| 代码与部署 | 生成预览、多语言/多平台、Dockerfile、docker-compose、K8s、构建与打包 | 高（部署）、中（预览/多目标） |
| 拓扑与多机 | deploymentTopology、节点与部署单元绑定 | 中 |
| 仿真 | SimulationEngine、仿真 API 与前端预演 | 中 |
| 行业与复用 | 行业模板、子流程/子图引用 | 低 |
| 集成 | 与 Viewer 任务同步、与 Cluster Center 接口 | 高（业务闭环） |
| 后端 | 模块化拆分、生成落盘/下载 | 中 |

---

## 11. 与需求/设计文档的对应

- **PROJECT_REQUIREMENTS_DOCUMENT.md**  
  - 4.1.1 流程设计：流程验证、更完整的节点库未完全实现。  
  - 4.2.1 代码生成：多目标与预览未实现。  
  - 4.2.2 容器化部署：Dockerfile、docker-compose、一键部署未实现。

- **Doc/05_FalconMindBuilder_Design.md**  
  - 第四节「待实现功能」：工程保存/打开/导出（部分有）、节点对齐/分组/注释、全局变量、模板管理、仿真与预演 — 多数未实现。  
  - 第五节后端接口大纲：FlowVersionService 已部分实现；NodeTemplateService.importFromSdk、createCustomTemplate、deleteTemplate 未实现；BuildService、DeploymentDescriptorService、SimulationEngine 未实现；IndustryTemplateService 未实现。  
  - 第七节与 SDK/Viewer 集成：Viewer 任务同步、Cluster Center 对接未实现。

- **Doc/BUILDER_OPTIMIZATION_RECOMMENDATIONS.md**  
  - 项目/流程选择器、节点与连接删除、撤销重做等已在当前版本实现；其余优化（对齐、分组、键盘快捷键完善、性能等）可与本文档合并排期。

完成上述项后，Builder 可达到设计文档与需求中的「零代码/低代码流程编排 + 多目标代码生成 + 容器化部署 + 仿真与行业模板 + 与 Viewer/Cluster Center 闭环」目标。
