"""
Custom Node Types Registry for FalconMindBuilder

Defines all P0 custom node types for PoC Scenario_01:
- VINSStatusCheck
- GPSDefenseActivator  
- VisualDetector
- VisualServoController
- SearchPatternGenerator
- TargetAwaiter
"""

from typing import Dict, List, Any, Optional
from pydantic import BaseModel, Field
from enum import Enum


class NodeCategory(str, Enum):
    """Node categories"""
    NAVIGATION = "navigation"
    PERCEPTION = "perception"
    GUIDANCE = "guidance"
    CONTROL = "control"
    UTILITY = "utility"


class ParameterDefinition(BaseModel):
    """Parameter definition for node"""
    name: str
    type: str = "string"  # string, int, float, bool, array, object
    description: str = ""
    default: Any = None
    required: bool = True
    min_value: Optional[float] = None
    max_value: Optional[float] = None
    options: Optional[List[str]] = None  # For enum types


class NodePortDefinition(BaseModel):
    """Input/Output port definition"""
    name: str
    type: str = "any"  # any, detection_array, tracking_array, navigation_state, etc.
    description: str = ""
    required: bool = True


class CustomNodeType(BaseModel):
    """Custom node type definition"""
    id: str  # Unique identifier
    name: str  # Display name
    description: str = ""
    category: NodeCategory
    icon: str = "default"  # Icon name for UI
    color: str = "#1890ff"  # Node color in UI
    
    # Process mapping
    sdk_process: str  # Maps to which SDK process
    dds_publishes: List[str] = []  # DDS topics published
    dds_subscribes: List[str] = []  # DDS topics subscribed
    
    # Node properties
    is_background: bool = False  # Runs continuously (e.g., VisualServoController)
    can_be_start: bool = True  # Can be a starting node
    can_be_end: bool = False  # Can be an ending node
    
    # Parameters
    parameters: List[ParameterDefinition] = []
    
    # Ports
    inputs: List[NodePortDefinition] = []
    outputs: List[NodePortDefinition] = []
    
    # Validation
    validation_rules: Dict[str, Any] = Field(default_factory=dict)


# =============================================================================
# P0 Custom Node Types Definition
# =============================================================================

CUSTOM_NODE_TYPES: Dict[str, CustomNodeType] = {
    # =========================================================================
    # Navigation Nodes
    # =========================================================================
    "VINSStatusCheck": CustomNodeType(
        id="VINSStatusCheck",
        name="VINS Status Check",
        description="Check VINS (Visual-Inertial Navigation System) initialization status",
        category=NodeCategory.NAVIGATION,
        icon="compass",
        color="#52c41a",
        sdk_process="vins_slam_process",
        dds_subscribes=["NavigationState"],
        can_be_start=True,
        can_be_end=False,
        parameters=[
            ParameterDefinition(
                name="min_confidence",
                type="float",
                description="Minimum position confidence to consider VINS ready",
                default=0.8,
                min_value=0.0,
                max_value=1.0
            ),
            ParameterDefinition(
                name="timeout_seconds",
                type="int",
                description="Maximum time to wait for VINS initialization",
                default=30,
                min_value=1,
                max_value=300
            )
        ],
        inputs=[
            NodePortDefinition(name="trigger", type="trigger", description="Start checking")
        ],
        outputs=[
            NodePortDefinition(name="ready", type="boolean", description="VINS is ready"),
            NodePortDefinition(name="not_ready", type="boolean", description="VINS not ready or timeout")
        ]
    ),
    
    "GPSDefenseActivator": CustomNodeType(
        id="GPSDefenseActivator",
        name="GPS Defense Activator",
        description="Activate GPS spoofing detection and protection",
        category=NodeCategory.NAVIGATION,
        icon="shield",
        color="#fa8c16",
        sdk_process="gps_defense_process",
        dds_subscribes=["NavigationState"],
        dds_publishes=["GPSDefenseStatus"],
        is_background=True,
        can_be_start=False,
        parameters=[
            ParameterDefinition(
                name="raim_check",
                type="bool",
                description="Enable RAIM (Receiver Autonomous Integrity Monitoring)",
                default=True
            ),
            ParameterDefinition(
                name="imu_consistency_check",
                type="bool",
                description="Check IMU consistency with GPS",
                default=True
            ),
            ParameterDefinition(
                name="vins_cross_check",
                type="bool",
                description="Cross-check with VINS position",
                default=True
            ),
            ParameterDefinition(
                name="alert_threshold",
                type="string",
                description="Alert threshold level",
                default="SUSPECTED",
                options=["SUSPECTED", "CONFIRMED", "CRITICAL"]
            )
        ],
        inputs=[
            NodePortDefinition(name="enable", type="trigger", description="Enable GPS defense")
        ],
        outputs=[
            NodePortDefinition(name="active", type="boolean", description="Defense is active"),
            NodePortDefinition(name="status", type="object", description="GPS defense status")
        ]
    ),
    
    # =========================================================================
    # Perception Nodes
    # =========================================================================
    "VisualDetector": CustomNodeType(
        id="VisualDetector",
        name="Visual Detector",
        description="Run YOLO object detection and DeepSORT tracking",
        category=NodeCategory.PERCEPTION,
        icon="eye",
        color="#1890ff",
        sdk_process="perception_process",
        dds_publishes=["DetectionArray", "TrackingArray"],
        is_background=True,
        can_be_start=False,
        parameters=[
            ParameterDefinition(
                name="model",
                type="string",
                description="YOLO model to use",
                default="yolov8n",
                options=["yolov8n", "yolov8s", "yolov8m", "yolov8l"]
            ),
            ParameterDefinition(
                name="classes",
                type="array",
                description="Target classes to detect",
                default=["person", "vehicle"]
            ),
            ParameterDefinition(
                name="confidence_threshold",
                type="float",
                description="Minimum detection confidence",
                default=0.6,
                min_value=0.0,
                max_value=1.0
            ),
            ParameterDefinition(
                name="nms_threshold",
                type="float",
                description="Non-maximum suppression threshold",
                default=0.45,
                min_value=0.0,
                max_value=1.0
            ),
            ParameterDefinition(
                name="enable_tracking",
                type="bool",
                description="Enable DeepSORT tracking",
                default=True
            ),
            ParameterDefinition(
                name="use_npu",
                type="bool",
                description="Use RK3588 NPU for inference",
                default=True
            )
        ],
        inputs=[
            NodePortDefinition(name="start", type="trigger", description="Start detection")
        ],
        outputs=[
            NodePortDefinition(name="detections", type="detection_array", description="Detection results"),
            NodePortDefinition(name="tracking", type="tracking_array", description="Tracking results")
        ]
    ),
    
    # =========================================================================
    # Guidance Nodes
    # =========================================================================
    "VisualServoController": CustomNodeType(
        id="VisualServoController",
        name="Visual Servo Controller",
        description="IBVS (Image-Based Visual Servoing) controller for target tracking",
        category=NodeCategory.GUIDANCE,
        icon="control",
        color="#722ed1",
        sdk_process="guidance_process",
        dds_subscribes=["TrackingArray"],
        dds_publishes=["GuidanceCommand"],
        is_background=True,
        can_be_start=False,
        parameters=[
            ParameterDefinition(
                name="control_mode",
                type="string",
                description="Control mode",
                default="IBVS",
                options=["IBVS", "PBVS", "HYBRID"]
            ),
            ParameterDefinition(
                name="update_rate",
                type="int",
                description="Control update rate (Hz)",
                default=20,
                min_value=1,
                max_value=50
            ),
            ParameterDefinition(
                name="desired_distance",
                type="float",
                description="Desired distance to target (m)",
                default=30.0,
                min_value=5.0,
                max_value=100.0
            ),
            ParameterDefinition(
                name="desired_height",
                type="float",
                description="Desired height above target (m)",
                default=10.0,
                min_value=5.0,
                max_value=50.0
            ),
            ParameterDefinition(
                name="kp_distance",
                type="float",
                description="Proportional gain for distance control",
                default=0.5,
                min_value=0.0,
                max_value=5.0
            ),
            ParameterDefinition(
                name="ki_distance",
                type="float",
                description="Integral gain for distance control",
                default=0.1,
                min_value=0.0,
                max_value=1.0
            ),
            ParameterDefinition(
                name="kd_distance",
                type="float",
                description="Derivative gain for distance control",
                default=0.2,
                min_value=0.0,
                max_value=1.0
            ),
            ParameterDefinition(
                name="enable_adaptive_gain",
                type="bool",
                description="Enable adaptive gain based on tracking quality",
                default=True
            )
        ],
        inputs=[
            NodePortDefinition(name="target_id", type="int", description="Target track ID to follow"),
            NodePortDefinition(name="config", type="object", description="Controller configuration")
        ],
        outputs=[
            NodePortDefinition(name="commands", type="guidance_command", description="Velocity commands"),
            NodePortDefinition(name="active", type="boolean", description="Controller is active"),
            NodePortDefinition(name="rate", type="float", description="Actual control rate (Hz)")
        ]
    ),
    
    # =========================================================================
    # Control Nodes
    # =========================================================================
    "SearchPatternGenerator": CustomNodeType(
        id="SearchPatternGenerator",
        name="Search Pattern Generator",
        description="Generate search waypoints for area coverage",
        category=NodeCategory.CONTROL,
        icon="search",
        color="#13c2c2",
        sdk_process="mission_planner_process",
        dds_publishes=["WaypointArray"],
        can_be_start=False,
        parameters=[
            ParameterDefinition(
                name="pattern",
                type="string",
                description="Search pattern type",
                default="LAWN_MOWER",
                options=["LAWN_MOWER", "SPIRAL", "ZIGZAG", "RANDOM"]
            ),
            ParameterDefinition(
                name="altitude",
                type="float",
                description="Search altitude (m)",
                default=50.0,
                min_value=10.0,
                max_value=120.0
            ),
            ParameterDefinition(
                name="speed",
                type="float",
                description="Search speed (m/s)",
                default=5.0,
                min_value=1.0,
                max_value=15.0
            ),
            ParameterDefinition(
                name="spacing",
                type="float",
                description="Spacing between search tracks (m)",
                default=20.0,
                min_value=5.0,
                max_value=100.0
            ),
            ParameterDefinition(
                name="overlap_rate",
                type="float",
                description="Overlap between adjacent tracks (0-1)",
                default=0.2,
                min_value=0.0,
                max_value=0.5
            ),
            ParameterDefinition(
                name="turn_radius",
                type="float",
                description="Minimum turn radius (m)",
                default=10.0,
                min_value=5.0,
                max_value=50.0
            )
        ],
        inputs=[
            NodePortDefinition(name="area", type="object", description="Search area polygon"),
            NodePortDefinition(name="trigger", type="trigger", description="Generate waypoints")
        ],
        outputs=[
            NodePortDefinition(name="waypoints", type="waypoint_array", description="Generated waypoints"),
            NodePortDefinition(name="count", type="int", description="Number of waypoints")
        ]
    ),
    
    "TargetAwaiter": CustomNodeType(
        id="TargetAwaiter",
        name="Target Awaiter",
        description="Wait for operator to select target from detection list",
        category=NodeCategory.CONTROL,
        icon="user",
        color="#eb2f96",
        sdk_process="mission_planner_process",
        dds_subscribes=["DetectionArray"],
        can_be_start=False,
        parameters=[
            ParameterDefinition(
                name="timeout_seconds",
                type="int",
                description="Maximum time to wait for selection (0 = infinite)",
                default=300,
                min_value=0,
                max_value=3600
            ),
            ParameterDefinition(
                name="target_classes",
                type="array",
                description="Acceptable target classes",
                default=["person"]
            ),
            ParameterDefinition(
                name="min_confidence",
                type="float",
                description="Minimum detection confidence",
                default=0.7,
                min_value=0.0,
                max_value=1.0
            )
        ],
        inputs=[
            NodePortDefinition(name="detections", type="detection_array", description="Available detections"),
            NodePortDefinition(name="trigger", type="trigger", description="Start waiting")
        ],
        outputs=[
            NodePortDefinition(name="selected", type="object", description="Selected target info"),
            NodePortDefinition(name="timeout", type="boolean", description="Timeout occurred")
        ]
    ),
}


# =============================================================================
# Helper Functions
# =============================================================================

def get_all_node_types() -> List[CustomNodeType]:
    """Get all custom node types"""
    return list(CUSTOM_NODE_TYPES.values())


def get_node_type(node_id: str) -> Optional[CustomNodeType]:
    """Get a specific node type by ID"""
    return CUSTOM_NODE_TYPES.get(node_id)


def get_node_types_by_category(category: NodeCategory) -> List[CustomNodeType]:
    """Get node types filtered by category"""
    return [node for node in CUSTOM_NODE_TYPES.values() if node.category == category]


def get_node_types_by_process(process_name: str) -> List[CustomNodeType]:
    """Get node types that map to a specific SDK process"""
    return [node for node in CUSTOM_NODE_TYPES.values() if node.sdk_process == process_name]


def validate_node_parameters(node_id: str, parameters: Dict[str, Any]) -> tuple[bool, List[str]]:
    """Validate parameters for a node type
    
    Returns:
        (is_valid, list_of_errors)
    """
    node_type = get_node_type(node_id)
    if not node_type:
        return False, [f"Unknown node type: {node_id}"]
    
    errors = []
    
    # Check required parameters
    for param_def in node_type.parameters:
        if param_def.required and param_def.name not in parameters:
            errors.append(f"Missing required parameter: {param_def.name}")
            continue
        
        if param_def.name in parameters:
            value = parameters[param_def.name]
            
            # Type validation
            if param_def.type == "int" and not isinstance(value, int):
                errors.append(f"Parameter {param_def.name} must be an integer")
            elif param_def.type == "float" and not isinstance(value, (int, float)):
                errors.append(f"Parameter {param_def.name} must be a number")
            elif param_def.type == "bool" and not isinstance(value, bool):
                errors.append(f"Parameter {param_def.name} must be a boolean")
            elif param_def.type == "string" and not isinstance(value, str):
                errors.append(f"Parameter {param_def.name} must be a string")
            
            # Range validation
            if param_def.min_value is not None and value < param_def.min_value:
                errors.append(f"Parameter {param_def.name} must be >= {param_def.min_value}")
            if param_def.max_value is not None and value > param_def.max_value:
                errors.append(f"Parameter {param_def.name} must be <= {param_def.max_value}")
            
            # Options validation
            if param_def.options and value not in param_def.options:
                errors.append(f"Parameter {param_def.name} must be one of: {param_def.options}")
    
    return len(errors) == 0, errors
