# Example 30: RTK Precision Positioning

## 验证目标

验证 RTK（实时动态差分）定位技术，通过基站-移动站差分解算，实现厘米级高精度定位。

## 验证内容

1. **RTK 解算** - 载波相位差分解算
2. **基站-移动站通信** - RTCM 数据链路
3. **模糊度固定** - 整周模糊度解算（AR）
4. **定位模式切换** - 单点/浮点解/固定解

## 验证方法

### 软件依赖

```bash
# 必需依赖
- FalconMindSDK
- RTKLIB (可选，用于参考对比)

# 安装 RTKLIB
git clone https://github.com/tomojitakasu/RTKLIB.git
cd RTKLIB/app/consapp/str2str/gcc
make
sudo make install
```

### 硬件依赖

| 设备 | 型号 | 必需 |
|------|------|------|
| 移动站 | u-blox ZED-F9P | 是 |
| 基站 | u-blox ZED-F9P | 是（或使用NTRIP） |
| 天线 | 多频GNSS天线 | 是 |
| 数据链 | 电台/4G模块 | 是 |

### 硬件连接

```
┌────────────────────────────────────────────────────────────┐
│                       系统架构                              │
├────────────────────────────────────────────────────────────┤
│                                                            │
│   ┌───────────┐    RTCM    ┌───────────┐                   │
│   │  基站      │◄──────────►│  移动站    │                   │
│   │  ZED-F9P  │   电台/4G  │  ZED-F9P  │                   │
│   │  (固定)   │            │  (车载)    │                   │
│   └─────┬─────┘            └─────┬─────┘                   │
│         │                        │                         │
│    ┌────┴────┐              ┌────┴────┐                   │
│    │天线1    │              │天线2    │                   │
│    └─────────┘              └─────────┘                   │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

#### ZED-F9P 连接

| 模块 | 连接 | 功能 |
|------|------|------|
| 基站 UART | 电台 TX | 发送 RTCM |
| 移动站 UART | 电台 RX | 接收 RTCM |
| 移动站 USB | PC/RK3588 | 输出定位结果 |

## 编译步骤

```bash
cd FalconMindSDK/examples/30_rtk_precision_positioning/x86
mkdir -p build && cd build
cmake ..
make -j4
```

## 运行步骤

### 方法1: 模拟模式（算法验证）

```bash
./30_rtk_precision_positioning_x86

# 预期输出：
[RTK] Initialization complete
[RTK] Rover: /dev/ttyUSB0
[RTK] Base: ntrip:// caster.centipede.fr:2101/MONT
[RTK] Solution mode: kinematic

[SOL] Single:    Lat: 48.8583°, Lon: 2.2945°, H: 35.2m
[SOL] Float:     Lat: 48.8583°, Lon: 2.2945°, H: 35.1m
[SOL] ✓ Fix:     Lat: 48.858320°, Lon: 2.294482°, H: 35.05m
           Accuracy: H=0.02m, V=0.03m

[Status] Fix rate: 95% | Satellites: 18 | AR validation: 12/12
```

### 方法2: NTRIP 网络基站

```bash
# 使用公共 NTRIP 基站
./30_rtk_precision_positioning_x86 \
    --rover /dev/ttyUSB0 \
    --ntrip caster.centipede.fr \
    --port 2101 \
    --mountpoint MONT \
    --user user:pass
```

### 方法3: 本地基站

```bash
# 基站配置
./30_rtk_precision_positioning_x86 \
    --base /dev/ttyUSB1 \
    --rover /dev/ttyUSB0 \
    --base-pos 48.8583,2.2945,35.0 \
    --radio /dev/ttyS0
```

## 期望结果

### 定位精度
| 模式 | 水平精度 | 垂直精度 | 初始化时间 |
|------|----------|----------|------------|
| Single | 2-3m | 3-5m | 即时 |
| Float | 0.5-2m | 1-3m | 10-30s |
| Fixed | 1-3cm | 2-5cm | 30-120s |

### 性能指标
- 更新率：10Hz
- 延迟：<100ms
- 固定率：>95%（开阔环境）

## 故障排除

**问题**: 无法固定模糊度
```bash
# 检查卫星数量
./30_rtk_precision_positioning_x86 --debug
# 需要 ≥8 颗共同卫星

# 检查基线长度
# 推荐 <10km
```

**问题**: NTRIP 连接失败
```bash
# 检查网络
ping caster.centipede.fr

# 检查挂载点
./30_rtk_precision_positioning_x86 --list-mountpoints
```

## 参考文档

- [RTKLIB Manual](https://rtklib.com/)
- [ZED-F9P Integration](https://www.u-blox.com/en/product/zed-f9p-module)
- [Centipede NTRIP](https://docs.centipede.fr/)
