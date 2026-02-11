# FalconMindSDK QEMU虚拟化测试框架

本文档描述如何使用QEMU虚拟机在x86开发机上模拟ARM64平台，进行FalconMindSDK的编译和测试。

---

## 目录

- [快速开始](#快速开始)
- [平台配置](#平台配置)
- [使用方法](#使用方法)
  - [方法一：启动QEMU后手动操作](#方法一启动qemu后手动操作)
  - [方法二：使用自动化脚本](#方法二使用自动化脚本)
- [SSH连接](#ssh连接)
- [板卡部署](#板卡部署)
- [常见问题](#常见问题)
- [资源规格](#资源规格)

---

## 快速开始

### 前置要求

```bash
# 安装QEMU
sudo apt-get update
sudo apt-get install -y \
    qemu-system-aarch64 \
    qemu-efi-aarch64 \
    wget \
    git

# 安装SSH客户端（用于连接虚拟机）
sudo apt-get install -y \
    openssh-client \
    sshpass
```

### 最简流程（以RK3576为例）

```bash
# 1. 启动QEMU虚拟机（首次会自动下载镜像，约1-2GB）
cd examples/qemu/rk3576
./start.sh

# 2. 新开终端，连接到虚拟机
./exec.sh

# 3. 在虚拟机内执行设置和测试
./setup.sh --all
```

---

## 平台配置

| 平台 | CPU架构 | CPU核心 | 内存 | 磁盘 | SSH端口 | OS版本 |
|------|---------|--------|------|------|---------|--------|
| **RK3576** | Cortex-A55×4 + Cortex-A76×4 (8核) | 8 | 8GB | 64GB | 2222 | Ubuntu 22.04 |
| **RK3588** | Cortex-A76×4 + Cortex-A55×4 (8核) | 8 | 8GB | 64GB | 2223 | Ubuntu 22.04 |
| **RV1126B** | Cortex-A53×4 (4核) | 4 | 4GB | 32GB | 2224 | Ubuntu 20.04 |
| **x86** | x86-64 (本地) | 4 | 4GB | 64GB | - | 本地系统 |

### 硬件规格说明

- **RK3576**: 8核big.LITTLE架构 (4×Cortex-A76 + 4×Cortex-A55)，6 TOPS NPU
- **RK3588**: 8核big.LITTLE架构 (4×Cortex-A76 + 4×Cortex-A55)，6 TOPS NPU
- **RV1126B**: 4核Cortex-A53，3.0 TOPS NPU

---

## 使用方法

### 方法一：启动QEMU后手动操作

#### 步骤1：启动QEMU虚拟机

```bash
# RK3576
cd examples/qemu/rk3576
./start.sh

# RK3588
cd examples/qemu/rk3588
./start.sh

# RV1126B
cd examples/qemu/rv1126b
./start.sh
```

启动后显示：
```
========================================
  RK3576 QEMU ARM64 Virtual Machine
========================================
内存: 8192MB, CPU: 8核
SSH: localhost:2222
用户: falcon, 密码: falcon
按 Ctrl+A, X 退出
```

**注意**: 按 `Ctrl+A, X` 可以退出QEMU（而不是Ctrl+C）

#### 步骤2：连接到虚拟机（新终端）

```bash
# 交互式shell
./exec.sh

# 或直接执行命令
./exec.sh "ls -la"

# 或SSH直接连接
ssh -p 2222 falcon@localhost
# 密码: falcon
```

#### 步骤3：在虚拟机内设置和测试

```bash
# 完整设置（安装依赖、工具链、编译、测试）
./setup.sh --all

# 或分步执行
./setup.sh --deps      # 仅安装依赖
./setup.sh --toolchain # 仅安装工具链
./setup.sh --build     # 仅编译examples
./setup.sh --test      # 仅运行测试
```

---

### 方法二：使用自动化脚本

#### 完整流程（一键执行）

```bash
# 在主机上执行，自动启动QEMU、连接、安装、编译、测试
./run-qemu-test.sh rk3576 --all
```

#### 仅编译

```bash
./run-qemu-test.sh rk3576 --build
```

#### 仅运行测试

```bash
./run-qemu-test.sh rk3576 --run
```

---

## SSH连接

### 虚拟机默认凭据

| 项目 | 值 |
|------|-----|
| 用户名 | falcon |
| 密码 | falcon |
| SSH端口 | 见上表 |

### 连接示例

```bash
# RK3576
ssh -p 2222 falcon@localhost

# RK3588
ssh -p 2223 falcon@localhost

# RV1126B
ssh -p 2224 falcon@localhost

# 使用sshpass（免输入密码）
sshpass -p "falcon" ssh -p 2222 falcon@localhost
```

### 免密设置（可选）

```bash
# 生成SSH密钥（如果还没有）
ssh-keygen -t rsa

# 复制到虚拟机
sshpass -p "falcon" ssh-copy-id -p 2222 falcon@localhost
```

---

## 板卡部署

### 配置板卡信息

编辑 `examples/upload.sh` 顶部的板卡配置：

```bash
# 板卡IP地址
BOARD_IPS=(
    ["rk3576"]="192.168.1.100"   # 修改为实际IP
    ["rk3588"]="192.168.1.101"
    ["rv1126b"]="192.168.1.102"
)

# 板卡密码
BOARD_PASSWORDS=(
    ["rk3576"]="your_password"    # 修改为实际密码
    ["rk3588"]="your_password"
    ["rv1126b"]="your_password"
)

# 板卡用户名
BOARD_USERS=(
    ["rk3576"]="root"            # 或其他用户名
    ["rk3588"]="root"
    ["rv1126b"]="root"
)
```

### 部署命令

```bash
# 上传到板卡并编译运行
./upload.sh rk3576 --build --run

# 仅同步文件
./upload.sh rk3576 --sync

# 完整流程（同步+编译+运行）
./upload.sh rk3576 --all

# 所有平台
./upload.sh x86 --all
./upload.sh rk3588 --all
./upload.sh rv1126b --all
```

### QEMU模式部署

```bash
# 启动RK3576 QEMU并执行完整测试
./upload.sh rk3576 --qemu --all
```

---

## 常见问题

### Q1: QEMU启动报错"BIOS not found"

```bash
# 安装QEMU EFI固件
sudo apt-get install -y qemu-efi-aarch64

# 检查固件位置
ls /usr/share/qemu-efi-aarch64/QEMU_EFI.fd
```

### Q2: 编译速度慢

```bash
# 确保开启了KVM加速
lsmod | grep kvm

# 如果没有KVM，QEMU会使用软件模拟，速度较慢
# 在BIOS中启用虚拟化技术(VT-x/AMD-V)
```

### Q3: 虚拟机无法联网

```bash
# 检查网络配置
ping google.com

# 重启网络服务
sudo systemctl restart NetworkManager
```

### Q4: 磁盘空间不足

```bash
# 扩展磁盘（需要重新创建）
# 或清理不需要的文件
sudo apt-get autoremove
sudo apt-get autoclean
```

### Q5: 无法连接到虚拟机SSH

```bash
# 检查SSH服务是否运行
sudo systemctl status ssh

# 手动启动
sudo systemctl start ssh

# 检查防火墙
sudo ufw status
```

### Q6: 共享目录不生效

```bash
# 确保QEMU启动时使用了virtfs选项
# 检查挂载点
ls /host_share/

# 手动挂载
sudo mkdir -p /host_share
sudo mount -t 9p -o trans=virtio,version=9p2000.L host_share /host_share
```

---

## 资源规格

### 开发机要求

| 项目 | 最低要求 | 推荐配置 |
|------|---------|---------|
| CPU | 支持虚拟化的x86-64 | Intel VT-x / AMD-V |
| 内存 | 8GB | 16GB+ |
| 磁盘 | 20GB | 50GB+ |
| OS | Ubuntu 20.04/22.04 | Ubuntu 22.04 LTS |

### 虚拟机资源配置

根据目标平台选择合适的资源配置：

- **RV1126B** (4核/4GB): 轻量级测试，资源占用小
- **RK3576/RK3588** (8核/8GB): 完整测试，支持并行编译

### 编译时间参考

| 平台 | 首次编译 | 增量编译 |
|------|---------|---------|
| QEMU (有KVM) | 5-10分钟 | 10-30秒 |
| QEMU (无KVM) | 30-60分钟 | 5-10分钟 |
| 物理板卡 | 2-5分钟 | 10-20秒 |

---

## 目录结构

```
examples/
├── upload.sh              # 板卡/QEMU部署主脚本
├── build.sh               # 编译脚本
├── run-tests.sh          # 测试运行脚本
├── run-qemu-test.sh      # QEMU自动化测试脚本
└── qemu/
    ├── x86/
    │   ├── start.sh       # 启动x86 QEMU
    │   ├── exec.sh        # SSH连接
    │   └── README.md
    ├── rk3576/
    │   ├── start.sh       # 启动RK3576 QEMU
    │   ├── setup.sh       # 环境设置（虚拟机内执行）
    │   ├── exec.sh        # SSH连接
    │   └── README.md
    ├── rk3588/
    │   ├── start.sh       # 启动RK3588 QEMU
    │   ├── setup.sh       # 环境设置（虚拟机内执行）
    │   ├── exec.sh        # SSH连接
    │   └── README.md
    └── rv1126b/
        ├── start.sh       # 启动RV1126B QEMU
        ├── setup.sh       # 环境设置（虚拟机内执行）
        ├── exec.sh        # SSH连接
        └── README.md
```

---

## 快捷命令速查

### 本地编译（x86）

```bash
cd examples
./build.sh x86              # 编译所有
./run-tests.sh x86          # 运行测试
```

### QEMU测试

```bash
./run-qemu-test.sh rk3576 --all  # 完整流程
./run-qemu-test.sh rk3588 --build # 仅编译
./run-qemu-test.sh rv1126b --run  # 仅运行
```

### 板卡部署

```bash
./upload.sh rk3576 --all    # 上传+编译+运行
./upload.sh rk3588 --sync   # 仅同步
./upload.sh rv1126b --qemu  # QEMU模式
```

---

## 注意事项

1. **首次启动**: 首次运行QEMU会自动下载Ubuntu cloud镜像（约500MB-1GB）
2. **内存分配**: 确保开发机有足够内存（推荐16GB+）
3. **KVM加速**: 强烈建议启用KVM，可大幅提升编译速度
4. **SSH连接**: 使用sshpass可以免输入密码
5. **退出QEMU**: 使用 `Ctrl+A, X` 退出，而不是 `Ctrl+C`

---

## 技术支持

如有问题，请检查：

1. QEMU安装：`qemu-system-aarch64 --version`
2. KVM支持：`lsmod | grep kvm`
3. 磁盘空间：`df -h`
4. 内存使用：`free -h`
