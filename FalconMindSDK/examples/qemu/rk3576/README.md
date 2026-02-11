# RK3576 QEMU Virtual Machine

本目录包含用于在x86开发机上模拟RK3576 ARM64环境的QEMU虚拟机配置。

## 系统规格

- **架构**: ARM64 (aarch64)
- **OS**: Ubuntu 22.04 LTS (Jammy Jellyfish)
- **CPU**: 4 cores
- **内存**: 4GB
- **磁盘**: 64GB

## 前置要求

### 1. 安装QEMU
```bash
# Ubuntu/Debian
sudo apt-get install qemu-system-aarch64 qemu-efi-aarch64 sgabios

# Fedora
sudo dnf install qemu-system-aarch64
```

### 2. 安装虚拟化支持 (可选，但推荐)
```bash
# 启用KVM支持
sudo modprobe kvm
sudo modprobe kvm_intel  # Intel CPU
# 或
sudo modprobe kvm_arm    # ARM CPU (如果运行在ARM主机上)
```

### 3. 安装依赖
```bash
sudo apt-get install \
    build-essential \
    cmake \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    libssl-dev \
    wget \
    unzip
```

## 使用方法

### 方法1: 使用Vagrant (推荐)

```bash
cd examples/qemu/rk3576
vagrant up
vagrant ssh
```

### 方法2: 直接运行QEMU

```bash
# 启动虚拟机
./start.sh

# 另开终端连接到虚拟机
./exec.sh "bash"

# 或使用SSH
ssh -p 2222 falcon@localhost
# 密码: falcon
```

## 快速开始

1. **启动虚拟机**:
```bash
cd examples/qemu/rk3576
./start.sh
```

2. **连接到虚拟机** (新终端):
```bash
./exec.sh
```

3. **在虚拟机中设置环境**:
```bash
# 安装SDK依赖
./setup.sh

# 或手动执行
cd ~/examples
bash build.sh rk3576
bash run-tests.sh rk3576
```

## 目录结构

```
qemu/rk3576/
├── start.sh           # 启动QEMU虚拟机
├── setup.sh           # 环境设置脚本
├── exec.sh            # 在虚拟机中执行命令
├── user-data          # Cloud-init配置
├── meta-data          # 实例元数据
├── Vagrantfile        # Vagrant配置
└── README.md          # 本文档
```

## 共享文件夹

虚拟机自动挂载以下目录:
- `~/examples` -> `<repo>/examples` (examples代码)
- `~/sdk` -> `<repo>` (SDK根目录)

## 工具链

交叉编译工具链已预装在:
```
/opt/FriendlyARM/toolchain/11.3-aarch64/
```

## 常见问题

### Q: 虚拟机启动很慢
A: 首次启动需要下载Ubuntu镜像，后续启动会快很多。

### Q: 无法连接到虚拟机
A: 检查SSH服务是否启动: `systemctl status ssh`

### Q: 编译速度慢
A: 确保开启了KVM加速，并分配足够的CPU核心。

### Q: 磁盘空间不足
A: 扩展磁盘或清理不需要的文件:
```bash
sudo apt-get autoremove
sudo apt-get autoclean
```

## 停止虚拟机

```bash
# 在虚拟机内
sudo poweroff

# 或从主机
pkill -f "qemu-system-aarch64.*rk3576"
```

## 资源

- QEMU文档: https://www.qemu.org/documentation/
- Ubuntu ARM: https://ubuntu.com/download/server/arm
