# FalconMindBuilder 开发任务清单

> **项目**: FalconMindBuilder - UAV 边缘侧可视化开发工具  
> **版本**: v1.0 MVP  
> **目标**: 实现 Builder 核心功能，支持任务编排、即时部署、配置解释执行  
> **时间**: 8 周开发周期

---

## 📁 项目目录结构

```
FalconMindBuilder/
├── frontend/              # 前端 (Vue3 + TypeScript)
│   ├── src/
│   │   ├── components/    # 组件
│   │   ├── views/         # 页面
│   │   ├── composables/   # 组合式函数
│   │   ├── stores/        # Pinia 状态管理
│   │   ├── types/         # TypeScript 类型
│   │   ├── utils/         # 工具函数
│   │   └── assets/        # 静态资源
│   ├── public/
│   └── package.json
├── backend/               # 后端 (FastAPI + Python)
│   ├── app/
│   │   ├── api/           # API 路由
│   │   ├── services/      # 业务逻辑
│   │   ├── models/        # 数据模型
│   │   ├── core/          # 核心配置
│   │   └── utils/         # 工具函数
│   ├── tests/
│   └── requirements.txt
├── tests/                 # 测试
│   ├── unit/              # 单元测试
│   ├── integration/       # 集成测试
│   └── e2e/               # 端到端测试
├── docs/                  # 文档
│   ├── api/               # API 文档
│   ├── design/            # 设计文档
│   └── deployment/        # 部署文档
├── config/                # 配置文件
│   ├── dev/               # 开发环境
│   ├── prod/              # 生产环境
│   └── test/              # 测试环境
├── scripts/               # 脚本
└── Doc/                   # 设计文档（已有）
```

---

## 🎯 开发阶段划分

### Phase 1: 基础架构搭建 (Week 1-2)

#### 1.1 项目初始化 ✅

**前端初始化**
- [x] 创建 Vue3 + TypeScript 项目
  - [x] 使用 Vite 初始化项目
  - [x] 配置 TypeScript 严格模式
  - [x] 安装 Vue Router 4
  - [x] 安装 Pinia 状态管理
  - [x] 配置 ESLint + Prettier
  - [x] 配置路径别名 (@/components 等)
  
**后端初始化**
- [x] 创建 FastAPI + Python 项目
  - [x] 初始化 requirements.txt
  - [x] 配置 FastAPI 应用结构
  - [x] 安装相关依赖
  - [x] 配置开发环境 (uvicorn reload)
  - [x] 配置 CORS
  
#### 1.2 基础依赖安装

**前端依赖**
- [x] UI 框架: Element Plus
- [x] 画布编辑器: @vue-flow/core + @vue-flow/background
- [x] 地图: Cesium
- [x] HTTP 客户端: Axios
- [x] 表单验证: Zod
- [x] 工具库: Lodash-es, Day.js

**后端依赖**
- [x] 数据库: SQLAlchemy + SQLite
- [x] 验证: Pydantic
- [x] 日志: loguru
- [x] CORS: FastAPI 内置
- [x] MQTT 客户端: paho-mqtt
- [x] SDK FFI: ctypes

#### 1.3 配置文件

- [x] 创建开发/生产/测试环境配置
- [x] 配置 Git 忽略 (.gitignore)
- [x] 创建启动脚本 (start.sh)

**交付物:**
- [x] 可运行的前端项目 (npm run dev)
- [x] 可运行的后端项目 (./start.sh)
- [x] 统一的代码风格配置

---

### Phase 2: 前端核心功能 (Week 3-4)

#### 2.1 基础布局与导航 ✅

- [x] 实现主布局组件
  - [x] FlowEditorView 编排页面
  - [x] 左侧边栏 - 组件库
  - [x] 右侧属性面板
  - [x] 主画布区域
  
- [x] 路由配置
  - [x] /builder - 编排页面
  - [x] /preview - 预览页面
  - [x] /templates - 模板库
  - [x] /settings - 设置页面

#### 2.2 画布编辑器 (Vue-Flow) ✅

- [x] 画布基础组件
  - [x] 初始化 Vue-Flow 画布
  - [x] 配置网格和背景
  - [x] 实现缩放和平移
  
- [x] 节点系统
  - [x] 基础节点样式
  - [x] 触发器节点样式
  - [x] 动作节点样式
  - [x] 条件节点样式
  - [x] 节点选中/高亮效果
  
- [x] 边线连接
  - [x] 实现节点间连接
  - [x] 边线删除功能

#### 2.3 组件库面板 ✅

- [x] 组件分类展示
  - [x] 触发器分类 (mission_start, timer, battery_low...)
  - [x] 动作分类 (search_area, take_photo, hover...)
  - [x] 条件分类 (battery, altitude, target_detected...)
  
- [x] 拖拽功能
  - [x] 从组件库拖拽到画布

#### 2.4 属性面板 ✅

- [x] 属性编辑器框架
  - [x] 根据选中节点动态显示属性
  - [x] SearchAreaProperties 组件
  - [x] PhotoProperties 组件
  - [x] HoverProperties 组件
  - [x] ConditionProperties 组件
  
- [x] 具体属性编辑
  - [x] 搜索区域编辑器 (Cesium 地图)
  - [x] 高度/速度滑块
  - [x] 检测参数配置
  - [x] 触发条件配置

#### 2.5 状态管理 (Pinia) ✅

- [x] 创建 Store
  - [x] useFlowStore - 画布状态
  - [x] 节点增删改查
  - [x] 边线增删改查
  - [x] 撤销/重做 (Undo/Redo)

**交付物:**
- [x] 可交互的画布编辑器
- [x] 可拖拽的组件库
- [x] 动态属性面板 (含地图编辑器)
- [x] 基础状态管理

---

### Phase 3: 后端核心功能 (Week 4-5)

#### 3.1 API 路由设计 ✅

- [x] 项目路由 (/api/projects)
  - [x] GET / - 获取项目列表
  - [x] POST / - 创建项目
  - [x] GET /:id - 获取项目详情
  - [x] PUT /:id - 更新项目
  - [x] DELETE /:id - 删除项目
  
- [x] Flow 路由 (/api/projects/{id}/flows)
  - [x] GET / - 获取 Flow 列表
  - [x] POST / - 创建 Flow
  - [x] GET /:flow_id - 获取 Flow
  - [x] PUT /:flow_id - 更新 Flow
  - [x] DELETE /:flow_id - 删除 Flow
  - [x] GET /:flow_id/export - 导出 Flow
  - [x] POST /:flow_id/validate - 验证 Flow ✅ 新增
  - [x] POST /:flow_id/execute - 执行 Flow ✅ 新增

#### 3.2 数据模型 ✅

- [x] 数据库设计 (SQLite)
  - [x] projects 表
  - [x] flows 表 (节点和边线 JSON)
  
- [x] 模型实现
  - [x] Project 模型
  - [x] Flow 模型

#### 3.3 配置验证服务 ✅

- [x] 配置验证器
  - [x] 节点连接验证
  - [x] 必填参数验证
  - [x] 业务规则验证
  - [x] 循环依赖检测
  
- [x] 错误处理
  - [x] 统一的错误响应格式
  - [x] 详细的错误信息

#### 3.4 SDK 集成服务 ✅

- [x] SDK C API 绑定
  - [x] ctypes FFI 封装
  - [x] FlowExecutor 管理
  
- [x] Flow 执行服务
  - [x] load_flow
  - [x] start/stop
  - [x] get_status

#### 3.5 UAV 通信服务 ⚠️

- [x] MQTT 服务基础
  - [x] paho-mqtt 集成
  - [ ] UAV 发现
  - [ ] 完整部署流程

**交付物:**
- [x] 完整的 REST API
- [x] 数据库模型和迁移
- [x] 配置验证和导出
- [x] SDK FFI 集成

---

### Phase 4: 集成与预览 (Week 5-6) ⚠️

#### 4.1 前后端集成 ✅

- [x] API 客户端封装
  - [x] 封装 Axios 实例
  - [x] 请求/响应拦截器
  - [x] 错误处理
  - [x] 类型定义
  
- [x] 数据同步
  - [x] 手动保存
  - [ ] 自动保存 (防抖)
  - [x] 加载项目

#### 4.2 实时预览 ⚠️

- [x] Cesium 初始化 ✅
  - [x] vite-plugin-cesium 配置
  - [x] useCesium composable
  - [x] MapEditor 组件
  
- [ ] 预览组件
  - [ ] UAV 运动模拟
  - [ ] 航点到达事件
  - [ ] 状态更新

#### 4.3 地图标绘 ✅

- [x] Cesium 组件封装
  - [x] useCesium composable
  - [x] MapEditor 组件
  
- [x] 标绘功能
  - [x] 多边形绘制 (搜索区域)
  - [ ] 航点标记
  - [ ] 轨迹线显示
  - [ ] 区域编辑 (拖拽顶点)

#### 4.4 任务部署 ⚠️

- [x] 部署对话框
  - [x] DeployDialog 组件
  
- [ ] 部署状态
  - [ ] 部署进度显示
  - [ ] 部署历史记录

**交付物:**
- [x] 完整的前后端集成
- [x] Cesium 地图编辑器
- [ ] 实时预览功能
- [ ] 任务部署功能

---

### Phase 5: 高级功能 (Week 6-7) 🔄

#### 5.1 模板系统 🔄

- [ ] 模板库
  - [ ] 内置模板 (搜索、巡逻、巡检)
  - [ ] 模板分类展示
  - [ ] 模板预览
  
- [ ] 模板实例化
  - [ ] 从模板创建项目
  - [ ] 参数替换
  - [ ] 向导式配置

#### 5.2 规则引擎 ⚠️

- [x] 规则编辑器
  - [x] 基础验证
  - [ ] 触发条件配置 UI
  - [ ] 动作序列配置
  
- [x] 规则验证
  - [x] 循环检测
  - [ ] 冲突检测

#### 5.3 多 UAV 支持 ❌

- [ ] UAV 管理
  - [ ] UAV 列表展示
  - [ ] UAV 状态监控
  - [ ] 批量部署

#### 5.4 导入/导出 ⚠️

- [x] 导出功能
  - [x] 导出为 SDK JSON
  - [ ] 导出 YAML
  - [ ] 导出压缩包
  
- [ ] 导入功能
  - [ ] 导入 JSON/YAML 配置

**交付物:**
- [ ] 模板系统
- [x] 基础规则验证
- [ ] 多 UAV 支持
- [ ] 完善的导入导出

---

### Phase 6: 测试与优化 (Week 7-8) ⚠️

#### 6.1 单元测试 ⚠️

**前端测试**
- [ ] 组件测试 (Vue Test Utils)
  - [ ] 画布组件测试
  - [ ] 节点组件测试
  - [ ] 属性面板测试
  
- [ ] Store 测试
  - [ ] Pinia Store 测试

**后端测试**
- [ ] 服务层测试 (pytest)
  - [x] 集成测试脚本
  - [ ] 验证服务测试
  - [ ] SDK FFI 测试

#### 6.2 集成测试 ✅

- [x] API 调用测试
- [x] 数据流测试

#### 6.3 性能优化 ⚠️

**前端优化**
- [x] 组件懒加载 (路由懒加载)
- [ ] 大数据量渲染优化
- [ ] 内存泄漏检查

**后端优化**
- [x] 数据库查询优化
- [ ] 缓存策略
- [ ] 并发处理

#### 6.4 部署准备 ✅

- [x] 生产环境配置
  - [x] Dockerfile (前端)
  - [x] Dockerfile (后端)
  - [x] docker-compose.yml
  
- [x] 部署脚本
  - [x] 一键启动脚本 (start.sh)

**交付物:**
- [ ] 测试覆盖率 > 70%
- [ ] 性能优化报告
- [x] Docker 部署支持
- [x] 生产环境配置

---

### Phase 7: 文档与发布 (Week 8) ⚠️

#### 7.1 技术文档 ✅

- [x] API 文档
  - [x] OpenAPI/Swagger 文档 (FastAPI 自动生成)
  - [x] API 使用示例 (test_integration.py)
  
- [x] 架构说明
  - [x] IMPLEMENTATION_STATUS.md

#### 7.2 用户文档 ❌

- [ ] 用户手册
  - [ ] 快速开始
  - [ ] 功能说明
  - [ ] 常见问题

#### 7.3 发布准备 ⚠️

- [x] README 完善
- [ ] CHANGELOG
- [ ] Release Notes

**交付物:**
- [x] 技术文档
- [ ] 用户手册
- [ ] v1.0 版本发布

---

## 📊 任务追踪

### 当前进度

| 阶段 | 任务数 | 已完成 | 进度 |
|------|--------|--------|------|
| Phase 1: 基础架构 | 12 | 12 | 100% ✅ |
| Phase 2: 前端核心 | 25 | 22 | 88% ✅ |
| Phase 3: 后端核心 | 20 | 18 | 90% ✅ |
| Phase 4: 集成预览 | 15 | 8 | 53% ⚠️ |
| Phase 5: 高级功能 | 16 | 5 | 31% ⚠️ |
| Phase 6: 测试优化 | 20 | 8 | 40% ⚠️ |
| Phase 7: 文档发布 | 12 | 6 | 50% ⚠️ |
| **总计** | **120** | **79** | **66%** |

### 里程碑

- [x] **Week 2 结束**: 基础架构完成，可运行空项目
- [x] **Week 4 结束**: 前端画布编辑器可交互，含 Cesium 地图
- [x] **Week 5 结束**: 后端 API 完整，数据库就绪，SDK FFI 完成
- [ ] **Week 6 结束**: 前后端集成，可保存/加载项目 ✅
- [ ] **Week 7 结束**: 预览和部署功能可用
- [ ] **Week 8 结束**: MVP 发布，文档完整

---

## 🛠️ 技术栈

### 前端
- **框架**: Vue 3 (Composition API)
- **语言**: TypeScript 5.x
- **构建**: Vite 5.x
- **路由**: Vue Router 4
- **状态**: Pinia 2.x
- **UI**: Element Plus 2.x
- **画布**: @vue-flow/core 1.x
- **地图**: Cesium 1.x
- **测试**: Vitest

### 后端
- **运行时**: Python 3.11+
- **框架**: FastAPI
- **数据库**: SQLite (SQLAlchemy)
- **验证**: Pydantic
- **日志**: loguru
- **测试**: pytest

### 工具
- **包管理**: pnpm (frontend), pip (backend)
- **代码规范**: ESLint + Prettier
- **Docker**: Docker + Docker Compose

---

## 📝 变更记录

### v1.0 (2024-03)
- ✅ 基础架构搭建完成
- ✅ 前端画布编辑器完成
- ✅ 后端 API 完成
- ✅ Cesium 地图编辑器集成
- ✅ SDK FFI 绑定实现
- ✅ Flow 验证服务
- ✅ Docker 部署配置

---

**创建日期**: 2024-03-02  
**最后更新**: 2024-03-03  
**版本**: v1.0