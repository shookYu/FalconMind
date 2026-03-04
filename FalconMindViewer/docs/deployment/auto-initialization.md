# 数据库自动化初始化指南

## 概述

FalconMindViewer 支持多种自动化数据库初始化方案，无需手动执行命令。

## 🚀 快速开始 (推荐)

### 方法1: 应用自动检测 (已启用)

应用启动时会自动检测数据库状态并初始化：

```bash
# 启动所有服务
docker-compose -f docker-compose.auto.yml up -d
```

应用会自动：
1. ✅ 检查数据库连接
2. ✅ 创建表结构 (如果表不存在)
3. ✅ 初始化默认数据 (如果数据库为空)
4. ✅ 跳过已存在的数据 (幂等性)

**默认账号**: admin / admin123

---

## ⚙️ 配置选项

### 环境变量

在 `.env` 文件中配置：

```env
# 启用/禁用自动初始化
AUTO_INIT_DB=true        # 自动创建表结构
AUTO_INIT_DATA=true      # 自动初始化默认数据

# 或者完全禁用
AUTO_INIT_DB=false
```

### 禁用自动初始化

```bash
# 方法1: 环境变量
export AUTO_INIT_DB=false
docker-compose up -d

# 方法2: Docker Compose
AUTO_INIT_DB=false docker-compose up -d
```

---

## 🛠️ 方案对比

| 方案 | 适用场景 | 复杂度 | 可靠性 |
|------|----------|--------|--------|
| **应用自动检测** | 开发/生产 | 低 | 高 |
| **Docker Entrypoint** | 生产部署 | 中 | 高 |
| **手动初始化** | 精细控制 | 高 | 高 |
| **CI/CD Pipeline** | 自动化部署 | 中 | 高 |

---

## 🔧 高级用法

### 手动触发初始化

即使禁用了自动初始化，也可以通过API手动触发：

```bash
curl -X POST http://localhost:9000/api/v1/admin/init-db
```

### 查看初始化状态

```bash
# 检查健康状态
curl http://localhost:9000/api/v1/health

# 检查数据库连接
# 应用日志会显示初始化状态
```

### 重置数据库 (⚠️ 危险操作)

```bash
# 1. 停止服务
docker-compose down

# 2. 删除数据卷
docker volume rm falconmind_postgres_data

# 3. 重新启动 (自动初始化)
docker-compose up -d
```

---

## 📋 初始化内容

自动初始化会创建以下内容：

### 1. 数据库表
- users (用户表)
- uavs (无人机表)
- block_categories (任务块分类)
- blocks (任务块)
- missions (任务表)
- flows (流程表)

### 2. 默认用户
- **用户名**: admin
- **密码**: admin123
- **角色**: 管理员

### 3. 任务块分类 (5个)
- 运动控制 (Movement)
- 任务执行 (Mission)
- 感知处理 (Perception)
- 逻辑控制 (Control)
- 通信 (Communication)

### 4. 内置任务块 (11个)
- 起飞、降落、移动到位置、悬停
- 开始录像、停止录像、拍照
- 目标检测
- 等待、电量判断、返航

### 5. 演示UAV (3个)
- UAV-001: 侦察无人机-01
- UAV-002: 侦察无人机-02
- UAV-003: 巡检无人机-01

---

## 🐛 故障排查

### 问题1: 初始化失败

**症状**: 应用启动但无法访问API

**解决**:
```bash
# 查看日志
docker-compose logs backend

# 手动初始化
docker-compose exec backend python scripts/init_data.py
```

### 问题2: 数据库连接失败

**症状**: "Database is unavailable"

**解决**:
```bash
# 检查数据库状态
docker-compose ps postgres

# 重启数据库
docker-compose restart postgres
```

### 问题3: 数据重复

**症状**: 提示唯一键冲突

**解决**:
自动初始化已经是幂等的，如果出现问题：
```bash
# 重置数据库
docker-compose down -v
docker-compose up -d
```

---

## 🔐 生产环境建议

### 1. 禁用自动初始化 (推荐)

生产环境应使用 Alembic 迁移：

```env
AUTO_INIT_DB=false
```

手动执行：
```bash
# 在 CI/CD 中执行
alembic upgrade head
```

### 2. 使用 Secrets

```bash
# 生成密钥
openssl rand -hex 32

# 使用 Docker Secrets
echo "my-secret-key" | docker secret create secret_key -
```

### 3. 备份策略

```bash
# 自动备份脚本
docker-compose exec postgres pg_dump -U falconmind falconmind > backup.sql
```

---

## 📖 相关文档

- [部署指南](deployment-guide.md)
- [API 文档](api-reference.md)
- [开发指南](../README.md)
