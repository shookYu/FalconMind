"""
Flow-to-Process Mapping Service

Maps Flow nodes to SDK business processes and generates process configurations.

Architecture:
    Flow JSON (Builder) -> FlowToProcessMapper -> Process Configurations
                                    -> DDS Topic Subscriptions
                                    -> MQTT Command Topics
"""

from typing import Dict, List, Any, Optional, Set
from pydantic import BaseModel
import json
from datetime import datetime

from ..core.node_types import (
    get_node_type, 
    validate_node_parameters,
    CUSTOM_NODE_TYPES
)


class ProcessConfig(BaseModel):
    """Configuration for a single SDK process"""
    process_name: str
    enabled: bool = True
    auto_start: bool = True
    parameters: Dict[str, Any] = {}
    dds_subscriptions: List[str] = []
    dds_publications: List[str] = []
    mqtt_subscriptions: List[str] = []
    mqtt_publications: List[str] = []
    depends_on: List[str] = []


class FlowDeploymentConfig(BaseModel):
    """Complete deployment configuration for a Flow"""
    flow_id: str
    flow_name: str
    mission_id: str
    timestamp: str
    processes: Dict[str, ProcessConfig]
    dds_domain_id: str = "0"
    mqtt_broker: str = "localhost"
    mqtt_port: int = 1883


class NodeExecutionOrder(BaseModel):
    """Defines execution order of nodes"""
    node_id: str
    node_type: str
    sdk_process: str
    execution_order: int
    is_background: bool = False
    background_starts_after: Optional[str] = None


class FlowToProcessMapper:
    """Maps Flow nodes to SDK processes"""
    
    # Process dependency graph
    PROCESS_DEPENDENCIES = {
        "vins_slam_process": [],  # No dependencies, starts first
        "gps_defense_process": ["vins_slam_process"],
        "perception_process": ["vins_slam_process"],
        "video_capture_process": [],  # Hardware, independent
        "guidance_process": ["perception_process", "vins_slam_process"],
        "mission_planner_process": ["vins_slam_process", "perception_process", "gps_defense_process"],
        "flight_control_process": ["guidance_process"],
        "data_logger_process": [],  # Can run standalone
        "system_manager_process": []  # Special, monitors others
    }
    
    # Default process priorities (lower = starts first)
    PROCESS_PRIORITIES = {
        "system_manager_process": 5,
        "video_capture_process": 10,
        "vins_slam_process": 15,
        "perception_process": 20,
        "gps_defense_process": 25,
        "mission_planner_process": 30,
        "guidance_process": 35,
        "flight_control_process": 40,
        "data_logger_process": 50
    }
    
    def __init__(self):
        self.process_configs: Dict[str, ProcessConfig] = {}
        self.execution_order: List[NodeExecutionOrder] = []
    
    def map_flow_to_processes(self, flow_data: Dict[str, Any]) -> FlowDeploymentConfig:
        """
        Map a Flow JSON to process configurations
        
        Args:
            flow_data: Flow JSON with nodes and edges
            
        Returns:
            FlowDeploymentConfig with process configurations
        """
        flow_id = flow_data.get("id", "unknown")
        flow_name = flow_data.get("name", "Unnamed Flow")
        nodes = flow_data.get("nodes", [])
        edges = flow_data.get("edges", [])
        
        # Reset state
        self.process_configs = {}
        self.execution_order = []
        
        # Step 1: Analyze nodes and build execution order
        self._analyze_nodes(nodes)
        
        # Step 2: Map nodes to processes
        self._map_nodes_to_processes(nodes)
        
        # Step 3: Resolve DDS/MQTT topics
        self._resolve_communication_topics(nodes)
        
        # Step 4: Resolve process dependencies
        self._resolve_dependencies()
        
        # Step 5: Generate deployment config
        deployment = FlowDeploymentConfig(
            flow_id=flow_id,
            flow_name=flow_name,
            mission_id=f"mission_{flow_id}_{datetime.now().strftime('%Y%m%d_%H%M%S')}",
            timestamp=datetime.now().isoformat(),
            processes=self.process_configs
        )
        
        return deployment
    
    def _analyze_nodes(self, nodes: List[Dict[str, Any]]):
        """Analyze nodes and determine execution order"""
        # Topological sort based on edges
        # For now, simple sequential ordering
        for idx, node in enumerate(nodes):
            node_id = node.get("id", f"node_{idx}")
            node_type_id = node.get("data", {}).get("type", "")
            
            # Get custom node type definition
            custom_type = get_node_type(node_type_id)
            if custom_type:
                execution_order = NodeExecutionOrder(
                    node_id=node_id,
                    node_type=node_type_id,
                    sdk_process=custom_type.sdk_process,
                    execution_order=idx,
                    is_background=custom_type.is_background
                )
                self.execution_order.append(execution_order)
    
    def _map_nodes_to_processes(self, nodes: List[Dict[str, Any]]):
        """Map nodes to SDK process configurations"""
        for node in nodes:
            node_id = node.get("id", "")
            node_type_id = node.get("data", {}).get("type", "")
            node_config = node.get("data", {}).get("config", {})
            
            # Get custom node type
            custom_type = get_node_type(node_type_id)
            if not custom_type:
                continue
            
            # Validate parameters
            is_valid, errors = validate_node_parameters(node_type_id, node_config)
            if not is_valid:
                raise ValueError(f"Invalid parameters for node {node_id}: {errors}")
            
            # Get or create process config
            process_name = custom_type.sdk_process
            if process_name not in self.process_configs:
                self.process_configs[process_name] = ProcessConfig(
                    process_name=process_name
                )
            
            # Merge node parameters into process config
            self._merge_node_parameters(process_name, node_type_id, node_config)
    
    def _merge_node_parameters(self, process_name: str, node_type_id: str, 
                               node_config: Dict[str, Any]):
        """Merge node parameters into process configuration"""
        custom_type = get_node_type(node_type_id)
        if not custom_type:
            return
        
        process_config = self.process_configs[process_name]
        
        # Add node-specific parameters
        if node_type_id == "VINSStatusCheck":
            process_config.parameters["vins_check"] = {
                "min_confidence": node_config.get("min_confidence", 0.8),
                "timeout_seconds": node_config.get("timeout_seconds", 30)
            }
            
        elif node_type_id == "GPSDefenseActivator":
            process_config.parameters["gps_defense"] = {
                "raim_check": node_config.get("raim_check", True),
                "imu_consistency_check": node_config.get("imu_consistency_check", True),
                "vins_cross_check": node_config.get("vins_cross_check", True),
                "alert_threshold": node_config.get("alert_threshold", "SUSPECTED")
            }
            
        elif node_type_id == "VisualDetector":
            process_config.parameters["detection"] = {
                "model": node_config.get("model", "yolov8n"),
                "classes": node_config.get("classes", ["person", "vehicle"]),
                "confidence_threshold": node_config.get("confidence_threshold", 0.6),
                "nms_threshold": node_config.get("nms_threshold", 0.45),
                "enable_tracking": node_config.get("enable_tracking", True),
                "use_npu": node_config.get("use_npu", True)
            }
            
        elif node_type_id == "VisualServoController":
            process_config.parameters["guidance"] = {
                "control_mode": node_config.get("control_mode", "IBVS"),
                "update_rate": node_config.get("update_rate", 20),
                "desired_distance": node_config.get("desired_distance", 30.0),
                "desired_height": node_config.get("desired_height", 10.0),
                "pid_params": {
                    "kp": node_config.get("kp_distance", 0.5),
                    "ki": node_config.get("ki_distance", 0.1),
                    "kd": node_config.get("kd_distance", 0.2)
                },
                "enable_adaptive_gain": node_config.get("enable_adaptive_gain", True)
            }
            
        elif node_type_id == "SearchPatternGenerator":
            process_config.parameters["search"] = {
                "pattern": node_config.get("pattern", "LAWN_MOWER"),
                "altitude": node_config.get("altitude", 50.0),
                "speed": node_config.get("speed", 5.0),
                "spacing": node_config.get("spacing", 20.0),
                "overlap_rate": node_config.get("overlap_rate", 0.2),
                "turn_radius": node_config.get("turn_radius", 10.0)
            }
            
        elif node_type_id == "TargetAwaiter":
            process_config.parameters["target_selection"] = {
                "timeout_seconds": node_config.get("timeout_seconds", 300),
                "target_classes": node_config.get("target_classes", ["person"]),
                "min_confidence": node_config.get("min_confidence", 0.7)
            }
    
    def _resolve_communication_topics(self, nodes: List[Dict[str, Any]]):
        """Resolve DDS and MQTT topics based on node types"""
        all_dds_subscriptions: Set[str] = set()
        all_dds_publications: Set[str] = set()
        
        for node in nodes:
            node_type_id = node.get("data", {}).get("type", "")
            custom_type = get_node_type(node_type_id)
            if not custom_type:
                continue
            
            process_name = custom_type.sdk_process
            if process_name not in self.process_configs:
                continue
            
            process_config = self.process_configs[process_name]
            
            # Add DDS topics
            for topic in custom_type.dds_subscribes:
                if topic not in process_config.dds_subscriptions:
                    process_config.dds_subscriptions.append(topic)
                all_dds_subscriptions.add(topic)
            
            for topic in custom_type.dds_publishes:
                if topic not in process_config.dds_publications:
                    process_config.dds_publications.append(topic)
                all_dds_publications.add(topic)
        
        # Add cross-process subscriptions
        # If Process A publishes Topic X and Process B uses Node that subscribes to Topic X,
        # ensure Process B subscribes to Topic X
        for process_name, config in self.process_configs.items():
            for topic in all_dds_publications:
                if topic not in config.dds_subscriptions:
                    # Check if this process needs this topic based on its nodes
                    for node in nodes:
                        node_type_id = node.get("data", {}).get("type", "")
                        custom_type = get_node_type(node_type_id)
                        if custom_type and custom_type.sdk_process == process_name:
                            if topic in custom_type.dds_subscribes:
                                config.dds_subscriptions.append(topic)
    
    def _resolve_dependencies(self):
        """Resolve process dependencies"""
        for process_name, config in self.process_configs.items():
            if process_name in self.PROCESS_DEPENDENCIES:
                for dep in self.PROCESS_DEPENDENCIES[process_name]:
                    if dep in self.process_configs and dep not in config.depends_on:
                        config.depends_on.append(dep)
    
    def generate_supervisor_config(self, deployment: FlowDeploymentConfig) -> str:
        """Generate SupervisorD configuration from deployment"""
        config_lines = [
            "; FalconMind Process Configuration",
            "; Auto-generated from Flow deployment",
            f"; Flow: {deployment.flow_name}",
            f"; Generated: {deployment.timestamp}",
            ""
        ]
        
        # Sort processes by priority
        sorted_processes = sorted(
            deployment.processes.items(),
            key=lambda x: self.PROCESS_PRIORITIES.get(x[0], 100)
        )
        
        for process_name, process_config in sorted_processes:
            config_lines.extend([
                f"[program:{process_name.replace('_process', '')}]",
                f"command=/opt/falconmind/bin/{process_name} --config /etc/falconmind/{process_name}.yaml",
                f"autostart={str(process_config.auto_start).lower()}",
                "autorestart=true",
                "startretries=3",
                "user=falconmind",
                f"stdout_logfile=/var/log/falconmind/{process_name}.log",
                "stdout_logfile_maxbytes=10MB",
                "stdout_logfile_backups=5",
                f"stderr_logfile=/var/log/falconmind/{process_name}_error.log",
                f"environment=FALCONMIND_MISSION_ID=\"{deployment.mission_id}\"",
                f"priority={self.PROCESS_PRIORITIES.get(process_name, 50)}",
                ""
            ])
        
        return "\n".join(config_lines)
    
    def generate_process_yaml(self, process_name: str, 
                             deployment: FlowDeploymentConfig) -> str:
        """Generate YAML configuration for a specific process"""
        if process_name not in deployment.processes:
            return ""
        
        config = deployment.processes[process_name]
        
        yaml_dict = {
            "mission_id": deployment.mission_id,
            "flow_id": deployment.flow_id,
            "enabled": config.enabled,
            "parameters": config.parameters,
            "dds": {
                "domain_id": deployment.dds_domain_id,
                "subscriptions": config.dds_subscriptions,
                "publications": config.dds_publications
            },
            "mqtt": {
                "broker": deployment.mqtt_broker,
                "port": deployment.mqtt_port,
                "subscriptions": config.mqtt_subscriptions,
                "publications": config.mqtt_publications
            }
        }
        
        if config.depends_on:
            yaml_dict["depends_on"] = config.depends_on
        
        import yaml
        return yaml.dump(yaml_dict, default_flow_style=False)


def get_required_processes_for_flow(flow_data: Dict[str, Any]) -> List[str]:
    """Get list of required SDK processes for a Flow"""
    required_processes = set()
    
    for node in flow_data.get("nodes", []):
        node_type_id = node.get("data", {}).get("type", "")
        custom_type = get_node_type(node_type_id)
        if custom_type:
            required_processes.add(custom_type.sdk_process)
    
    # Add implicit dependencies
    mapper = FlowToProcessMapper()
    all_processes = set(required_processes)
    for proc in required_processes:
        if proc in mapper.PROCESS_DEPENDENCIES:
            all_processes.update(mapper.PROCESS_DEPENDENCIES[proc])
    
    # Sort by priority
    return sorted(all_processes, key=lambda x: mapper.PROCESS_PRIORITIES.get(x, 100))
