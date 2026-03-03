"""
Flow 配置验证服务

提供 Flow 配置的完整验证，包括：
- 节点类型验证
- 连接有效性验证
- 参数验证
- 循环依赖检测
"""
from typing import List, Dict, Any, Optional, Set
from dataclasses import dataclass, field
from enum import Enum


class ValidationSeverity(Enum):
    """验证严重程度"""
    ERROR = "error"
    WARNING = "warning"
    INFO = "info"


@dataclass
class ValidationError:
    """验证错误"""
    type: str
    message: str
    severity: ValidationSeverity
    node_id: Optional[str] = None
    edge_id: Optional[str] = None
    field: Optional[str] = None


@dataclass
class ValidationResult:
    """验证结果"""
    valid: bool
    errors: List[ValidationError] = field(default_factory=list)
    
    def add_error(self, type: str, message: str, node_id: Optional[str] = None, 
                  edge_id: Optional[str] = None, field: Optional[str] = None):
        """添加错误"""
        self.errors.append(ValidationError(
            type=type,
            message=message,
            severity=ValidationSeverity.ERROR,
            node_id=node_id,
            edge_id=edge_id,
            field=field
        ))
        self.valid = False
    
    def add_warning(self, type: str, message: str, node_id: Optional[str] = None,
                    edge_id: Optional[str] = None, field: Optional[str] = None):
        """添加警告"""
        self.errors.append(ValidationError(
            type=type,
            message=message,
            severity=ValidationSeverity.WARNING,
            node_id=node_id,
            edge_id=edge_id,
            field=field
        ))
    
    def to_dict(self) -> Dict[str, Any]:
        """转换为字典"""
        return {
            "valid": self.valid,
            "errors": [
                {
                    "type": e.type,
                    "message": e.message,
                    "severity": e.severity.value,
                    "node_id": e.node_id,
                    "edge_id": e.edge_id,
                    "field": e.field
                }
                for e in self.errors
            ]
        }


# 有效的节点类型定义
VALID_NODE_TYPES = {
    # 触发器节点
    "mission_start",
    "timer",
    "battery_low",
    "target_detected",
    "gps_lost",
    "communication_lost",
    
    # 动作节点
    "search_area",
    "take_photo",
    "hover",
    "return_home",
    "goto_waypoint",
    "send_message",
    "land",
    "takeoff",
    
    # 条件节点
    "battery_check",
    "altitude_check",
    "target_count",
    "compare",
}

# 必需的参数配置
REQUIRED_PARAMS = {
    "search_area": ["area", "altitude"],
    "take_photo": [],
    "hover": ["duration"],
    "return_home": [],
    "goto_waypoint": ["lat", "lng"],
    "send_message": ["topic"],
}


class FlowValidator:
    """Flow 配置验证器"""
    
    def validate(self, nodes: List[Dict], edges: List[Dict]) -> ValidationResult:
        """
        验证 Flow 配置
        
        Args:
            nodes: 节点列表
            edges: 边列表
            
        Returns:
            ValidationResult
        """
        result = ValidationResult(valid=True)
        
        # 1. 检查空流程
        if not nodes:
            result.add_error("EMPTY_FLOW", "Flow must contain at least one node")
            return result
        
        # 2. 验证节点
        node_ids = set()
        trigger_count = 0
        
        for node in nodes:
            self._validate_node(node, result)
            node_id = node.get("id")
            if node_id:
                if node_id in node_ids:
                    result.add_error("DUPLICATE_NODE_ID", f"Duplicate node ID: {node_id}", node_id=node_id)
                node_ids.add(node_id)
            
            # 统计触发器
            node_type = node.get("data", {}).get("type", "")
            if node_type.startswith("trigger_") or node_type in ["mission_start", "timer", "battery_low", "target_detected"]:
                trigger_count += 1
        
        # 3. 检查触发器
        if trigger_count == 0:
            result.add_error("NO_TRIGGER", "Flow must have at least one trigger node")
        
        # 4. 验证边
        for edge in edges:
            self._validate_edge(edge, node_ids, result)
        
        # 5. 检查孤立节点
        connected_nodes = self._get_connected_nodes(edges)
        for node in nodes:
            node_id = node.get("id")
            node_type = node.get("data", {}).get("type", "")
            if node_id and node_id not in connected_nodes:
                # 触发器可以是孤立的（作为起点）
                if not (node_type.startswith("trigger_") or node_type in ["mission_start", "timer"]):
                    result.add_warning("DISCONNECTED_NODE", f"Node '{node.get('data', {}).get('label', node_id)}' is not connected", node_id=node_id)
        
        # 6. 检测循环依赖
        cycles = self._detect_cycles(nodes, edges)
        if cycles:
            for cycle in cycles:
                cycle_str = " -> ".join(cycle)
                result.add_error("CIRCULAR_DEPENDENCY", f"Circular dependency detected: {cycle_str}")
        
        return result
    
    def _validate_node(self, node: Dict, result: ValidationResult):
        """验证单个节点"""
        node_id = node.get("id")
        if not node_id:
            result.add_error("MISSING_NODE_ID", "Node is missing 'id' field")
            return
        
        data = node.get("data", {})
        node_type = data.get("type", "")
        
        if not node_type:
            result.add_error("MISSING_NODE_TYPE", "Node is missing 'type' field", node_id=node_id)
            return
        
        # 验证节点类型
        if node_type not in VALID_NODE_TYPES:
            result.add_warning("UNKNOWN_NODE_TYPE", f"Unknown node type: {node_type}", node_id=node_id)
        
        # 验证参数
        config = data.get("config", {})
        if node_type in REQUIRED_PARAMS:
            for param in REQUIRED_PARAMS[node_type]:
                if param not in config or config[param] is None:
                    result.add_error("MISSING_REQUIRED_PARAM", f"Missing required parameter: {param}", node_id=node_id, field=param)
        
        # 验证特定节点类型
        if node_type == "search_area":
            self._validate_search_area_node(config, node_id, result)
        elif node_type == "goto_waypoint":
            self._validate_goto_node(config, node_id, result)
    
    def _validate_search_area_node(self, config: Dict, node_id: str, result: ValidationResult):
        """验证搜索区域节点"""
        area = config.get("area", [])
        if not area or len(area) < 3:
            result.add_error("INVALID_AREA", "Search area must have at least 3 points", node_id=node_id, field="area")
        
        altitude = config.get("altitude")
        if altitude is not None:
            if altitude < 10 or altitude > 500:
                result.add_warning("ALTITUDE_OUT_OF_RANGE", f"Altitude {altitude} is outside recommended range (10-500m)", node_id=node_id, field="altitude")
        
        speed = config.get("speed")
        if speed is not None:
            if speed < 1 or speed > 20:
                result.add_warning("SPEED_OUT_OF_RANGE", f"Speed {speed} is outside recommended range (1-20m/s)", node_id=node_id, field="speed")
    
    def _validate_goto_node(self, config: Dict, node_id: str, result: ValidationResult):
        """验证前往航点节点"""
        lat = config.get("lat")
        lng = config.get("lng")
        
        if lat is not None and (lat < -90 or lat > 90):
            result.add_error("INVALID_LATITUDE", f"Invalid latitude: {lat}", node_id=node_id, field="lat")
        
        if lng is not None and (lng < -180 or lng > 180):
            result.add_error("INVALID_LONGITUDE", f"Invalid longitude: {lng}", node_id=node_id, field="lng")
    
    def _validate_edge(self, edge: Dict, node_ids: Set[str], result: ValidationResult):
        """验证边"""
        edge_id = edge.get("id")
        source = edge.get("source")
        target = edge.get("target")
        
        if not source:
            result.add_error("MISSING_EDGE_SOURCE", "Edge is missing 'source' field", edge_id=edge_id)
        elif source not in node_ids:
            result.add_error("INVALID_EDGE_SOURCE", f"Edge source node not found: {source}", edge_id=edge_id)
        
        if not target:
            result.add_error("MISSING_EDGE_TARGET", "Edge is missing 'target' field", edge_id=edge_id)
        elif target not in node_ids:
            result.add_error("INVALID_EDGE_TARGET", f"Edge target node not found: {target}", edge_id=edge_id)
        
        if source and target and source == target:
            result.add_warning("SELF_LOOP", f"Edge connects node to itself: {source}", edge_id=edge_id)
    
    def _get_connected_nodes(self, edges: List[Dict]) -> Set[str]:
        """获取所有连接的节点 ID"""
        connected = set()
        for edge in edges:
            if edge.get("source"):
                connected.add(edge["source"])
            if edge.get("target"):
                connected.add(edge["target"])
        return connected
    
    def _detect_cycles(self, nodes: List[Dict], edges: List[Dict]) -> List[List[str]]:
        """使用 DFS 检测循环依赖"""
        # 构建邻接表
        adjacency = {}
        node_id_set = set()
        
        for node in nodes:
            node_id = node.get("id")
            if node_id:
                adjacency[node_id] = []
                node_id_set.add(node_id)
        
        for edge in edges:
            source = edge.get("source")
            target = edge.get("target")
            if source and target and source in node_id_set and target in node_id_set:
                adjacency[source].append(target)
        
        # DFS 检测环
        cycles = []
        visited = set()
        rec_stack = set()
        path = []
        
        def dfs(node_id):
            visited.add(node_id)
            rec_stack.add(node_id)
            path.append(node_id)
            
            for neighbor in adjacency.get(node_id, []):
                if neighbor not in visited:
                    dfs(neighbor)
                elif neighbor in rec_stack:
                    # 发现环
                    cycle_start = path.index(neighbor)
                    cycles.append(path[cycle_start:] + [neighbor])
            
            path.pop()
            rec_stack.remove(node_id)
        
        for node_id in node_id_set:
            if node_id not in visited:
                dfs(node_id)
        
        return cycles


# 单例实例
_validator: Optional[FlowValidator] = None


def get_validator() -> FlowValidator:
    """获取 FlowValidator 单例"""
    global _validator
    if _validator is None:
        _validator = FlowValidator()
    return _validator


def validate_flow(nodes: List[Dict], edges: List[Dict]) -> ValidationResult:
    """
    便捷函数：验证 Flow
    
    Args:
        nodes: 节点列表
        edges: 边列表
        
    Returns:
        ValidationResult
    """
    return get_validator().validate(nodes, edges)
