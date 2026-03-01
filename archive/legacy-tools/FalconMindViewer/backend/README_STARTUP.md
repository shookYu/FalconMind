# Viewer Backend 启动说明

## 启动方式

### 方式1: 使用启动脚本（推荐）

使用 PoC_test 的启动脚本，会自动检测并使用优化版本：

```bash
cd /home/shook/work/FalconMind
./PoC_test/scripts/start_all_services.sh
```

**脚本行为**:
- 优先使用 `main_optimized.py`（优化版本）
- 如果优化版本不存在或依赖未安装，自动回退到 `main.py`（原版本）
- 自动检查并安装 `pydantic-settings` 依赖

### 方式2: 手动启动

#### 启动优化版本

```bash
cd FalconMindViewer/backend

# 确保依赖已安装
pip install -r requirements.txt

# 启动优化版本
python main_optimized.py

# 或使用 uvicorn
uvicorn main_optimized:app --host 0.0.0.0 --port 9000
```

#### 启动原版本（向后兼容）

```bash
cd FalconMindViewer/backend
python main.py

# 或使用 uvicorn
uvicorn main:app --host 0.0.0.0 --port 9000
```

## 版本选择逻辑

启动脚本的版本选择逻辑：

1. **检查优化版本** (`main_optimized.py`)
   - 如果文件存在，检查依赖 `pydantic-settings`
   - 如果依赖未安装，尝试自动安装
   - 如果安装失败，回退到原版本

2. **回退到原版本** (`main.py`)
   - 如果优化版本不可用，使用原版本
   - 确保向后兼容

## 停止服务

### 使用脚本停止

```bash
./PoC_test/scripts/stop_all_services.sh
```

### 手动停止

```bash
# 查找进程
ps aux | grep "uvicorn.*9000"

# 停止进程
kill <PID>

# 或强制停止
pkill -f "uvicorn.*9000"
```

## 重启服务

```bash
./PoC_test/scripts/restart_all_services.sh
```

## 检查服务状态

```bash
# 健康检查
curl http://localhost:9000/health

# 查看日志
tail -f /tmp/falconmind_logs/viewer.log

# 或查看本地日志
tail -f FalconMindViewer/backend/logs/viewer.log
```

## 配置

### 环境变量

创建 `.env` 文件（可选）：

```env
# API配置
API_HOST=0.0.0.0
API_PORT=9000
API_RELOAD=false

# 日志配置
LOG_LEVEL=INFO
LOG_FILE=./logs/viewer.log

# WebSocket配置
WS_MAX_CONNECTIONS=100
WS_HEARTBEAT_INTERVAL=30
```

### 默认配置

如果不设置环境变量，将使用 `config.py` 中的默认值。

## 故障排查

### 端口被占用

```bash
# 检查端口占用
lsof -i :9000

# 停止占用端口的进程
kill -9 <PID>
```

### 依赖问题

```bash
# 检查依赖
pip list | grep pydantic-settings

# 安装依赖
pip install -r requirements.txt
```

### 日志查看

```bash
# 查看启动日志
tail -f /tmp/falconmind_logs/viewer.log

# 查看应用日志
tail -f FalconMindViewer/backend/logs/viewer.log
```

## API 路由变更

优化版本使用 `/api/v1` 前缀：

- `POST /api/v1/ingress/telemetry` - 遥测接入
- `GET /api/v1/uavs` - 获取UAV列表
- `GET /api/v1/uavs/{uav_id}` - 获取UAV状态
- `GET /api/v1/missions` - 获取任务列表
- `POST /api/v1/missions` - 创建任务
- `WS /ws/telemetry` - WebSocket订阅（无前缀）

**注意**: 如果前端使用旧的路由，需要更新前端代码或添加路由兼容层。
