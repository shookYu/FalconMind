# FalconMindViewer 部署配置更新

## 更新内容

### 1. 端口映射
backend 服务现在暴露 9000 端口供外部访问：

```yaml
backend:
  ports:
    - "9000:9000"
```

### 2. 环境变量
新增 ClusterCenter 相关配置：

```yaml
environment:
  # 数据库和Redis (已有)
  DATABASE_URL: postgresql://falconmind:falconmind123@postgres:5432/falconmind
  REDIS_URL: redis://:falconmind123@redis:6379/0
  
  # ClusterCenter 功能开关
  ENABLE_CLUSTER_FEATURES: "true"
  ENABLE_CONFLICT_RESOLUTION: "true"
  ENABLE_AUTO_SCALING: "false"
  
  # 安全参数
  MIN_SEPARATION_DISTANCE: "50.0"  # 最小分离距离(米)
  MIN_ALTITUDE_SEPARATION: "20.0"  # 最小高度分离(米)
  
  # 集群管理
  DEFAULT_CLUSTER_ROLE: "WORKER"
  LEADER_ELECTION_INTERVAL: "300"  # Leader选举间隔(秒)
```

### 3. 健康检查
为 backend 添加健康检查：

```yaml
healthcheck:
  test: ["CMD", "curl", "-f", "http://localhost:9000/api/v1/health"]
  interval: 30s
  timeout: 10s
  retries: 3
  start_period: 40s
```

### 4. 资源限制
新增资源限制配置：

```yaml
deploy:
  resources:
    limits:
      cpus: '2.0'
      memory: 2G
    reservations:
      cpus: '1.0'
      memory: 512M
```

## 更新后的完整 docker-compose.yml

见 docker-compose.prod.yml

## 验证清单

- [ ] PostgreSQL 连接正常
- [ ] Redis 连接正常
- [ ] 所有 API 路由可访问
- [ ] 集群任务创建正常
- [ ] 冲突检测功能正常
- [ ] 集群管理功能正常
- [ ] 遥测数据接收正常
