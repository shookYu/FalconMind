"""
Flow 格式转换工具

统一 Console 和 Builder 的 Flow 数据格式
"""

from typing import Dict, List, Any, Optional
from datetime import datetime


class FlowConverter:
    """Flow 格式转换器"""
    
    @staticmethod
    def console_to_builder(console_flow: Dict[str, Any]) -> Dict[str, Any]:
        """
        将 Console Flow 格式转换为 Builder 标准格式
        
        Args:
            console_flow: Console 的 Flow 数据
            
        Returns:
            Builder 标准格式的 Flow 数据
        """
        definition = console_flow.get('definition', {})
        
        # 转换边线字段名 connections -> edges
        edges = definition.get('connections', [])
        
        # 转换边线格式
        converted_edges = [
            FlowConverter._convert_edge(edge)
            for edge in edges
        ]
        
        return {
            'id': str(console_flow.get('id', '')),
            'name': console_flow.get('name', ''),
            'description': console_flow.get('description'),
            'version': console_flow.get('version', '1.0'),
            'nodes': definition.get('nodes', []),
            'edges': converted_edges,
            'created_at': FlowConverter._format_timestamp(
                console_flow.get('created_at')
            ),
            'updated_at': FlowConverter._format_timestamp(
                console_flow.get('updated_at')
            ),
            'created_by': str(console_flow.get('created_by')) if console_flow.get('created_by') else None,
            'project_id': console_flow.get('mission_id'),  # mission_id 映射为 project_id
            'is_template': console_flow.get('is_template', False),
            'metadata': {
                'source': 'console',
                'source_block_id': console_flow.get('source_block_id')
            }
        }
    
    @staticmethod
    def builder_to_console(builder_flow: Dict[str, Any], 
                          mission_id: Optional[str] = None) -> Dict[str, Any]:
        """
        将 Builder Flow 格式转换为 Console 格式
        
        Args:
            builder_flow: Builder 的 Flow 数据
            mission_id: Console 的任务ID（可选）
            
        Returns:
            Console 格式的 Flow 数据
        """
        # 转换边线格式 edges -> connections
        edges = builder_flow.get('edges', [])
        connections = [
            FlowConverter._convert_edge_to_connection(edge)
            for edge in edges
        ]
        
        return {
            'id': builder_flow.get('id'),
            'name': builder_flow.get('name'),
            'description': builder_flow.get('description'),
            'version': builder_flow.get('version', '1.0'),
            'definition': {
                'nodes': builder_flow.get('nodes', []),
                'connections': connections,
                'viewport': builder_flow.get('viewport', {
                    'x': 0, 'y': 0, 'zoom': 1
                })
            },
            'mission_id': mission_id or builder_flow.get('project_id'),
            'created_at': builder_flow.get('created_at'),
            'updated_at': builder_flow.get('updated_at'),
            'created_by': builder_flow.get('created_by'),
            'is_template': builder_flow.get('is_template', False),
            'source_block_id': builder_flow.get('metadata', {}).get('source_block_id')
        }
    
    @staticmethod
    def _convert_edge(edge: Dict[str, Any]) -> Dict[str, Any]:
        """转换边线格式 Console -> Builder"""
        return {
            'id': edge.get('id'),
            'source': edge.get('source'),
            'target': edge.get('target'),
            'sourceHandle': edge.get('source_handle') or edge.get('sourceHandle'),
            'targetHandle': edge.get('target_handle') or edge.get('targetHandle'),
            'type': edge.get('type', 'default'),
            'animated': edge.get('animated', False),
            'label': edge.get('label'),
            'style': edge.get('style', {})
        }
    
    @staticmethod
    def _convert_edge_to_connection(edge: Dict[str, Any]) -> Dict[str, Any]:
        """转换边线格式 Builder -> Console"""
        return {
            'id': edge.get('id'),
            'source': edge.get('source'),
            'target': edge.get('target'),
            'source_handle': edge.get('sourceHandle') or edge.get('source_handle'),
            'target_handle': edge.get('targetHandle') or edge.get('target_handle'),
            'type': edge.get('type'),
            'animated': edge.get('animated'),
            'label': edge.get('label')
        }
    
    @staticmethod
    def _format_timestamp(ts: Any) -> str:
        """格式化时间戳"""
        if isinstance(ts, datetime):
            return ts.isoformat()
        elif isinstance(ts, str):
            return ts
        else:
            return datetime.utcnow().isoformat()
    
    @staticmethod
    def convert_nodes(nodes: List[Dict[str, Any]], target_format: str = 'builder') -> List[Dict[str, Any]]:
        """
        转换节点数组格式
        
        Args:
            nodes: 节点数组
            target_format: 目标格式 'builder' 或 'console'
            
        Returns:
            转换后的节点数组
        """
        if target_format == 'builder':
            return [FlowConverter._convert_node_to_builder(n) for n in nodes]
        else:
            return [FlowConverter._convert_node_to_console(n) for n in nodes]
    
    @staticmethod
    def _convert_node_to_builder(node: Dict[str, Any]) -> Dict[str, Any]:
        """转换节点 Console -> Builder"""
        data = node.get('data', {})
        return {
            'id': node.get('id'),
            'type': node.get('type'),
            'position': node.get('position', {'x': 0, 'y': 0}),
            'data': {
                'type': data.get('type') or node.get('subtype'),
                'label': data.get('label', node.get('id')),
                'config': data.get('config') or data.get('parameters', {})
            },
            'width': node.get('width'),
            'height': node.get('height'),
            'selected': node.get('selected', False)
        }
    
    @staticmethod
    def _convert_node_to_console(node: Dict[str, Any]) -> Dict[str, Any]:
        """转换节点 Builder -> Console"""
        data = node.get('data', {})
        return {
            'id': node.get('id'),
            'type': node.get('type'),
            'subtype': data.get('type'),
            'position': node.get('position'),
            'data': {
                'type': data.get('type'),
                'label': data.get('label'),
                'parameters': data.get('config', {})
            },
            'width': node.get('width'),
            'height': node.get('height')
        }
    
    @staticmethod
    def to_sdk_format(flow: Dict[str, Any]) -> Dict[str, Any]:
        """
        转换为 SDK FlowExecutor 格式
        
        Args:
            flow: 标准格式的 Flow
            
        Returns:
            SDK 执行格式
        """
        nodes = flow.get('nodes', [])
        edges = flow.get('edges', [])
        
        return {
            'flow_id': flow.get('id'),
            'name': flow.get('name'),
            'version': flow.get('version', '1.0'),
            'nodes': [
                {
                    'node_id': node['id'],
                    'template_id': node.get('data', {}).get('type', ''),
                    'parameters': node.get('data', {}).get('config', {})
                }
                for node in nodes
            ],
            'edges': [
                {
                    'edge_id': edge['id'],
                    'from_node_id': edge['source'],
                    'to_node_id': edge['target'],
                    'condition': edge.get('label')
                }
                for edge in edges
            ],
            'execution_config': {
                'max_execution_time': 3600,
                'retry_count': 3,
                'on_failure': 'abort'
            }
        }


class FlowValidator:
    """Flow 验证器"""
    
    @staticmethod
    def validate(flow: Dict[str, Any]) -> Dict[str, Any]:
        """
        验证 Flow 数据格式
        
        Args:
            flow: Flow 数据
            
        Returns:
            验证结果 { valid: bool, errors: [] }
        """
        errors = []
        
        # 必填字段检查
        if not flow.get('id'):
            errors.append({'field': 'id', 'message': 'Flow ID 不能为空'})
        
        if not flow.get('name'):
            errors.append({'field': 'name', 'message': 'Flow 名称不能为空'})
        
        nodes = flow.get('nodes', [])
        edges = flow.get('edges', [])
        
        # 节点检查
        if not nodes:
            errors.append({'field': 'nodes', 'message': 'Flow 必须包含至少一个节点'})
        
        # 触发器检查
        triggers = [n for n in nodes if n.get('type') == 'trigger']
        if not triggers:
            errors.append({'field': 'nodes', 'message': 'Flow 必须包含一个触发器节点'})
        
        # 连接检查
        node_ids = {n.get('id') for n in nodes}
        for edge in edges:
            if edge.get('source') not in node_ids:
                errors.append({
                    'field': f'edges.{edge.get("id")}.source',
                    'message': f'边线引用了不存在的源节点: {edge.get("source")}'
                })
            if edge.get('target') not in node_ids:
                errors.append({
                    'field': f'edges.{edge.get("id")}.target',
                    'message': f'边线引用了不存在的目标节点: {edge.get("target")}'
                })
        
        # 搜索区域参数检查
        for node in nodes:
            if node.get('data', {}).get('type') == 'search_area':
                area = node.get('data', {}).get('config', {}).get('area', [])
                if len(area) < 3:
                    errors.append({
                        'field': f'nodes.{node.get("id")}.data.config.area',
                        'message': '搜索区域必须包含至少3个点'
                    })
                
                altitude = node.get('data', {}).get('config', {}).get('altitude')
                if altitude is not None and (altitude < 10 or altitude > 500):
                    errors.append({
                        'field': f'nodes.{node.get("id")}.data.config.altitude',
                        'message': '飞行高度必须在 10-500 米之间'
                    })
        
        return {
            'valid': len(errors) == 0,
            'errors': errors
        }


# 快捷函数
def convert_console_to_builder(console_flow: Dict[str, Any]) -> Dict[str, Any]:
    """快捷函数：Console -> Builder"""
    return FlowConverter.console_to_builder(console_flow)


def convert_builder_to_console(builder_flow: Dict[str, Any], 
                               mission_id: Optional[str] = None) -> Dict[str, Any]:
    """快捷函数：Builder -> Console"""
    return FlowConverter.builder_to_console(builder_flow, mission_id)


def validate_flow(flow: Dict[str, Any]) -> Dict[str, Any]:
    """快捷函数：验证 Flow"""
    return FlowValidator.validate(flow)
