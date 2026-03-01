# FalconMindConsole

FalconMindConsole - 无人机智能任务统一控制台

> 整合 FalconMindBuilder + FalconMindViewer，提供一站式任务编排、部署与监控能力

---

## 📖 文档导航

### 架构设计
- [系统架构设计（概述版）](docs/architecture/system-architecture-overview.md) - 总体架构设计
- [系统架构设计（基于代码分析）](docs/architecture/system-architecture-code-based.md) - 基于实际代码的详细架构
- [整合架构设计](docs/architecture/integration-architecture.md) - Builder+Viewer整合方案
- [详细设计文档](docs/architecture/system-design-v1.md) - 完整的数据库、API、组件设计

### API文档
- [API参考文档](docs/api/api-reference.md) - 完整API端点说明

### 部署文档
- [部署指南](docs/deployment/deployment-guide.md) - 开发/生产环境部署

### 开发资源
- [📋 开发任务清单](TODO.md) - 详细的开发任务分解 (12周计划)
- [📊 执行跟踪](PROGRESS.md) - 每日/每周进度跟踪
- [💻 代码框架](docs/code-framework.md) - 核心代码框架参考

---

## 🚀 快速开始

### 环境要求
- Node.js 18+
- Python 3.11+
- PostgreSQL 15+
- Redis 7+

### 方式1: 使用快速启动脚本（推荐）

```bash
# 1. 克隆项目
git clone <repository-url>
cd FalconMindConsole

# 2. 一键启动开发环境
./start-dev.sh

# 3. 启动所有服务
./start-services.sh

# 4. 访问
http://localhost:8080
```

### 方式2: 手动搭建

```bash
# 1. 克隆项目
git clone <repository-url>
cd FalconMindConsole

# 2. 启动基础设施
docker-compose -f docker-compose.dev.yml up -d

# 3. 启动后端
cd backend
python -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate
pip install -r requirements.txt
alembic upgrade head
python -m app.main

# 4. 启动前端
cd ../frontend
npm install
npm run dev

# 5. 访问
http://localhost:8080
```

默认账号：
- 用户名: admin
- 密码: admin123

### 生产环境部署

```bash
# 使用 Docker Compose
docker-compose -f docker-compose.prod.yml up -d
```

详见 [部署指南](docs/deployment/deployment-guide.md)

---

## 📁 项目结构

```
FalconMindConsole/
├── frontend/           # Vue3前端
├── backend/            # FastAPI后端
├── docs/               # 文档
│   ├── architecture/   # 架构设计
│   ├── api/            # API文档
│   └── deployment/     # 部署文档
├── scripts/            # 脚本
├── config/             # 配置文件
├── tests/              # 测试
├── TODO.md             # 开发任务清单
├── PROGRESS.md         # 进度跟踪
└── README.md           # 本文件
```

---

## 🛠️ 技术栈

### 前端
- Vue 3.3+ + TypeScript
- Vite + Pinia
- Element Plus + CesiumJS

### 后端
- FastAPI + SQLAlchemy
- PostgreSQL + Redis
- MQTT / WebSocket

### 边缘
- NodeAgent (C++)
- FalconMindSDK

---

## 📅 开发计划

详见 [TODO.md](TODO.md) 和 [详细设计文档](docs/architecture/system-design-v1.md#九实施计划)

**Phase 1** (Week 1-2): 基础搭建 ⏳  
**Phase 2** (Week 3-6): 后端核心 ⏳  
**Phase 3** (Week 7-9): 前端开发 ⏳  
**Phase 4** (Week 10-11): 测试优化 ⏳  
**Phase 5** (Week 12): 生产部署 ⏳

---

## 🤝 贡献

1. Fork项目
2. 创建特性分支 (`git checkout -b feature/xxx`)
3. 提交代码 (`git commit -m 'feat: add xxx'`)
4. 创建PR

---

## 📄 许可证

Apache License 2.0

---

**快速链接**:
- [🐛 提交Bug](https://github.com/your-org/FalconMindConsole/issues)
- [💡 功能建议](https://github.com/your-org/FalconMindConsole/discussions)
- [📖 查看文档](./docs)
