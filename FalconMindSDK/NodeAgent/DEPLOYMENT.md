# FalconMind NodeAgent 部署指南

## 目录

- [快速开始](#快速开始)
- [系统要求](#系统要求)
- [安装方式](#安装方式)
  - [Docker部署（推荐）](#docker部署推荐)
  - [Systemd部署](#systemd部署)
  - [源码编译安装](#源码编译安装)
- [配置说明](#配置说明)
- [验证安装](#验证安装)
- [故障排除](#故障排除)
- [监控与日志](#监控与日志)

## 快速开始

### 一键安装脚本

```bash
# 下载安装脚本
curl -fsSL https://raw.githubusercontent.com/your-org/FalconMind/main/install.sh | sudo bash

# 或使用 wget
wget -qO- https://raw.githubusercontent.com/your-org/FalconMind/main/install.sh | sudo bash
```

## 系统要求

### 硬件要求
- **CPU**: ARM Cortex-A72 / x86_64 (2+ cores)
- **内存**: 512MB RAM (推荐 1GB)
- **存储**: 1GB 可用空间
- **网络**: 4G/5G/WiFi/Ethernet

### 软件要求
- **操作系统**: Ubuntu 20.04/22.04, Debian 11/12, Raspberry Pi OS
- **内核**: Linux 5.4+
- **依赖**: SQLite3, OpenSSL, zlib

### 支持平台
- ✅ x86_64 (Intel/AMD)
- ✅ ARM64 (Raspberry Pi 4, Jetson Nano/Orin)
- ✅ ARMv7 (Raspberry Pi 3)
- ⚠️ ARM32 (Raspberry Pi Zero 2W, 有限支持)

## 安装方式

### Docker部署（推荐）

#### 1. 安装Docker

```bash
# Ubuntu/Debian
curl -fsSL https://get.docker.com | sudo sh

# 或手动安装
sudo apt-get update
sudo apt-get install -y docker.io docker-compose

# 添加用户到docker组
sudo usermod -aG docker $USER
```

#### 2. 克隆仓库

```bash
git clone https://github.com/your-org/FalconMind.git
cd FalconMind/FalconMindSDK/NodeAgent
```

#### 3. 配置

```bash
# 复制配置模板
mkdir -p config
cp config/config.example.yaml config/config.yaml

# 编辑配置
nano config/config.yaml
```

#### 4. 启动服务

```bash
# 启动NodeAgent
docker-compose up -d

# 查看日志
docker-compose logs -f nodeagent

# 带监控启动（Prometheus + Grafana）
docker-compose --profile monitoring up -d
```

### Systemd部署

#### 1. 编译安装

```bash
# 安装依赖
sudo apt-get update
sudo apt-get install -y build-essential cmake git sqlite3 libsqlite3-dev libssl-dev

# 编译
cd FalconMind/FalconMindSDK/NodeAgent
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 安装
sudo make install
```

#### 2. 安装服务

```bash
cd ../systemd
sudo ./install.sh
```

#### 3. 管理服务

```bash
# 启动服务
sudo systemctl start nodeagent

# 查看状态
sudo systemctl status nodeagent

# 查看日志
sudo journalctl -u nodeagent -f

# 设置开机自启
sudo systemctl enable nodeagent
```

### 源码编译安装

```bash
# 完整手动安装
cd FalconMind/FalconMindSDK/NodeAgent

# 创建构建目录
mkdir -p build && cd build

# 配置
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DNODEAGENT_USE_MQTT=OFF

# 编译
make -j$(nproc)

# 运行测试
make test

# 安装
sudo make install
sudo ldconfig

# 创建用户
sudo useradd -r -s /bin/false nodeagent

# 创建目录
sudo mkdir -p /etc/nodeagent /var/lib/nodeagent /var/log/nodeagent
sudo chown nodeagent:nodeagent /var/lib/nodeagent /var/log/nodeagent
```

## 配置说明

### 配置文件位置

- **Docker**: `./config/config.yaml` (挂载到 `/etc/nodeagent/config.yaml`)
- **Systemd**: `/etc/nodeagent/config.yaml`
- **源码**: `/usr/local/etc/nodeagent/config.yaml`

### 主要配置项

```yaml
uav:
  id: "UAV_001"              # UAV唯一标识
  name: "Alpha"              # 显示名称
  type: "quadcopter"         # 类型: quadcopter/fixed_wing/helicopter

gcs:
  host: "192.168.1.100"      # 地面站IP
  port: 8080                 # 地面站端口
  heartbeat_interval_ms: 1000  # 心跳间隔
  heartbeat_timeout_ms: 5000   # 心跳超时

swarm:
  enabled: true              # 启用集群功能
  swarm_id: "SWARM_001"      # 集群ID
  discovery_interval_ms: 5000  # 发现间隔
  heartbeat_interval_ms: 1000  # 机间心跳间隔
  heartbeat_timeout_ms: 5000   # 机间心跳超时

autonomy:
  enabled: true              # 启用离线自治
  heartbeat_timeout_seconds: 10     # GCS心跳超时
  max_offline_duration_minutes: 30  # 最大离线时间
  low_battery_threshold: 30         # 低电量阈值(%)
  critical_battery_threshold: 15    # 临界电量阈值(%)

storage:
  db_path: "/var/lib/nodeagent/offline.db"
  max_telemetry_buffer: 1000   # 遥测缓存大小
  max_task_history: 100        # 任务历史保留数

logging:
  level: "INFO"              # 日志级别: DEBUG/INFO/WARN/ERROR
  output: "/var/log/nodeagent"
  max_file_size: 100         # MB
  max_files: 10              # 保留文件数
  format: "json"             # 格式: json/text

metrics:
  enabled: true              # 启用指标收集
  export_interval_seconds: 30
  prometheus_port: 9090      # Prometheus端口
```

## 验证安装

### 1. 检查服务状态

```bash
# Docker
docker-compose ps
docker-compose logs nodeagent

# Systemd
sudo systemctl status nodeagent
sudo journalctl -u nodeagent -f
```

### 2. 健康检查

```bash
# Docker
docker-compose exec nodeagent nodeagent-health

# Systemd
sudo /usr/local/bin/nodeagent-health
```

### 3. 查看状态报告

```bash
# Docker
docker-compose exec nodeagent nodeagent-status

# Systemd
sudo /usr/local/bin/nodeagent-status
```

### 4. 测试API

```bash
# 检查指标端点
curl http://localhost:9090/metrics

# 检查健康状态
curl http://localhost:8080/health
```

### 5. 运行单元测试

```bash
# Docker
docker-compose exec nodeagent nodeagent_unit_tests

# Systemd
sudo /usr/local/bin/nodeagent_unit_tests
```

## 故障排除

### 服务无法启动

```bash
# 检查日志
sudo journalctl -u nodeagent -n 100 --no-pager

# 检查配置文件语法
sudo /usr/local/bin/nodeagent_demo --check-config

# 验证权限
ls -la /var/lib/nodeagent /var/log/nodeagent /etc/nodeagent
```

### 数据库错误

```bash
# 检查数据库
sqlite3 /var/lib/nodeagent/offline.db ".tables"

# 修复数据库（如有损坏）
sqlite3 /var/lib/nodeagent/offline.db ".recover" | sqlite3 /var/lib/nodeagent/offline.db.fixed
mv /var/lib/nodeagent/offline.db /var/lib/nodeagent/offline.db.bak
mv /var/lib/nodeagent/offline.db.fixed /var/lib/nodeagent/offline.db
sudo chown nodeagent:nodeagent /var/lib/nodeagent/offline.db
```

### 串口权限问题

```bash
# 添加用户到dialout组
sudo usermod -a -G dialout $USER

# 或设置udev规则
echo 'SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", MODE="0666"' | \
  sudo tee /etc/udev/rules.d/50-usb-serial.rules
sudo udevadm control --reload-rules
```

### 内存不足

```bash
# 检查内存使用
docker stats nodeagent
# 或
ps aux | grep nodeagent

# 调整限制
# docker-compose.yml:
# deploy:
#   resources:
#     limits:
#       memory: 1G
```

### 网络连接问题

```bash
# 检查网络连接
ping -c 3 <GCS_IP>
curl -v http://<GCS_IP>:<PORT>/health

# 检查防火墙
sudo iptables -L -n | grep <PORT>
sudo ufw status

# 查看路由
ip route
```

## 监控与日志

### 日志查看

```bash
# 实时日志
# Docker
docker-compose logs -f nodeagent

# Systemd
sudo journalctl -u nodeagent -f

# 日志文件
sudo tail -f /var/log/nodeagent/*.log
```

### Prometheus监控

```bash
# 访问Prometheus UI
open http://localhost:9091

# 查询示例
nodeagent_telemetry_buffer_size
nodeagent_tasks_completed_total
```

### Grafana仪表板

```bash
# 访问Grafana
open http://localhost:3000
# 默认用户名: admin
# 默认密码: admin
```

### 关键指标

- `nodeagent_uav_state`: UAV当前状态
- `nodeagent_gcs_connected`: GCS连接状态
- `nodeagent_telemetry_buffer_size`: 遥测缓冲区大小
- `nodeagent_offline_tasks_total`: 离线任务数
- `nodeagent_swarm_partition_count`: 集群分区数
- `nodeagent_connection_quality`: 连接质量评分

## 升级

### Docker升级

```bash
# 拉取最新镜像
docker-compose pull

# 重新部署
docker-compose up -d

# 清理旧镜像
docker image prune -f
```

### Systemd升级

```bash
# 停止服务
sudo systemctl stop nodeagent

# 重新编译安装
cd FalconMind/FalconMindSDK/NodeAgent/build
make clean
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install

# 重启服务
sudo systemctl start nodeagent
```

## 卸载

```bash
# Docker
docker-compose down -v
docker image rm falconmind-nodeagent:latest

# Systemd
cd FalconMind/FalconMindSDK/NodeAgent/systemd
sudo ./install.sh uninstall

# 清理数据（可选）
sudo rm -rf /etc/nodeagent /var/lib/nodeagent /var/log/nodeagent
```

## 支持

- **文档**: https://github.com/your-org/FalconMind/docs
- **Issues**: https://github.com/your-org/FalconMind/issues
- **邮件**: support@falconmind.ai

## License

Apache License 2.0
