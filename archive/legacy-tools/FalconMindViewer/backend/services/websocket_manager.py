"""
WebSocket 连接管理器
优化后的连接管理，支持心跳、队列、重连等
"""
import asyncio
from typing import Set, Optional
from fastapi import WebSocket
from collections import deque
import logging

from config import settings

logger = logging.getLogger(__name__)


class ConnectionManager:
    """
    管理所有 WebSocket 连接，将最新遥测广播给前端 Viewer
    优化版本：支持消息队列、心跳检测、连接数限制
    """
    
    def __init__(self, max_connections: int = None, max_queue_size: int = None):
        """
        初始化连接管理器
        
        Args:
            max_connections: 最大连接数
            max_queue_size: 消息队列最大大小
        """
        self.active_connections: Set[WebSocket] = set()
        self.max_connections = max_connections or settings.ws_max_connections
        self.max_queue_size = max_queue_size or settings.ws_max_queue_size
        
        # 消息队列
        self.message_queue: Optional[asyncio.Queue] = None
        self.broadcast_task: Optional[asyncio.Task] = None
        self.running = False
        
        # 心跳任务
        self.heartbeat_interval = settings.ws_heartbeat_interval
        self.heartbeat_tasks: dict = {}  # websocket -> task
    
    async def start(self):
        """启动连接管理器（启动广播任务）"""
        if self.running:
            return
        
        self.running = True
        self.message_queue = asyncio.Queue(maxsize=self.max_queue_size)
        self.broadcast_task = asyncio.create_task(self._broadcast_worker())
        logger.info("WebSocket连接管理器已启动")
    
    async def stop(self):
        """停止连接管理器"""
        self.running = False
        
        # 取消广播任务
        if self.broadcast_task:
            self.broadcast_task.cancel()
            try:
                await self.broadcast_task
            except asyncio.CancelledError:
                pass
        
        # 取消所有心跳任务
        for task in self.heartbeat_tasks.values():
            task.cancel()
        self.heartbeat_tasks.clear()
        
        # 关闭所有连接
        for ws in list(self.active_connections):
            try:
                await ws.close()
            except Exception:
                pass
        self.active_connections.clear()
        
        logger.info("WebSocket连接管理器已停止")
    
    async def connect(self, websocket: WebSocket) -> bool:
        """
        接受新的 WebSocket 连接
        
        Args:
            websocket: WebSocket 连接
        
        Returns:
            是否成功连接
        """
        # 检查连接数限制
        if len(self.active_connections) >= self.max_connections:
            logger.warning(f"连接数已达上限 ({self.max_connections})，拒绝新连接")
            try:
                await websocket.close(code=1008, reason="Too many connections")
            except Exception:
                pass
            return False
        
        try:
            await websocket.accept()
            self.active_connections.add(websocket)
            
            # 启动心跳任务
            heartbeat_task = asyncio.create_task(self._heartbeat(websocket))
            self.heartbeat_tasks[websocket] = heartbeat_task
            
            logger.info(f"WebSocket连接已建立，当前连接数: {len(self.active_connections)}")
            return True
        except Exception as e:
            logger.error(f"接受WebSocket连接失败: {e}")
            return False
    
    def disconnect(self, websocket: WebSocket) -> None:
        """
        断开 WebSocket 连接
        
        Args:
            websocket: WebSocket 连接
        """
        if websocket in self.active_connections:
            self.active_connections.remove(websocket)
            logger.info(f"WebSocket连接已断开，当前连接数: {len(self.active_connections)}")
        
        # 取消心跳任务
        if websocket in self.heartbeat_tasks:
            task = self.heartbeat_tasks.pop(websocket)
            task.cancel()
    
    async def queue_broadcast(self, message: dict) -> None:
        """
        将消息加入广播队列（非阻塞）
        
        Args:
            message: 要广播的消息
        """
        if not self.running or not self.message_queue:
            logger.warning("连接管理器未运行，消息被丢弃")
            return
        
        try:
            self.message_queue.put_nowait(message)
        except asyncio.QueueFull:
            logger.warning("广播队列已满，消息被丢弃")
    
    async def broadcast(self, message: dict) -> None:
        """
        立即广播消息（同步，阻塞）
        建议使用 queue_broadcast 进行异步广播
        
        Args:
            message: 要广播的消息
        """
        disconnected = []
        for ws in self.active_connections:
            try:
                await ws.send_json(message)
            except Exception as e:
                logger.warning(f"广播消息失败: {e}")
                disconnected.append(ws)
        
        # 清理断开的连接
        for ws in disconnected:
            self.disconnect(ws)
    
    async def _broadcast_worker(self):
        """后台广播任务"""
        while self.running:
            try:
                # 从队列获取消息（带超时）
                message = await asyncio.wait_for(
                    self.message_queue.get(),
                    timeout=1.0
                )
                await self._broadcast_to_all(message)
            except asyncio.TimeoutError:
                continue
            except Exception as e:
                logger.error(f"广播任务错误: {e}")
    
    async def _broadcast_to_all(self, message: dict):
        """实际广播逻辑"""
        disconnected = []
        for ws in self.active_connections:
            try:
                await ws.send_json(message)
            except Exception as e:
                logger.debug(f"发送消息失败: {e}")
                disconnected.append(ws)
        
        # 清理断开的连接
        for ws in disconnected:
            self.disconnect(ws)
    
    async def _heartbeat(self, websocket: WebSocket):
        """心跳任务，检测连接状态"""
        try:
            while websocket in self.active_connections:
                await asyncio.sleep(self.heartbeat_interval)
                try:
                    await websocket.send_json({"type": "ping"})
                except Exception as e:
                    logger.debug(f"心跳发送失败: {e}")
                    break
        except asyncio.CancelledError:
            pass
        except Exception as e:
            logger.debug(f"心跳任务异常: {e}")
        finally:
            # 连接断开，清理
            self.disconnect(websocket)
    
    def get_connection_count(self) -> int:
        """获取当前连接数"""
        return len(self.active_connections)
