"""
Service Discovery - 服务发现
支持 Consul、etcd 等自动服务发现
"""

import os
import json
import asyncio
from typing import Dict, List, Optional, Callable
from dataclasses import dataclass
from datetime import datetime, timedelta
import logging

logger = logging.getLogger(__name__)


@dataclass
class ServiceNode:
    """服务节点"""
    node_id: str
    address: str
    port: int
    metadata: Dict = None
    last_seen: datetime = None
    
    def __post_init__(self):
        if self.last_seen is None:
            self.last_seen = datetime.utcnow()
        if self.metadata is None:
            self.metadata = {}


class ServiceDiscovery:
    """服务发现抽象基类"""
    
    def __init__(self):
        self.nodes: Dict[str, ServiceNode] = {}
        self.watch_callbacks: List[Callable] = []
    
    async def register(self, node_id: str, address: str, port: int, metadata: Dict = None):
        """注册节点"""
        raise NotImplementedError
    
    async def deregister(self, node_id: str):
        """注销节点"""
        raise NotImplementedError
    
    async def discover(self) -> List[ServiceNode]:
        """发现节点"""
        raise NotImplementedError
    
    async def watch(self, callback: Callable):
        """监听节点变化"""
        self.watch_callbacks.append(callback)
    
    def _notify_watchers(self, event: str, node: ServiceNode):
        """通知监听器"""
        for callback in self.watch_callbacks:
            try:
                callback(event, node)
            except Exception as e:
                logger.error(f"Error in watch callback: {e}")


class StaticServiceDiscovery(ServiceDiscovery):
    """静态服务发现（手动配置）"""
    
    def __init__(self, static_nodes: List[Dict] = None):
        super().__init__()
        self.static_nodes = static_nodes or []
        
        # 从环境变量读取
        peer_nodes_json = os.getenv("PEER_NODES", "[]")
        try:
            env_nodes = json.loads(peer_nodes_json)
            self.static_nodes.extend(env_nodes)
        except json.JSONDecodeError:
            logger.warning("Failed to parse PEER_NODES environment variable")
        
        # 初始化静态节点
        for node_info in self.static_nodes:
            node = ServiceNode(
                node_id=node_info.get("node_id"),
                address=node_info.get("address"),
                port=node_info.get("port"),
                metadata=node_info.get("metadata", {})
            )
            self.nodes[node.node_id] = node
    
    async def register(self, node_id: str, address: str, port: int, metadata: Dict = None):
        """注册节点"""
        node = ServiceNode(
            node_id=node_id,
            address=address,
            port=port,
            metadata=metadata or {}
        )
        self.nodes[node_id] = node
        self._notify_watchers("register", node)
        logger.info(f"Registered static node: {node_id} at {address}:{port}")
    
    async def deregister(self, node_id: str):
        """注销节点"""
        if node_id in self.nodes:
            node = self.nodes.pop(node_id)
            self._notify_watchers("deregister", node)
            logger.info(f"Deregistered static node: {node_id}")
    
    async def discover(self) -> List[ServiceNode]:
        """发现节点"""
        return list(self.nodes.values())


class ConsulServiceDiscovery(ServiceDiscovery):
    """Consul 服务发现"""
    
    def __init__(
        self,
        consul_host: str = "localhost",
        consul_port: int = 8500,
        service_name: str = "falconmind-cluster-center"
    ):
        super().__init__()
        self.consul_host = consul_host
        self.consul_port = consul_port
        self.service_name = service_name
        self.consul_base_url = f"http://{consul_host}:{consul_port}"
        self.watching = False
    
    async def register(
        self,
        node_id: str,
        address: str,
        port: int,
        metadata: Dict = None
    ):
        """注册节点到 Consul"""
        try:
            import aiohttp
        except ImportError:
            logger.error("aiohttp not installed, cannot use Consul")
            return
        
        service_id = f"{self.service_name}-{node_id}"
        service_data = {
            "ID": service_id,
            "Name": self.service_name,
            "Address": address,
            "Port": port,
            "Tags": [f"node_id:{node_id}"],
            "Meta": metadata or {},
            "Check": {
                "HTTP": f"http://{address}:{port}/health",
                "Interval": "10s",
                "Timeout": "2s",
                "DeregisterCriticalServiceAfter": "30s"  # 健康检查失败后30秒注销
            }
        }
        
        url = f"{self.consul_base_url}/v1/agent/service/register"
        
        try:
            async with aiohttp.ClientSession() as session:
                async with session.put(url, json=service_data) as resp:
                    if resp.status == 200:
                        logger.info(f"Registered node {node_id} to Consul")
                    else:
                        logger.error(f"Failed to register to Consul: {resp.status}")
        except Exception as e:
            logger.error(f"Error registering to Consul: {e}")
    
    async def deregister(self, node_id: str):
        """从 Consul 注销节点"""
        try:
            import aiohttp
        except ImportError:
            return
        
        service_id = f"{self.service_name}-{node_id}"
        url = f"{self.consul_base_url}/v1/agent/service/deregister/{service_id}"
        
        try:
            async with aiohttp.ClientSession() as session:
                async with session.put(url) as resp:
                    if resp.status == 200:
                        logger.info(f"Deregistered node {node_id} from Consul")
        except Exception as e:
            logger.error(f"Error deregistering from Consul: {e}")
    
    async def discover(self) -> List[ServiceNode]:
        """从 Consul 发现节点"""
        try:
            import aiohttp
        except ImportError:
            logger.error("aiohttp not installed, cannot use Consul")
            return []
        
        url = f"{self.consul_base_url}/v1/health/service/{self.service_name}?passing=true"
        
        try:
            async with aiohttp.ClientSession() as session:
                async with session.get(url) as resp:
                    if resp.status == 200:
                        services = await resp.json()
                        nodes = []
                        
                        for service in services:
                            service_info = service.get("Service", {})
                            node_id = None
                            
                            # 从 Tags 或 Meta 中提取 node_id
                            tags = service_info.get("Tags", [])
                            for tag in tags:
                                if tag.startswith("node_id:"):
                                    node_id = tag.split(":", 1)[1]
                                    break
                            
                            if not node_id:
                                # 从 Meta 中获取
                                meta = service_info.get("Meta", {})
                                node_id = meta.get("node_id", service_info.get("ID", ""))
                            
                            node = ServiceNode(
                                node_id=node_id,
                                address=service_info.get("Address", ""),
                                port=service_info.get("Port", 0),
                                metadata=service_info.get("Meta", {}),
                                last_seen=datetime.utcnow()
                            )
                            nodes.append(node)
                            self.nodes[node_id] = node
                        
                        return nodes
                    else:
                        logger.error(f"Failed to discover from Consul: {resp.status}")
                        return []
        except Exception as e:
            logger.error(f"Error discovering from Consul: {e}")
            return []
    
    async def start_watching(self, interval: float = 5.0):
        """启动节点监听"""
        if self.watching:
            return
        
        self.watching = True
        
        async def watch_loop():
            while self.watching:
                try:
                    discovered_nodes = await self.discover()
                    current_node_ids = set(self.nodes.keys())
                    discovered_node_ids = {n.node_id for n in discovered_nodes}
                    
                    # 检测新节点
                    for node in discovered_nodes:
                        if node.node_id not in current_node_ids:
                            self._notify_watchers("register", node)
                    
                    # 检测移除的节点
                    for node_id in current_node_ids - discovered_node_ids:
                        if node_id in self.nodes:
                            node = self.nodes.pop(node_id)
                            self._notify_watchers("deregister", node)
                    
                    await asyncio.sleep(interval)
                except Exception as e:
                    logger.error(f"Error in watch loop: {e}")
                    await asyncio.sleep(interval)
        
        asyncio.create_task(watch_loop())
    
    def stop_watching(self):
        """停止节点监听"""
        self.watching = False


class EtcdServiceDiscovery(ServiceDiscovery):
    """etcd 服务发现"""
    
    def __init__(
        self,
        etcd_host: str = "localhost",
        etcd_port: int = 2379,
        service_prefix: str = "/falconmind/cluster-center"
    ):
        super().__init__()
        self.etcd_host = etcd_host
        self.etcd_port = etcd_port
        self.service_prefix = service_prefix
        self.etcd_base_url = f"http://{etcd_host}:{etcd_port}"
    
    async def register(
        self,
        node_id: str,
        address: str,
        port: int,
        metadata: Dict = None
    ):
        """注册节点到 etcd"""
        try:
            import aiohttp
        except ImportError:
            logger.error("aiohttp not installed, cannot use etcd")
            return
        
        key = f"{self.service_prefix}/{node_id}"
        value = json.dumps({
            "address": address,
            "port": port,
            "metadata": metadata or {}
        })
        
        url = f"{self.etcd_base_url}/v3/kv/put"
        payload = {
            "key": key.encode().hex(),
            "value": value.encode().hex()
        }
        
        try:
            async with aiohttp.ClientSession() as session:
                async with session.post(url, json=payload) as resp:
                    if resp.status == 200:
                        logger.info(f"Registered node {node_id} to etcd")
                    else:
                        logger.error(f"Failed to register to etcd: {resp.status}")
        except Exception as e:
            logger.error(f"Error registering to etcd: {e}")
    
    async def deregister(self, node_id: str):
        """从 etcd 注销节点"""
        try:
            import aiohttp
        except ImportError:
            return
        
        key = f"{self.service_prefix}/{node_id}"
        url = f"{self.etcd_base_url}/v3/kv/deleterange"
        payload = {
            "key": key.encode().hex()
        }
        
        try:
            async with aiohttp.ClientSession() as session:
                async with session.post(url, json=payload) as resp:
                    if resp.status == 200:
                        logger.info(f"Deregistered node {node_id} from etcd")
        except Exception as e:
            logger.error(f"Error deregistering from etcd: {e}")
    
    async def discover(self) -> List[ServiceNode]:
        """从 etcd 发现节点"""
        try:
            import aiohttp
        except ImportError:
            logger.error("aiohttp not installed, cannot use etcd")
            return []
        
        url = f"{self.etcd_base_url}/v3/kv/range"
        prefix = self.service_prefix.encode().hex()
        payload = {
            "key": prefix,
            "range_end": self._increment_key(prefix)
        }
        
        try:
            async with aiohttp.ClientSession() as session:
                async with session.post(url, json=payload) as resp:
                    if resp.status == 200:
                        data = await resp.json()
                        nodes = []
                        
                        for kv in data.get("kvs", []):
                            key = bytes.fromhex(kv["key"]).decode()
                            value = json.loads(bytes.fromhex(kv["value"]).decode())
                            
                            node_id = key.split("/")[-1]
                            node = ServiceNode(
                                node_id=node_id,
                                address=value.get("address", ""),
                                port=value.get("port", 0),
                                metadata=value.get("metadata", {}),
                                last_seen=datetime.utcnow()
                            )
                            nodes.append(node)
                            self.nodes[node_id] = node
                        
                        return nodes
                    else:
                        logger.error(f"Failed to discover from etcd: {resp.status}")
                        return []
        except Exception as e:
            logger.error(f"Error discovering from etcd: {e}")
            return []
    
    def _increment_key(self, key_hex: str) -> str:
        """递增键（用于范围查询）"""
        # 简化实现：在最后一个字符加1
        try:
            key_bytes = bytes.fromhex(key_hex)
            key_bytes = key_bytes[:-1] + bytes([key_bytes[-1] + 1])
            return key_bytes.hex()
        except:
            # 如果失败，返回一个很大的键
            return "f" * len(key_hex)


def create_service_discovery() -> ServiceDiscovery:
    """
    创建服务发现实例（根据环境变量选择）
    
    环境变量:
    - DISCOVERY_TYPE: "static", "consul", "etcd" (默认: static)
    - CONSUL_HOST: Consul 主机
    - CONSUL_PORT: Consul 端口
    - ETCD_HOST: etcd 主机
    - ETCD_PORT: etcd 端口
    """
    discovery_type = os.getenv("DISCOVERY_TYPE", "static").lower()
    
    if discovery_type == "consul":
        return ConsulServiceDiscovery(
            consul_host=os.getenv("CONSUL_HOST", "localhost"),
            consul_port=int(os.getenv("CONSUL_PORT", "8500"))
        )
    elif discovery_type == "etcd":
        return EtcdServiceDiscovery(
            etcd_host=os.getenv("ETCD_HOST", "localhost"),
            etcd_port=int(os.getenv("ETCD_PORT", "2379"))
        )
    else:
        # 静态服务发现（默认）
        return StaticServiceDiscovery()
