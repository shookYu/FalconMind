# FalconMindConsole 部署指南

> **版本**: v1.0  
> **最后更新**: 2026-02-28

---

## 目录

- [环境要求](#环境要求)
- [开发环境部署](#开发环境部署)
- [生产环境部署](#生产环境部署)
- [Docker部署](#docker部署)
- [Kubernetes部署](#kubernetes部署)
- [配置说明](#配置说明)
- [监控与日志](#监控与日志)
- [故障排查](#故障排查)

---

## 环境要求

### 开发环境

| 组件 | 版本 | 说明 |
|------|------|------|
| Node.js | 18.x+ | 前端运行环境 |
| Python | 3.11+ | 后端运行环境 |
| PostgreSQL | 15.x | 主数据库 |
| Redis | 7.x | 缓存、消息队列 |
| MQTT Broker | 2.x | 设备通信 (可选) |

### 生产环境

| 组件 | 最低配置 | 推荐配置 |
|------|----------|----------|
| CPU | 4核 | 8核+ |
| 内存 | 8GB | 16GB+ |
| 磁盘 | 100GB SSD | 500GB SSD+ |
| 网络 | 100Mbps | 1Gbps |

---

## 开发环境部署

### 1. 克隆代码

```bash
git clone https://github.com/your-org/FalconMindConsole.git
cd FalconMindConsole
```

### 2. 启动数据库

使用 Docker 启动 PostgreSQL 和 Redis：

```bash
docker-compose -f docker-compose.dev.yml up -d postgres redis
```

或者本地安装：

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y postgresql-15 redis-server

# macOS
brew install postgresql@15 redis
```

创建数据库：

```bash
sudo -u postgres psql -c "CREATE DATABASE falconmind;"
sudo -u postgres psql -c "CREATE USER falconmind WITH PASSWORD 'password123';"
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE falconmind TO falconmind;"
```

### 3. 后端部署

```bash
cd backend

# 创建虚拟环境
python3.11 -m venv venv
source venv/bin/activate  # Windows: venv\Scripts\activate

# 安装依赖
pip install -r requirements.txt

# 数据库迁移
alembic upgrade head

# 创建初始数据
python scripts/init_data.py

# 启动服务
uvicorn app.main:app --reload --host 0.0.0.0 --port 9000
```

后端服务将运行在 http://localhost:9000

### 4. 前端部署

```bash
cd frontend

# 安装依赖
npm install

# 启动开发服务器
npm run dev
```

前端将运行在 http://localhost:8080

### 5. 访问系统

打开浏览器访问：http://localhost:8080

默认账号：
- 用户名: admin
- 密码: admin123

---

## 生产环境部署

### 1. 系统准备

```bash
# Ubuntu 22.04 LTS

# 更新系统
sudo apt-get update
sudo apt-get upgrade -y

# 安装基础工具
sudo apt-get install -y \
    git \
    curl \
    wget \
    vim \
    htop \
    nginx \
    certbot \
    python3-certbot-nginx

# 安装 Node.js 18
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# 安装 Python 3.11
sudo apt-get install -y python3.11 python3.11-venv python3.11-pip

# 安装 PostgreSQL 15
sudo sh -c 'echo "deb http://apt.postgresql.org/pub/repos/apt $(lsb_release -cs)-pgdg main" > /etc/apt/sources.list.d/pgdg.list'
wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | sudo apt-key add -
sudo apt-get update
sudo apt-get install -y postgresql-15 postgresql-contrib

# 安装 Redis
sudo apt-get install -y redis-server
```

### 2. 配置 PostgreSQL

```bash
# 编辑配置文件
sudo vim /etc/postgresql/15/main/postgresql.conf

# 修改以下配置
listen_addresses = '*'
max_connections = 200
shared_buffers = 2GB
effective_cache_size = 6GB
work_mem = 10MB
maintenance_work_mem = 512MB

# 配置访问权限
sudo vim /etc/postgresql/15/main/pg_hba.conf

# 添加
host    falconmind    falconmind    127.0.0.1/32    md5
host    falconmind    falconmind    ::1/128         md5

# 重启服务
sudo systemctl restart postgresql

# 创建数据库和用户
sudo -u postgres psql -c "CREATE DATABASE falconmind;"
sudo -u postgres psql -c "CREATE USER falconmind WITH PASSWORD 'your_secure_password';"
sudo -u postgres psql -c "GRANT ALL PRIVILEGES ON DATABASE falconmind TO falconmind;"
```

### 3. 配置 Redis

```bash
sudo vim /etc/redis/redis.conf

# 修改以下配置
bind 127.0.0.1
port 6379
maxmemory 1gb
maxmemory-policy allkeys-lru
requirepass your_redis_password

# 重启服务
sudo systemctl restart redis
```

### 4. 部署后端

```bash
# 创建应用目录
sudo mkdir -p /opt/falconmind/backend
sudo chown -R $USER:$USER /opt/falconmind

# 克隆代码
cd /opt/falconmind
git clone https://github.com/your-org/FalconMindConsole.git backend
cd backend/backend

# 创建虚拟环境
python3.11 -m venv venv
source venv/bin/activate

# 安装依赖
pip install -r requirements.txt

# 创建环境变量文件
cat > .env << EOF
# 数据库
DATABASE_URL=postgresql://falconmind:your_secure_password@localhost:5432/falconmind

# Redis
REDIS_URL=redis://:your_redis_password@localhost:6379/0

# JWT
SECRET_KEY=your-secret-key-here-change-in-production
ALGORITHM=HS256
ACCESS_TOKEN_EXPIRE_MINUTES=60

# 环境
ENVIRONMENT=production
DEBUG=false

# CORS
CORS_ORIGINS=["https://your-domain.com"]

# MQTT (可选)
MQTT_ENABLED=true
MQTT_BROKER_HOST=localhost
MQTT_BROKER_PORT=1883
MQTT_USERNAME=mqtt_user
MQTT_PASSWORD=mqtt_password
EOF

# 数据库迁移
alembic upgrade head

# 创建初始数据
python scripts/init_data.py
```

### 5. 使用 Systemd 管理后端服务

```bash
sudo vim /etc/systemd/system/falconmind-backend.service
```

```ini
[Unit]
Description=FalconMindConsole Backend
After=network.target postgresql.service redis.service

[Service]
Type=simple
User=falconmind
Group=falconmind
WorkingDirectory=/opt/falconmind/backend/backend
Environment="PATH=/opt/falconmind/backend/backend/venv/bin"
EnvironmentFile=/opt/falconmind/backend/backend/.env
ExecStart=/opt/falconmind/backend/backend/venv/bin/uvicorn app.main:app --host 0.0.0.0 --port 9000 --workers 4
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
```

```bash
# 创建用户
sudo useradd -r -s /bin/false falconmind

# 设置权限
sudo chown -R falconmind:falconmind /opt/falconmind

# 启动服务
sudo systemctl daemon-reload
sudo systemctl enable falconmind-backend
sudo systemctl start falconmind-backend

# 查看状态
sudo systemctl status falconmind-backend
sudo journalctl -u falconmind-backend -f
```

### 6. 部署前端

```bash
# 构建前端
cd /opt/falconmind/backend/frontend

# 安装依赖
npm install

# 构建生产版本
npm run build

# 输出目录: dist/
```

### 7. 配置 Nginx

```bash
sudo vim /etc/nginx/sites-available/falconmind
```

```nginx
# HTTP 重定向到 HTTPS
server {
    listen 80;
    server_name your-domain.com;
    return 301 https://$server_name$request_uri;
}

# HTTPS
server {
    listen 443 ssl http2;
    server_name your-domain.com;

    # SSL 证书
    ssl_certificate /etc/letsencrypt/live/your-domain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/your-domain.com/privkey.pem;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers HIGH:!aNULL:!MD5;
    ssl_prefer_server_ciphers on;

    # 前端静态文件
    location / {
        root /opt/falconmind/backend/frontend/dist;
        index index.html;
        try_files $uri $uri/ /index.html;
    }

    # API 代理
    location /api/ {
        proxy_pass http://localhost:9000/;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection 'upgrade';
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
        proxy_cache_bypass $http_upgrade;
        
        # 超时设置
        proxy_connect_timeout 60s;
        proxy_send_timeout 60s;
        proxy_read_timeout 60s;
    }

    # WebSocket 代理
    location /ws/ {
        proxy_pass http://localhost:9000/ws/;
        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    # Gzip 压缩
    gzip on;
    gzip_vary on;
    gzip_min_length 1024;
    gzip_types text/plain text/css text/xml text/javascript application/javascript application/xml+rss application/json;
}
```

```bash
# 启用站点
sudo ln -s /etc/nginx/sites-available/falconmind /etc/nginx/sites-enabled/
sudo rm /etc/nginx/sites-enabled/default

# 测试配置
sudo nginx -t

# 重启 Nginx
sudo systemctl restart nginx
```

### 8. 配置 SSL 证书

```bash
# 使用 Let's Encrypt
sudo certbot --nginx -d your-domain.com

# 自动续期
sudo certbot renew --dry-run
```

---

## Docker部署

### 开发环境

```bash
# 构建并启动所有服务
docker-compose -f docker-compose.dev.yml up -d

# 查看日志
docker-compose -f docker-compose.dev.yml logs -f

# 停止服务
docker-compose -f docker-compose.dev.yml down
```

### 生产环境

```bash
# 构建生产镜像
docker-compose -f docker-compose.prod.yml build

# 启动服务
docker-compose -f docker-compose.prod.yml up -d

# 查看状态
docker-compose -f docker-compose.prod.yml ps

# 查看日志
docker-compose -f docker-compose.prod.yml logs -f backend
docker-compose -f docker-compose.prod.yml logs -f frontend

# 停止服务
docker-compose -f docker-compose.prod.yml down

# 数据备份
docker-compose -f docker-compose.prod.yml exec postgres pg_dump -U falconmind falconmind > backup.sql
```

### docker-compose.prod.yml

```yaml
version: '3.8'

services:
  postgres:
    image: postgres:15-alpine
    environment:
      POSTGRES_USER: falconmind
      POSTGRES_PASSWORD: ${DB_PASSWORD}
      POSTGRES_DB: falconmind
    volumes:
      - postgres_data:/var/lib/postgresql/data
    networks:
      - falconmind-network
    restart: unless-stopped
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U falconmind"]
      interval: 5s
      timeout: 5s
      retries: 5

  redis:
    image: redis:7-alpine
    command: redis-server --requirepass ${REDIS_PASSWORD}
    volumes:
      - redis_data:/data
    networks:
      - falconmind-network
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "redis-cli", "ping"]
      interval: 5s
      timeout: 3s
      retries: 5

  backend:
    build:
      context: ./backend
      dockerfile: Dockerfile.prod
    environment:
      - DATABASE_URL=postgresql://falconmind:${DB_PASSWORD}@postgres:5432/falconmind
      - REDIS_URL=redis://:${REDIS_PASSWORD}@redis:6379/0
      - SECRET_KEY=${SECRET_KEY}
      - ENVIRONMENT=production
    ports:
      - "9000:9000"
    volumes:
      - ./backend/logs:/app/logs
    networks:
      - falconmind-network
    depends_on:
      postgres:
        condition: service_healthy
      redis:
        condition: service_healthy
    restart: unless-stopped

  frontend:
    build:
      context: ./frontend
      dockerfile: Dockerfile.prod
    ports:
      - "80:80"
      - "443:443"
    volumes:
      - ./nginx/nginx.conf:/etc/nginx/nginx.conf
      - ./nginx/ssl:/etc/nginx/ssl
    networks:
      - falconmind-network
    depends_on:
      - backend
    restart: unless-stopped

volumes:
  postgres_data:
  redis_data:

networks:
  falconmind-network:
    driver: bridge
```

---

## Kubernetes部署

### 1. 创建 Namespace

```bash
kubectl create namespace falconmind
```

### 2. 配置 ConfigMap

```yaml
# k8s/configmap.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: falconmind-config
  namespace: falconmind
data:
  ENVIRONMENT: "production"
  DEBUG: "false"
  DATABASE_URL: "postgresql://falconmind:$(DB_PASSWORD)@postgres:5432/falconmind"
  REDIS_URL: "redis://:$(REDIS_PASSWORD)@redis:6379/0"
```

### 3. 配置 Secret

```bash
# 创建 Secret
kubectl create secret generic falconmind-secrets \
  --namespace falconmind \
  --from-literal=DB_PASSWORD=your_db_password \
  --from-literal=REDIS_PASSWORD=your_redis_password \
  --from-literal=SECRET_KEY=your_secret_key
```

### 4. 部署 PostgreSQL

```yaml
# k8s/postgres.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: postgres
  namespace: falconmind
spec:
  serviceName: postgres
  replicas: 1
  selector:
    matchLabels:
      app: postgres
  template:
    metadata:
      labels:
        app: postgres
    spec:
      containers:
      - name: postgres
        image: postgres:15-alpine
        env:
        - name: POSTGRES_USER
          value: falconmind
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: falconmind-secrets
              key: DB_PASSWORD
        - name: POSTGRES_DB
          value: falconmind
        ports:
        - containerPort: 5432
        volumeMounts:
        - name: postgres-storage
          mountPath: /var/lib/postgresql/data
  volumeClaimTemplates:
  - metadata:
      name: postgres-storage
    spec:
      accessModes: ["ReadWriteOnce"]
      resources:
        requests:
          storage: 100Gi
---
apiVersion: v1
kind: Service
metadata:
  name: postgres
  namespace: falconmind
spec:
  selector:
    app: postgres
  ports:
  - port: 5432
    targetPort: 5432
```

### 5. 部署后端

```yaml
# k8s/backend.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: backend
  namespace: falconmind
spec:
  replicas: 3
  selector:
    matchLabels:
      app: backend
  template:
    metadata:
      labels:
        app: backend
    spec:
      containers:
      - name: backend
        image: your-registry/falconmind-backend:v1.0.0
        ports:
        - containerPort: 9000
        envFrom:
        - configMapRef:
            name: falconmind-config
        env:
        - name: DATABASE_URL
          valueFrom:
            secretKeyRef:
              name: falconmind-secrets
              key: DATABASE_URL
        - name: SECRET_KEY
          valueFrom:
            secretKeyRef:
              name: falconmind-secrets
              key: SECRET_KEY
        resources:
          requests:
            memory: "512Mi"
            cpu: "500m"
          limits:
            memory: "1Gi"
            cpu: "1000m"
        livenessProbe:
          httpGet:
            path: /api/v1/health
            port: 9000
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /api/v1/health
            port: 9000
          initialDelaySeconds: 5
          periodSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: backend
  namespace: falconmind
spec:
  selector:
    app: backend
  ports:
  - port: 9000
    targetPort: 9000
  type: ClusterIP
```

### 6. 部署前端

```yaml
# k8s/frontend.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: frontend
  namespace: falconmind
spec:
  replicas: 2
  selector:
    matchLabels:
      app: frontend
  template:
    metadata:
      labels:
        app: frontend
    spec:
      containers:
      - name: frontend
        image: your-registry/falconmind-frontend:v1.0.0
        ports:
        - containerPort: 80
        resources:
          requests:
            memory: "128Mi"
            cpu: "100m"
          limits:
            memory: "256Mi"
            cpu: "200m"
---
apiVersion: v1
kind: Service
metadata:
  name: frontend
  namespace: falconmind
spec:
  selector:
    app: frontend
  ports:
  - port: 80
    targetPort: 80
  type: ClusterIP
```

### 7. 配置 Ingress

```yaml
# k8s/ingress.yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: falconmind-ingress
  namespace: falconmind
  annotations:
    kubernetes.io/ingress.class: nginx
    cert-manager.io/cluster-issuer: letsencrypt-prod
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  tls:
  - hosts:
    - your-domain.com
    secretName: falconmind-tls
  rules:
  - host: your-domain.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: backend
            port:
              number: 9000
      - path: /ws
        pathType: Prefix
        backend:
          service:
            name: backend
            port:
              number: 9000
      - path: /
        pathType: Prefix
        backend:
          service:
            name: frontend
            port:
              number: 80
```

### 8. 部署到 Kubernetes

```bash
# 应用配置
kubectl apply -f k8s/configmap.yaml
kubectl apply -f k8s/postgres.yaml
kubectl apply -f k8s/redis.yaml
kubectl apply -f k8s/backend.yaml
kubectl apply -f k8s/frontend.yaml
kubectl apply -f k8s/ingress.yaml

# 查看状态
kubectl get pods -n falconmind
kubectl get svc -n falconmind
kubectl get ingress -n falconmind

# 查看日志
kubectl logs -f deployment/backend -n falconmind
kubectl logs -f deployment/frontend -n falconmind

# 扩缩容
kubectl scale deployment backend --replicas=5 -n falconmind
```

---

## 配置说明

### 后端环境变量

| 变量名 | 必填 | 默认值 | 说明 |
|--------|------|--------|------|
| DATABASE_URL | 是 | - | PostgreSQL连接字符串 |
| REDIS_URL | 是 | - | Redis连接字符串 |
| SECRET_KEY | 是 | - | JWT密钥 |
| ENVIRONMENT | 否 | development | 环境: development, staging, production |
| DEBUG | 否 | true | 调试模式 |
| CORS_ORIGINS | 否 | ["*"] | CORS允许的源 |
| MQTT_ENABLED | 否 | false | 是否启用MQTT |
| MQTT_BROKER_HOST | 否 | localhost | MQTT服务器地址 |
| MQTT_BROKER_PORT | 否 | 1883 | MQTT端口 |

### 前端环境变量

| 变量名 | 必填 | 默认值 | 说明 |
|--------|------|--------|------|
| VITE_API_BASE_URL | 是 | - | API基础URL |
| VITE_WS_URL | 是 | - | WebSocket URL |
| VITE_CESIUM_BASE_URL | 否 | - | Cesium基础URL |

---

## 监控与日志

### 日志配置

后端日志默认输出到 `stdout`，可通过以下方式查看：

```bash
# Systemd
sudo journalctl -u falconmind-backend -f

# Docker
docker-compose logs -f backend

# Kubernetes
kubectl logs -f deployment/backend -n falconmind
```

### 监控指标

后端提供 Prometheus 指标端点：

```
GET /metrics
```

指标包括：
- HTTP请求数和延迟
- 数据库连接池状态
- UAV在线状态
- 任务执行统计

### 健康检查

```bash
# 后端健康检查
curl http://localhost:9000/api/v1/health

# 响应
{
  "status": "healthy",
  "version": "1.0.0",
  "timestamp": "2026-02-28T10:30:00Z",
  "checks": {
    "database": "ok",
    "redis": "ok",
    "mqtt": "ok"
  }
}
```

---

## 故障排查

### 常见问题

#### 1. 数据库连接失败

**症状**: 后端启动失败，报错 "could not connect to database"

**排查**:
```bash
# 检查PostgreSQL状态
sudo systemctl status postgresql

# 检查连接
psql -h localhost -U falconmind -d falconmind

# 检查日志
sudo tail -f /var/log/postgresql/postgresql-15-main.log
```

**解决**:
- 确认PostgreSQL已启动: `sudo systemctl start postgresql`
- 确认用户和数据库存在
- 检查 pg_hba.conf 配置
- 确认防火墙允许5432端口

#### 2. 前端无法连接后端

**症状**: 前端页面报错 "Network Error"

**排查**:
```bash
# 检查后端状态
curl http://localhost:9000/api/v1/health

# 检查端口监听
netstat -tlnp | grep 9000

# 检查Nginx配置
sudo nginx -t
```

**解决**:
- 确认后端服务已启动
- 检查Nginx代理配置
- 确认CORS配置正确

#### 3. WebSocket连接失败

**症状**: 实时数据不更新

**排查**:
- 检查浏览器开发者工具Network标签
- 检查Nginx WebSocket配置
- 检查后端日志

**解决**:
- 确认Nginx配置了WebSocket升级
- 检查防火墙是否拦截WS连接

#### 4. 数据库迁移失败

**症状**: `alembic upgrade head` 报错

**排查**:
```bash
# 查看当前版本
alembic current

# 查看历史
alembic history

# 检查数据库表
\dt
```

**解决**:
- 手动修复: `alembic stamp head`
- 重置: 删除 alembic_version 表后重新执行

### 性能优化

#### 数据库优化

```sql
-- 分析表
ANALYZE;

-- 重建索引
REINDEX DATABASE falconmind;

-- 清理旧遥测数据
DELETE FROM telemetry_history 
WHERE timestamp < NOW() - INTERVAL '3 months';
```

#### Redis优化

```bash
# 监控Redis
redis-cli info
redis-cli monitor

# 清理缓存
redis-cli FLUSHDB
```

### 备份与恢复

#### 数据库备份

```bash
# 手动备份
pg_dump -h localhost -U falconmind falconmind > backup_$(date +%Y%m%d).sql

# 自动备份脚本 (添加到crontab)
0 2 * * * pg_dump -h localhost -U falconmind falconmind | gzip > /backup/falconmind_$(date +\%Y\%m\%d).sql.gz
```

#### 数据库恢复

```bash
# 恢复备份
psql -h localhost -U falconmind falconmind < backup_20260228.sql
```

---

**文档维护:**
- 版本: v1.0
- 更新日期: 2026-02-28
- 维护者: FalconMind Team
