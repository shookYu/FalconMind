# FalconMindSDK 架构改进建议：视频流转与多模块通信

## 1. 当前架构问题分析

### 1.1 视频流问题
- **现状**: 视频数据在内存中直接传递，缺乏标准化流转机制
- **问题**: 
  - 内存拷贝开销大
  - 不支持远程监控
  - 多模块共享视频困难
  - 没有回放/录制能力

### 1.2 模块通信问题
- **现状**: 模块间通过共享内存/直接调用耦合
- **问题**:
  - 模块无法独立部署
  - 单点故障影响全局
  - 难以水平扩展
  - 调试困难

### 1.3 进程架构问题
- **现状**: 所有功能集中在单一进程
- **问题**:
  - 一个模块崩溃导致全局失效
  - 无法单独升级/重启某个模块
  - 资源竞争严重

---

## 2. 目标架构设计

### 2.1 整体架构图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           边缘侧 (UAV - RK3588)                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      视频流转层 (MediaTx + RTSP)                     │   │
│  │                                                                     │   │
│  │   ┌─────────────┐     ┌─────────────┐     ┌─────────────────────┐  │   │
│  │   │  Camera     │────▶│  MediaTx    │────▶│  RTSP Server        │  │   │
│  │   │  (V4L2)     │     │  (GStreamer)│     │  (多路流)            │  │   │
│  │   └─────────────┘     └─────────────┘     └─────────────────────┘  │   │
│  │                              │                                      │   │
│  │                              ▼                                      │   │
│  │   ┌─────────────────────────────────────────────────────────────┐   │   │
│  │   │                    视频流类型                                 │   │   │
│  │   │  • /live/camera - 原始视频流                                 │   │   │
│  │   │  • /live/detected - 检测框叠加流                            │   │   │
│  │   │  • /live/tracked - 跟踪轨迹叠加流                           │   │   │
│  │   │  • /live/thermal - 热成像流 (如果有)                         │   │   │
│  │   └─────────────────────────────────────────────────────────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      通信总线层 (MQTT + DDS)                         │   │
│  │                                                                     │   │
│  │   ┌──────────────────────────────────────────────────────────┐    │   │
│  │   │  MQTT Broker (Mosquitto / NanoMQ)                        │    │   │
│  │   │  ─────────────────────────────────────────────────────   │    │   │
│  │   │  Topics:                                                   │    │   │
│  │   │  • /falconmind/{uav_id}/telemetry/position               │    │   │
│  │   │  • /falconmind/{uav_id}/telemetry/attitude             │    │   │
│  │   │  • /falconmind/{uav_id}/telemetry/battery              │    │   │
│  │   │  • /falconmind/{uav_id}/detection/targets              │    │   │
│  │   │  • /falconmind/{uav_id}/tracking/tracks                │    │   │
│  │   │  • /falconmind/{uav_id}/guidance/command               │    │   │
│  │   │  • /falconmind/{uav_id}/navigation/gps_defense         │    │   │
│  │   │  • /falconmind/{uav_id}/mission/status                 │    │   │
│  │   │  • /falconmind/{uav_id}/system/health                  │    │   │
│  │   └──────────────────────────────────────────────────────────┘    │   │
│  │                                                                     │   │
│  │   ┌──────────────────────────────────────────────────────────┐    │   │
│  │   │  DDS Domain (Fast DDS / Cyclone DDS)                     │    │   │
│  │   │  ─────────────────────────────────────────────────────   │    │   │
│  │   │  用于高带宽、低延迟、高可靠性的模块间通信                  │    │   │
│  │   │  Topics:                                                   │    │   │
│  │   │  • DetectionArray (实时检测结果)                          │    │   │
│  │   │  • TrackingArray (实时跟踪结果)                           │    │   │
│  │   │  • ImageFrame (视频帧元数据，不传输实际像素)               │    │   │
│  │   │  • GuidanceCommand (制导指令)                             │    │   │
│  │   │  • NavigationState (导航状态)                             │    │   │
│  │   └──────────────────────────────────────────────────────────┘    │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      业务进程层 (独立进程)                           │   │
│  │                                                                     │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌─────────┐ │   │
│  │  │   Video      │  │  Perception  │  │   Guidance   │  │  GPS    │ │   │
│  │  │   Capture    │  │   Pipeline   │  │   Control    │  │ Defense │ │   │
│  │  │   Process    │  │   Process    │  │   Process    │  │ Process │ │   │
│  │  │  (进程1)      │  │  (进程2)      │  │  (进程3)      │  │(进程4)  │ │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘  └─────────┘ │   │
│  │         │                 │                 │               │        │   │
│  │         │                 │                 │               │        │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌─────────┐ │   │
│  │  │   Mission    │  │   VINS       │  │   Flight     │  │  Logger │ │   │
│  │  │   Planner    │  │   SLAM       │  │   Control    │  │ Process │ │   │
│  │  │   Process    │  │   Process    │  │   Process    │  │ (进程8) │ │   │
│  │  │  (进程5)      │  │  (进程6)      │  │  (进程7)      │  └─────────┘ │   │
│  │  └──────────────┘  └──────────────┘  └──────────────┘              │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                      进程管理器 (SupervisorD)                        │   │
│  │                                                                     │   │
│  │   • 进程启动/停止/重启                                              │   │
│  │   • 健康检查                                                        │   │
│  │   • 自动恢复                                                        │   │
│  │   • 资源监控                                                        │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

                                    │
                                    │ WebRTC / MQTT
                                    ▼

┌─────────────────────────────────────────────────────────────────────────────┐
│                           地面站 (Web Browser)                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │  FalconMindViewer (Vue3 + WebRTC)                                   │   │
│  │                                                                     │   │
│  │   • 视频播放 (WebRTC Player)                                        │   │
│  │   • 实时遥测                                                        │   │
│  │   • 目标显示                                                        │   │
│  │   • 任务控制                                                        │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. 详细设计方案

### 3.1 视频流转方案 (MediaTx + RTSP + WebRTC)

#### 3.1.1 为什么选择这个方案？

| 方案 | 优点 | 缺点 | 适用场景 |
|------|------|------|----------|
| **MediaTx + RTSP** | 成熟、低延迟、支持多路 | 需要转WebRTC | 边缘到边缘 |
| **WebRTC原生** | 浏览器直接播放 | 复杂、资源占用大 | 浏览器直连 |
| **RTMP** | 成熟 | 延迟高、已过时 | 不推荐使用 |
| **HLS/DASH** | 兼容性好 | 延迟高(3-10s) | 录播场景 |

**推荐**: MediaTx (GStreamer) 提供 RTSP 流，通过 SFU (mediasoup/Janus) 转 WebRTC 给浏览器

#### 3.1.2 视频流设计

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         视频流架构 (Media Pipeline)                          │
└─────────────────────────────────────────────────────────────────────────────┘

Video Source (V4L2)
       │
       ▼
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   v4l2src       │────▶│   tee           │────▶│   queue         │
│   (原始视频)     │     │   (分流)        │     │                 │
└─────────────────┘     └─────────────────┘     └─────────────────┘
                                                       │
       ┌───────────────────────────────────────────────┼──────────────────┐
       │                                               │                  │
       ▼                                               ▼                  ▼
┌─────────────────┐                          ┌─────────────────┐  ┌─────────────────┐
│   分支1: 编码    │                          │   分支2: AI处理  │  │   分支3: 录制   │
│                 │                          │                 │  │                 │
│  videoconvert   │                          │  videoscale     │  │  mp4mux         │
│  └─▶ x264enc    │                          │  └─▶ rknn       │  │  └─▶ filesink  │
│       └─▶       │                          │       └─▶       │  │                 │
│  rtspclientsink │                          │  detection      │  │  (本地存储)      │
│  (/live/camera) │                          │  overlay        │  │                 │
│                 │                          │       │         │  └─────────────────┘
└─────────────────┘                          │       ▼         │
                                             │  textoverlay    │
                                             │  bbox绘制       │
                                             │       │         │
                                             │       ▼         │
                                             │  rtspclientsink │
                                             │  (/live/detected)
                                             │                 │
                                             └─────────────────┘
```

#### 3.1.3 GStreamer Pipeline 示例

```bash
# 原始视频流 (H.264)
gst-launch-1.0 v4l2src device=/dev/video0 ! \
    video/x-raw,format=NV12,width=1920,height=1080,framerate=30/1 ! \
    videoconvert ! x264enc tune=zerolatency bitrate=4000 ! \
    rtspclientsink location=rtsp://localhost:8554/live/camera

# 检测叠加流
gst-launch-1.0 v4l2src device=/dev/video0 ! \
    video/x-raw,format=RGB,width=640,height=480 ! \
    tee name=t ! \
    queue ! \
    videoscale ! video/x-raw,width=416,height=416 ! \
    appsink name=ai_input t. ! \
    queue ! \
    textoverlay text="Detection Stream" ! \
    x264enc tune=zerolatency ! \
    rtspclientsink location=rtsp://localhost:8554/live/detected

# MediaTx 服务启动
mediamtx /etc/mediamtx.yml
```

#### 3.1.4 WebRTC 转换 (Janus Gateway)

```yaml
# janus.plugin.streaming.jcfg
stream-1: {
    type = "rtsp"
    id = 1
    description = "Camera Live Stream"
    audio = false
    video = true
    url = "rtsp://localhost:8554/live/camera"
    rtsp_user = ""
    rtsp_pwd = ""
}

stream-2: {
    type = "rtsp"
    id = 2
    description = "Detection Overlay Stream"
    audio = false
    video = true
    url = "rtsp://localhost:8554/live/detected"
}
```

---

### 3.2 模块间通信方案 (MQTT + DDS)

#### 3.2.1 为什么双总线？

| 特性 | MQTT | DDS |
|------|------|-----|
| **延迟** | ms级 | μs级 |
| **吞吐量** | 中等 | 高 |
| **QoS** | 3级 | 23级组合 |
| **发现** | 中央 broker | 分布式 |
| **资源** | 低 | 中等 |
| **适用** | 控制指令、遥测 | 实时数据、视频元数据 |

**推荐**: 
- **MQTT**: 控制指令、配置、状态、慢速遥测
- **DDS**: 检测结果、跟踪数据、制导命令、高频率数据

#### 3.2.2 MQTT 主题设计

```
falconmind/
├── {uav_id}/
│   ├── telemetry/
│   │   ├── position          (GPS位置 10Hz)
│   │   ├── attitude          (姿态 50Hz)
│   │   ├── velocity          (速度 20Hz)
│   │   ├── battery           (电量 1Hz)
│   │   └── system            (系统状态 1Hz)
│   │
│   ├── detection/
│   │   ├── targets           (目标列表，有目标时发布)
│   │   ├── events            (检测事件：new/lost/update)
│   │   └── stats             (统计信息 1Hz)
│   │
│   ├── tracking/
│   │   ├── tracks            (跟踪轨迹)
│   │   └── events            (跟踪事件)
│   │
│   ├── guidance/
│   │   ├── command           (制导指令输出)
│   │   ├── status            (制导状态)
│   │   └── target            (当前跟踪目标)
│   │
│   ├── navigation/
│   │   ├── gps_defense       (GPS防护状态)
│   │   ├── vins              (视觉定位状态)
│   │   └── mode              (导航模式：GPS/VINS/融合)
│   │
│   ├── mission/
│   │   ├── status            (任务状态)
│   │   ├── progress          (任务进度)
│   │   ├── waypoints         (当前航点)
│   │   └── command           (任务控制：start/pause/resume/abort)
│   │
│   ├── system/
│   │   ├── health            (健康状态)
│   │   ├── log               (日志)
│   │   └── config            (配置更新)
│   │
│   └── video/
│       ├── streams           (可用视频流列表)
│       └── webrtc/offer      (WebRTC 信令)
│
└── broadcast/
    ├── discovery             (设备发现)
    └── time_sync             (时间同步)
```

#### 3.2.3 DDS 主题设计 (IDL)

```cpp
// DetectionArray.idl
struct Detection {
    uint64 timestamp_ns;
    int32 track_id;
    float bbox[4];        // x, y, width, height (normalized)
    float confidence;
    string class_name;
    float distance;       // estimated distance in meters
};

struct DetectionArray {
    uint64 timestamp_ns;
    string frame_id;
    sequence<Detection> detections;
};

// TrackingArray.idl
struct Track {
    int32 track_id;
    uint64 first_seen_ns;
    uint64 last_seen_ns;
    float bbox[4];
    float velocity[2];    // vx, vy in pixel/s
    uint32 hit_count;
    uint32 miss_count;
    boolean is_confirmed;
};

struct TrackingArray {
    uint64 timestamp_ns;
    sequence<Track> tracks;
};

// GuidanceCommand.idl
struct GuidanceCommand {
    uint64 timestamp_ns;
    uint8 mode;           // 0=position, 1=velocity, 2=attitude
    float vx;             // m/s
    float vy;             // m/s
    float vz;             // m/s
    float yaw_rate;       // rad/s
    float target_distance; // desired distance to target
    boolean valid;
};

// NavigationState.idl
struct NavigationState {
    uint64 timestamp_ns;
    uint8 source;         // 0=GPS, 1=VINS, 2=Fused
    double latitude;
    double longitude;
    double altitude;
    float velocity_ned[3];
    uint8 gps_spoofing_level;  // 0=none, 1=suspected, 2=detected
    boolean gps_reliable;
    float vins_confidence;
};
```

#### 3.2.4 QoS 策略

```cpp
// DDS QoS 配置
// 检测结果 - 高频但可丢失
detection_qos.reliability = BEST_EFFORT;
detection_qos.durability = VOLATILE;
detection_qos.history = KEEP_LAST;
detection_qos.depth = 1;  // 只保留最新一帧

// 制导命令 - 必须可靠到达
guidance_qos.reliability = RELIABLE;
guidance_qos.durability = VOLATILE;
guidance_qos.history = KEEP_LAST;
guidance_qos.depth = 1;
guidance_qos.deadline.period = 50ms;  // 20Hz控制频率

// 导航状态 - 持久化，支持 late joiner
navigation_qos.reliability = RELIABLE;
navigation_qos.durability = TRANSIENT_LOCAL;
navigation_qos.history = KEEP_LAST;
navigation_qos.depth = 10;
```

---

### 3.3 进程架构设计

#### 3.3.1 进程划分原则

1. **功能内聚**: 每个进程负责一个独立功能域
2. **故障隔离**: 一个进程崩溃不影响其他进程
3. **资源隔离**: CPU/内存/GPU 资源独立分配
4. **独立升级**: 可以单独更新某个进程

#### 3.3.2 进程清单

| 进程名 | 功能 | CPU优先级 | 内存限制 | GPU | 重启策略 |
|--------|------|-----------|----------|-----|----------|
| **video-capture** | 视频采集、编码、RTSP推流 | 高 | 512MB | NPU(预处理) | 自动重启 |
| **perception** | 目标检测、跟踪、距离估计 | 高 | 1GB | NPU(推理) | 自动重启 |
| **guidance** | 视觉制导、IBVS控制 | 实时 | 256MB | 否 | 自动重启 |
| **gps-defense** | GPS欺骗检测、RAIM | 中 | 128MB | 否 | 自动重启 |
| **vins-slam** | 视觉惯性导航 | 高 | 512MB | GPU/VPU | 自动重启 |
| **mission-planner** | 任务规划、航点生成 | 中 | 256MB | 否 | 自动重启 |
| **flight-control** | 飞控通信、MAVLink | 实时 | 128MB | 否 | 自动重启 |
| **data-logger** | 数据记录、回放 | 低 | 1GB | 否 | 不重启 |
| **system-manager** | 进程监控、健康检查 | 实时 | 64MB | 否 | 不重启 |
| **mqtt-broker** | MQTT消息总线 | 高 | 128MB | 否 | 自动重启 |
| **dds-daemon** | DDS域参与者 | 高 | 256MB | 否 | 自动重启 |

#### 3.3.3 进程间数据流

```
                    ┌─────────────────────────────────────────────────────────────┐
                    │                         数据流向                              │
                    └─────────────────────────────────────────────────────────────┘

Video Capture Process
       │ (RTSP)
       ▼
Perception Process ──────────────────────────────────────────────┐
       │ (DDS: DetectionArray)                                    │
       │                                                          │
       ▼                                                          │
Guidance Process ◀───────────────────────────────────────────────┤
       │ (DDS: GuidanceCommand)                                   │
       │                                                          │
       ▼                                                          │
Flight Control Process ──────────────────────────────────────────┘
       │ (MAVLink)
       ▼
Pixhawk/PX4


GPS Defense Process ◀── MQTT ──▶ System Manager (健康监控)
       │
       │ (DDS: NavigationState)
       ▼
Navigation Fusion


VINS SLAM Process ◀── DDS ──▶ Navigation Fusion
       │
       │ (DDS: NavigationState)
       ▼
Mission Planner
```

---

### 3.4 实施路线图

#### Phase 1: 基础设施 (2周)

1. **部署 MQTT Broker**
   ```bash
   # 选择 NanoMQ (轻量级，适合嵌入式)
   docker run -d -p 1883:1883 -p 8083:8083 \
       --name nanomq \
       emqx/nanomq:latest
   ```

2. **部署 MediaTx**
   ```bash
   # 编译安装 MediaTx (原 rtsp-simple-server)
   wget https://github.com/bluenviron/mediamtx/releases/download/v1.6.0/mediamtx_v1.6.0_linux_arm64.tar.gz
   ```

3. **部署 DDS**
   ```bash
   # 选择 Fast DDS (ROS2默认)
   sudo apt install ros-humble-fastdds
   ```

4. **部署 SupervisorD**
   ```bash
   sudo apt install supervisor
   ```

#### Phase 2: 视频流改造 (2周)

1. **创建 Video Capture Process**
   - 从 CameraSourceNode 独立出来
   - 使用 GStreamer 实现 RTSP 推流
   - 支持多路输出

2. **集成 WebRTC Gateway**
   - 部署 Janus Gateway
   - 配置 RTSP 到 WebRTC 转换

3. **Viewer 端改造**
   - 添加 WebRTC 播放器
   - 支持多路视频切换

#### Phase 3: 进程拆分 (3周)

1. **拆分 Perception Process**
   - 独立进程运行检测+跟踪
   - DDS 发布 DetectionArray
   - MQTT 发布检测事件

2. **拆分 Guidance Process**
   - 订阅 DetectionArray (DDS)
   - 发布 GuidanceCommand (DDS)
   - 20Hz 控制频率

3. **拆分其他进程**
   - GPS Defense
   - VINS SLAM
   - Mission Planner

#### Phase 4: 集成测试 (2周)

1. **进程间通信测试**
2. **故障恢复测试**
3. **性能压力测试**
4. **端到端集成测试**

---

### 3.5 代码示例

#### 3.5.1 DDS Publisher (DetectionArray)

```cpp
// perception_process.cpp
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>

using namespace eprosima::fastdds::dds;

class DetectionPublisher {
public:
    bool init() {
        // 创建 DomainParticipant
        participant_ = DomainParticipantFactory::get_instance()
            ->create_participant(0, PARTICIPANT_QOS_DEFAULT);
        
        // 注册类型
        type_.register_type(participant_);
        
        // 创建 Topic
        topic_ = participant_->create_topic("DetectionArray", 
                                            type_.get_type_name(), 
                                            TOPIC_QOS_DEFAULT);
        
        // 创建 Publisher
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
        
        // 配置 QoS
        DataWriterQos dw_qos;
        dw_qos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        dw_qos.durability().kind = VOLATILE_DURABILITY_QOS;
        dw_qos.history().kind = KEEP_LAST_HISTORY_QOS;
        dw_qos.history().depth = 1;
        
        // 创建 DataWriter
        writer_ = publisher_->create_datawriter(topic_, dw_qos, &listener_);
        
        return true;
    }
    
    void publish(const DetectionArray& detections) {
        writer_->write(&detections);
    }

private:
    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    DetectionArrayPubSubType type_;
    DetectionListener listener_;
};
```

#### 3.5.2 MQTT Publisher (遥测)

```cpp
// telemetry_publisher.cpp
#include <mqtt/async_client.h>

class MqttTelemetryPublisher {
public:
    bool connect(const std::string& broker_url, const std::string& uav_id) {
        client_ = std::make_unique<mqtt::async_client>(broker_url, 
                                                        "falconmind_" + uav_id);
        
        mqtt::connect_options conn_opts;
        conn_opts.set_keep_alive_interval(20);
        conn_opts.set_clean_session(true);
        conn_opts.set_automatic_reconnect(true);
        
        client_->connect(conn_opts)->wait();
        
        uav_id_ = uav_id;
        return true;
    }
    
    void publish_position(const Position& pos) {
        json j = {
            {"timestamp", pos.timestamp_ns},
            {"lat", pos.latitude},
            {"lon", pos.longitude},
            {"alt", pos.altitude}
        };
        
        std::string topic = "/falconmind/" + uav_id_ + "/telemetry/position";
        mqtt::message_ptr msg = mqtt::make_message(topic, j.dump());
        msg->set_qos(0);  // 位置数据可以丢失
        client_->publish(msg);
    }
    
    void publish_detection_event(const DetectionEvent& event) {
        json j = {
            {"event_type", event.type},  // "new", "lost", "update"
            {"track_id", event.track_id},
            {"class_name", event.class_name},
            {"confidence", event.confidence}
        };
        
        std::string topic = "/falconmind/" + uav_id_ + "/detection/events";
        mqtt::message_ptr msg = mqtt::make_message(topic, j.dump());
        msg->set_qos(1);  // 事件需要至少送达一次
        client_->publish(msg);
    }

private:
    std::unique_ptr<mqtt::async_client> client_;
    std::string uav_id_;
};
```

#### 3.5.3 GStreamer RTSP 推流

```cpp
// video_rtsp_pusher.cpp
#include <gst/gst.h>

class VideoRtspPusher {
public:
    bool init(const std::string& rtsp_url) {
        gst_init(nullptr, nullptr);
        
        // 创建 pipeline
        pipeline_ = gst_parse_launch(
            "v4l2src device=/dev/video0 ! "
            "video/x-raw,format=NV12,width=1920,height=1080,framerate=30/1 ! "
            "videoconvert ! "
            "x264enc tune=zerolatency bitrate=4000 speed-preset=ultrafast ! "
            "rtspclientsink location=" + rtsp_url,
            nullptr);
        
        return pipeline_ != nullptr;
    }
    
    void start() {
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);
    }
    
    void stop() {
        gst_element_set_state(pipeline_, GST_STATE_NULL);
    }

private:
    GstElement* pipeline_;
};
```

#### 3.5.4 SupervisorD 配置

```ini
; /etc/supervisor/conf.d/falconmind.conf

[program:falconmind-video]
command=/opt/falconmind/bin/video-capture-process --config /etc/falconmind/video.yaml
autostart=true
autorestart=true
user=falconmind
priority=10
stdout_logfile=/var/log/falconmind/video.log
stderr_logfile=/var/log/falconmind/video.error.log

[program:falconmind-perception]
command=/opt/falconmind/bin/perception-process --config /etc/falconmind/perception.yaml
autostart=true
autorestart=true
user=falconmind
priority=20
stdout_logfile=/var/log/falconmind/perception.log
stderr_logfile=/var/log/falconmind/perception.error.log

[program:falconmind-guidance]
command=/opt/falconmind/bin/guidance-process --config /etc/falconmind/guidance.yaml
autostart=true
autorestart=true
user=falconmind
priority=30
stdout_logfile=/var/log/falconmind/guidance.log
stderr_logfile=/var/log/falconmind/guidance.error.log

[program:falconmind-mqtt-broker]
command=/usr/sbin/mosquitto -c /etc/mosquitto/mosquitto.conf
autostart=true
autorestart=true
user=mosquitto
priority=5

[group:falconmind]
programs=falconmind-video,falconmind-perception,falconmind-guidance
```

---

## 4. 关键技术选型对比

### 4.1 DDS 实现对比

| 特性 | Fast DDS | Cyclone DDS | RTI Connext | OpenDDS |
|------|----------|-------------|-------------|---------|
| **许可** | Apache 2.0 | EPL 2.0 | 商业/社区 | 开源 |
| **ROS2** | 默认 |  Tier 1 | 支持 | 不支持 |
| **性能** | 高 | 高 | 极高 | 中 |
| **RK3588** | ✅ | ✅ | ❌(资源大) | ❌ |
| **推荐** | ⭐⭐⭐ | ⭐⭐ | ⭐ | ❌ |

**推荐: Fast DDS** (ROS2默认，文档完善，RK平台验证)

### 4.2 MQTT Broker 对比

| 特性 | Mosquitto | NanoMQ | EMQX | VerneMQ |
|------|-----------|--------|------|---------|
| **资源** | 低 | 极低 | 高 | 中 |
| **性能** | 中 | 高 | 极高 | 高 |
| **边缘** | ✅ | ✅⭐ | ❌ | ❌ |
| **功能** | 基础 | 现代 | 全功能 | 分布式 |
| **推荐** | ⭐⭐ | ⭐⭐⭐ | ❌ | ❌ |

**推荐: NanoMQ** (专为嵌入式设计，MQTT 5.0，资源占用极低)

### 4.3 RTSP/WebRTC 方案对比

| 方案 | 延迟 | 资源 | 复杂度 | 推荐 |
|------|------|------|--------|------|
| MediaTx + Janus | 低 | 中 | 中 | ⭐⭐⭐ |
| GStreamer + webrtcbin | 低 | 高 | 高 | ⭐⭐ |
| mediasoup | 极低 | 中 | 高 | ⭐⭐⭐(多用户) |
| Pion | 低 | 低 | 中 | ⭐⭐(Go生态) |

**推荐: MediaTx + Janus** (成熟稳定，RK平台有移植案例)

---

## 5. 风险与应对

| 风险 | 可能性 | 影响 | 应对措施 |
|------|--------|------|----------|
| **DDS延迟不达标** | 中 | 高 | 准备MQTT降级方案 |
| **视频流卡顿** | 中 | 中 | 分级编码，自适应码率 |
| **进程间通信故障** | 低 | 高 | watchdog + 自动重启 |
| **NPU资源竞争** | 高 | 高 | 进程级NPU调度器 |
| **内存不足** | 中 | 高 | cgroup限制 + OOM killer配置 |

---

## 6. 总结

### 改进价值

1. **视频流标准化**: RTSP/WebRTC 工业标准，支持浏览器直接播放
2. **模块解耦**: 独立进程，故障隔离，可独立升级
3. **通信标准化**: MQTT/DDS 广泛支持，便于第三方集成
4. **可观测性**: 全链路监控，健康检查，自动恢复

### 下一步行动

1. **评估当前SDK改动量**: 确定是否需要重构 SDK 接口
2. **搭建测试环境**: 使用 RK3588 开发板验证方案
3. **分阶段实施**: 先视频流，再通信，最后进程拆分
4. **性能基准测试**: 对比改造前后的延迟和资源占用

---

*文档版本: v1.0*  
*作者: FalconMind Architect*  
*日期: 2026-03-06*
