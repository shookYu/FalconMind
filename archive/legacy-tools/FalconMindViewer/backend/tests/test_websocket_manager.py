"""
WebSocket管理器测试
"""
import pytest
import asyncio
from unittest.mock import AsyncMock, MagicMock, patch
from services.websocket_manager import ConnectionManager

# 配置pytest-asyncio
pytest_plugins = ('pytest_asyncio',)


@pytest.fixture
def connection_manager():
    """创建连接管理器实例"""
    return ConnectionManager(max_connections=10, max_queue_size=100)


@pytest.mark.asyncio
class TestConnectionManager:
    """WebSocket管理器测试类"""
    
    async def test_start_stop(self, connection_manager):
        """测试启动和停止"""
        await connection_manager.start()
        assert connection_manager.running is True
        assert connection_manager.message_queue is not None
        
        await connection_manager.stop()
        assert connection_manager.running is False
        assert len(connection_manager.active_connections) == 0
    
    async def test_connect_success(self, connection_manager):
        """测试成功连接"""
        await connection_manager.start()
        
        websocket = AsyncMock()
        websocket.accept = AsyncMock()
        
        result = await connection_manager.connect(websocket)
        
        assert result is True
        assert websocket in connection_manager.active_connections
        websocket.accept.assert_called_once()
    
    async def test_connect_max_connections(self, connection_manager):
        """测试连接数限制"""
        await connection_manager.start()
        
        # 填满连接
        for i in range(10):
            ws = AsyncMock()
            ws.accept = AsyncMock()
            await connection_manager.connect(ws)
        
        # 尝试第11个连接（应该被拒绝）
        ws11 = AsyncMock()
        ws11.close = AsyncMock()
        result = await connection_manager.connect(ws11)
        
        assert result is False
        assert ws11 not in connection_manager.active_connections
        ws11.close.assert_called_once()
    
    async def test_disconnect(self, connection_manager):
        """测试断开连接"""
        await connection_manager.start()
        
        websocket = AsyncMock()
        websocket.accept = AsyncMock()
        await connection_manager.connect(websocket)
        
        assert websocket in connection_manager.active_connections
        
        connection_manager.disconnect(websocket)
        
        assert websocket not in connection_manager.active_connections
    
    async def test_queue_broadcast(self, connection_manager):
        """测试消息队列广播"""
        await connection_manager.start()
        
        message = {"type": "test", "data": "test_data"}
        await connection_manager.queue_broadcast(message)
        
        # 检查消息是否在队列中
        queued_message = await connection_manager.message_queue.get()
        assert queued_message == message
    
    async def test_broadcast_to_all(self, connection_manager):
        """测试广播给所有连接"""
        await connection_manager.start()
        
        # 创建多个模拟连接
        ws1 = AsyncMock()
        ws1.accept = AsyncMock()
        ws1.send_json = AsyncMock()
        await connection_manager.connect(ws1)
        
        ws2 = AsyncMock()
        ws2.accept = AsyncMock()
        ws2.send_json = AsyncMock()
        await connection_manager.connect(ws2)
        
        # 广播消息
        message = {"type": "test", "data": "test_data"}
        await connection_manager.broadcast(message)
        
        # 验证所有连接都收到了消息
        ws1.send_json.assert_called_once_with(message)
        ws2.send_json.assert_called_once_with(message)
    
    async def test_broadcast_disconnected_cleanup(self, connection_manager):
        """测试广播时自动清理断开的连接"""
        await connection_manager.start()
        
        # 创建连接
        ws1 = AsyncMock()
        ws1.accept = AsyncMock()
        ws1.send_json = AsyncMock()
        await connection_manager.connect(ws1)
        
        ws2 = AsyncMock()
        ws2.accept = AsyncMock()
        ws2.send_json = AsyncMock(side_effect=Exception("Connection closed"))
        await connection_manager.connect(ws2)
        
        # 广播消息（ws2会失败）
        message = {"type": "test", "data": "test_data"}
        await connection_manager.broadcast(message)
        
        # ws1应该收到消息
        ws1.send_json.assert_called_once_with(message)
        
        # ws2应该被自动清理
        assert ws2 not in connection_manager.active_connections
    
    async def test_get_connection_count(self, connection_manager):
        """测试获取连接数"""
        await connection_manager.start()
        
        assert connection_manager.get_connection_count() == 0
        
        ws1 = AsyncMock()
        ws1.accept = AsyncMock()
        await connection_manager.connect(ws1)
        
        assert connection_manager.get_connection_count() == 1
        
        ws2 = AsyncMock()
        ws2.accept = AsyncMock()
        await connection_manager.connect(ws2)
        
        assert connection_manager.get_connection_count() == 2
