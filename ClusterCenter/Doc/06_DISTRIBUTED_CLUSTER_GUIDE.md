# 06. 分布式集群支持指南

> **阅读顺序**: 第 6 篇  
> **最后更新**: 2024-01-30

## 📚 文档导航

- **00_PROGRESS_INVENTORY.md** - 项目进展盘点
- **05_ADVANCED_OPTIMIZATIONS_SUMMARY.md** - 高级优化功能总结
- **07_DISTRIBUTED_CLUSTER_ENHANCEMENT.md** - 分布式集群完善功能

## 当前状态

### ✅ 已实现的功能

1. **Raft 算法框架**: 完整的 Raft 选举和日志复制逻辑
2. **节点发现**: 节点注册和发现机制
3. **RPC 通信框架**: HTTP-based RPC 客户端和服务器
4. **数据同步框架**: 通过 Raft 日志复制同步数据

### ⚠️ 当前限制

1. **网络通信**: RPC 实现是框架性的，需要完善错误处理和重试
2. **节点发现**: 需要配置或服务发现机制（如 Consul、etcd）
3. **数据同步**: 需要实现具体的数据同步逻辑
4. **故障恢复**: 需要实现节点故障检测和恢复

## 分布式集群架构

```
┌─────────────────────────────────────────────────────────┐
│              Distributed Cluster Center                 │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │   Node 1     │  │   Node 2     │  │   Node 3     │  │
│  │  (Leader)    │  │  (Follower)  │  │  (Follower)  │  │
│  ├──────────────┤  ├──────────────┤  ├──────────────┤  │
│  │ Raft Node    │  │ Raft Node    │  │ Raft Node    │  │
│  │ RPC Server   │  │ RPC Server   │  │ RPC Server   │  │
│  │ Data Store   │  │ Data Store   │  │ Data Store   │  │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  │
│         │                  │                  │         │
│         └──────────────────┴──────────────────┘         │
│                    Raft Consensus                         │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

## 部署方式

### 方式 1: 多节点部署（推荐）

每个节点运行独立的 Cluster Center 实例：

```bash
# Node 1 (Leader)
export NODE_ID=node_1
export NODE_ADDRESS=192.168.1.10
export NODE_PORT=8888
python3 distributed_main.py

# Node 2 (Follower)
export NODE_ID=node_2
export NODE_ADDRESS=192.168.1.11
export NODE_PORT=8888
export PEER_NODES='[{"node_id":"node_1","address":"192.168.1.10","port":8888}]'
python3 distributed_main.py

# Node 3 (Follower)
export NODE_ID=node_3
export NODE_ADDRESS=192.168.1.12
export NODE_PORT=8888
export PEER_NODES='[{"node_id":"node_1","address":"192.168.1.10","port":8888},{"node_id":"node_2","address":"192.168.1.11","port":8888}]'
python3 distributed_main.py
```

### 方式 2: Docker Compose 部署

```yaml
version: '3.8'
services:
  cluster-center-1:
    image: falconmind/cluster-center:latest
    environment:
      - NODE_ID=node_1
      - NODE_ADDRESS=0.0.0.0
      - NODE_PORT=8888
    ports:
      - "8888:8888"
      - "8889:8889"
  
  cluster-center-2:
    image: falconmind/cluster-center:latest
    environment:
      - NODE_ID=node_2
      - NODE_ADDRESS=0.0.0.0
      - NODE_PORT=8888
      - PEER_NODES=[{"node_id":"node_1","address":"cluster-center-1","port":8888}]
    ports:
      - "8890:8888"
      - "8891:8889"
  
  cluster-center-3:
    image: falconmind/cluster-center:latest
    environment:
      - NODE_ID=node_3
      - NODE_ADDRESS=0.0.0.0
      - NODE_PORT=8888
      - PEER_NODES=[{"node_id":"node_1","address":"cluster-center-1","port":8888},{"node_id":"node_2","address":"cluster-center-2","port":8888}]
    ports:
      - "8892:8888"
      - "8893:8889"
```

## 配置说明

### 环境变量

- `NODE_ID`: 节点唯一标识
- `NODE_ADDRESS`: 节点监听地址
- `NODE_PORT`: 节点 HTTP 端口
- `RAFT_PORT`: Raft RPC 端口（默认：NODE_PORT + 1）
- `PEER_NODES`: 对等节点列表（JSON 格式）

### 节点发现

当前支持手动配置对等节点。未来可以集成：
- Consul
- etcd
- Kubernetes Service Discovery
- DNS-based Discovery

## 数据一致性

### Raft 保证

- **强一致性**: 所有节点通过 Raft 算法保证数据一致性
- **日志复制**: 所有写操作通过 Raft 日志复制到多数节点
- **读一致性**: 从领导者读取保证强一致性

### 数据同步

1. **任务数据**: 通过 Raft 日志复制
2. **UAV 状态**: 定期同步（最终一致性）
3. **遥测数据**: 不通过 Raft（高频率，最终一致性）

## 故障处理

### 节点故障

1. **跟随者故障**: 不影响集群运行，自动从日志恢复
2. **领导者故障**: 自动选举新领导者
3. **网络分区**: 多数派继续服务，少数派不可用

### 恢复机制

1. **日志恢复**: 节点重启后从 Raft 日志恢复
2. **快照恢复**: 使用快照加速恢复
3. **数据同步**: 从领导者同步最新数据

## 性能考虑

### 写性能

- **延迟**: 需要多数派确认（通常 < 100ms）
- **吞吐量**: 取决于网络和磁盘性能

### 读性能

- **从领导者读**: 强一致性，低延迟
- **从跟随者读**: 最终一致性，可扩展

### 扩展性

- **水平扩展**: 可以添加更多跟随者节点
- **读扩展**: 跟随者可以处理读请求
- **写扩展**: 受 Raft 限制（需要多数派）

## 使用示例

### 启动分布式集群

```python
# 节点 1
manager1 = create_distributed_cluster(
    node_id="node_1",
    address="192.168.1.10",
    port=8888
)
await manager1.start()

# 节点 2
manager2 = create_distributed_cluster(
    node_id="node_2",
    address="192.168.1.11",
    port=8888,
    peer_nodes=[{"node_id": "node_1", "address": "192.168.1.10", "port": 8888}]
)
await manager2.start()
```

### 查询集群状态

```bash
curl http://192.168.1.10:8888/cluster/info
```

### 注册新节点

```bash
curl -X POST http://192.168.1.10:8888/cluster/register \
  -H "Content-Type: application/json" \
  -d '{
    "node_id": "node_3",
    "address": "192.168.1.12",
    "port": 8888
  }'
```

## 后续改进

### 短期（1-2周）

1. **完善 RPC 通信**: 错误处理、重试、超时
2. **节点健康检查**: 定期检查节点状态
3. **数据同步实现**: 实现具体的任务和 UAV 数据同步

### 中期（1个月）

1. **服务发现集成**: 集成 Consul 或 etcd
2. **负载均衡**: 读请求负载均衡到跟随者
3. **监控和告警**: 集群健康监控

### 长期（2-3个月）

1. **gRPC 支持**: 替换 HTTP RPC 为 gRPC
2. **跨区域部署**: 支持跨数据中心的部署
3. **自动扩缩容**: 基于负载自动添加/移除节点

## 总结

**当前实现状态**: 
- ✅ 支持分布式集群架构
- ✅ Raft 算法完整实现
- ⚠️ 网络通信需要完善
- ⚠️ 需要配置对等节点

**推荐部署方式**: 
- 生产环境：3-5 个节点（奇数个）
- 开发环境：1 个节点（单机模式）

**下一步**: 
完善网络通信层，实现自动节点发现，完善数据同步机制。
