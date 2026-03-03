# FalconMindBuilder

> UAV 边缘侧可视化开发工具 - 零代码/低代码构建无人机业务应用

## 🎯 项目概述

FalconMindBuilder 是运行在 UAV 边缘设备（RK3588/RK3576）上的可视化开发工具，提供 BS 架构的 Web UI，让用户通过浏览器即可快速开发无人机业务应用。

## 🏗️ 系统架构

```
用户 PC/平板
    │
    │ WiFi / 有线网络
    ▼
┌─────────────────────────────────────────────────────────────┐
│                UAV 边缘设备 (RK3588)                         │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │           FalconMindBuilder (BS 架构)                  │  │
│  │  ┌──────────────┐  ┌──────────────┐  ┌────────────┐  │  │
│  │  │  Vue3 前端   │  │  Node.js后端 │  │  SQLite    │  │  │
│  │  │ (浏览器访问) │  │  (API 服务)  │  │ (配置存储) │  │  │
│  │  └──────┬───────┘  └──────┬───────┘  └─────┬──────┘  │  │
│  │         │                 │                │         │  │
│  │         └─────────────────┼────────────────┘         │  │
│  │                           ▼                          │  │
│  │         ┌────────────────────────────────────┐      │  │
│  │         │    SDK FlowExecutor (解释执行)      │      │  │
│  │         └──────────────┬───────────────────────┘      │  │
│  │                        │                              │  │
│  │           ┌────────────┴────────────┐                 │  │
│  │           ▼                         ▼                 │  │
│  │  ┌────────────────┐      ┌────────────────┐          │  │
│  │  │ FalconMindSDK  │─────▶│ 飞控 (PX4)     │          │  │
│  │  └────────────────┘      └────────────────┘          │  │
│  └───────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

## 📁 项目结构

```
FalconMindBuilder/
├── frontend/              # Vue3 + TypeScript 前端
│   ├── src/
│   │   ├── components/    # 组件
│   │   ├── views/         # 页面
│   │   ├── composables/   # 组合式函数
│   │   ├── stores/        # Pinia 状态管理
│   │   ├── types/         # TypeScript 类型
│   │   ├── utils/         # 工具函数
│   │   └── assets/        # 静态资源
│   └── package.json
├── backend/               # Node.js + Express 后端
│   ├── src/
│   │   ├── routes/        # API 路由
│   │   ├── services/      # 业务逻辑
│   │   ├── models/        # 数据模型
│   │   ├── middleware/    # 中间件
│   │   ├── types/         # TypeScript 类型
│   │   └── utils/         # 工具函数
│   └── package.json
├── tests/                 # 测试
│   ├── unit/              # 单元测试
│   ├── integration/       # 集成测试
│   └── e2e/               # 端到端测试
├── docs/                  # 项目文档
├── config/                # 配置文件
├── scripts/               # 脚本
├── Doc/                   # 设计文档
└── TODO.md                # 开发任务清单
```

## 🚀 快速开始

### 环境要求

- Node.js 18+
- pnpm 8+ (推荐)
- SQLite3

### 安装依赖

```bash
# 前端
cd frontend
pnpm install

# 后端
cd backend
pnpm install
```

### 开发模式

```bash
# 启动前端 (端口 5173)
cd frontend
pnpm dev

# 启动后端 (端口 3000)
cd backend
pnpm dev
```

### 构建

```bash
# 构建前端
cd frontend
pnpm build

# 构建后端
cd backend
pnpm build
```

### 部署

```bash
# 使用 Docker
docker-compose up -d

# 或手动部署
./scripts/deploy.sh
```

## 📖 文档

- [设计文档](./Doc/) - 架构设计、可行性分析、技术细节
- [开发任务清单](./TODO.md) - 详细的开发计划
- [API 文档](./docs/api/) - REST API 文档
- [部署文档](./docs/deployment/) - 部署指南

## 🛠️ 技术栈

### 前端
- Vue 3 + TypeScript
- Vite
- Pinia
- Element Plus
- @vue-flow/core
- Cesium

### 后端
- Node.js + Express
- TypeScript
- SQLite3
- Zod

## 📝 开发规范

- 代码风格: ESLint + Prettier
- Git 提交: Conventional Commits
- 分支管理: Git Flow

## 📊 开发进度

参见 [TODO.md](./TODO.md) 了解详细的开发计划和当前进度。

## 🤝 贡献

1. Fork 项目
2. 创建分支 (`git checkout -b feature/xxx`)
3. 提交更改 (`git commit -m 'feat: add xxx'`)
4. 推送分支 (`git push origin feature/xxx`)
5. 创建 Pull Request

## 📄 许可证

MIT License

---

**FalconMindBuilder** - 让无人机开发更简单 🚁
