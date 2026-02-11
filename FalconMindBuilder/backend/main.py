"""
FalconMindBuilder Backend - 最小可用版
提供节点库管理、流程存储、代码生成功能
"""

from typing import Dict, List, Optional
from datetime import datetime
from enum import Enum
import json
import os
from pathlib import Path

from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel


# ========== 数据模型 ==========

class NodeCategory(str, Enum):
    FLIGHT = "FLIGHT"
    SENSORS = "SENSORS"
    PERCEPTION = "PERCEPTION"
    MISSION = "MISSION"
    LOGIC = "LOGIC"
    UTILITY = "UTILITY"


class PortType(str, Enum):
    IMAGE = "IMAGE"
    DETECTION = "DETECTION"
    TRACKING = "TRACKING"
    FLIGHT_STATE = "FLIGHT_STATE"
    FLIGHT_COMMAND = "FLIGHT_COMMAND"
    TELEMETRY = "TELEMETRY"
    ANY = "ANY"


class PortDefinition(BaseModel):
    name: str
    type: PortType
    description: Optional[str] = ""


class NodeTemplate(BaseModel):
    template_id: str
    name: str
    category: NodeCategory
    description: str = ""
    input_ports: List[PortDefinition] = []
    output_ports: List[PortDefinition] = []
    parameters_schema: Dict = {}  # JSON Schema 格式的参数定义
    sdk_class_name: str  # SDK 中的类名，如 "CameraSourceNode"


class FlowNode(BaseModel):
    node_id: str
    template_id: str
    position: Dict[str, float]  # {x, y}
    parameters: Dict = {}  # 节点参数值


class FlowEdge(BaseModel):
    edge_id: str
    from_node_id: str
    from_port: str
    to_node_id: str
    to_port: str


class FlowCreateRequest(BaseModel):
    """创建流程的请求模型（只包含用户提供的字段）"""
    name: str
    description: str = ""
    nodes: List[FlowNode] = []
    edges: List[FlowEdge] = []


class FlowDefinition(BaseModel):
    """流程定义模型（包含所有字段）"""
    flow_id: str
    name: str
    description: str = ""
    version: str = "1.0"  # 版本号（语义化版本，如 "1.0", "1.1", "2.0"）
    nodes: List[FlowNode] = []
    edges: List[FlowEdge] = []
    created_at: str
    updated_at: str
    version_comment: str = ""  # 版本注释（可选）


class ProjectCreateRequest(BaseModel):
    """创建工程的请求模型（只包含用户提供的字段）"""
    name: str
    description: str = ""


class ProjectInfo(BaseModel):
    """工程信息模型（包含所有字段）"""
    project_id: str
    name: str
    description: str = ""
    created_at: str
    updated_at: str


class FlowCreateRequest(BaseModel):
    """创建流程的请求模型（只包含用户提供的字段）"""
    name: str
    description: str = ""
    nodes: List[FlowNode] = []
    edges: List[FlowEdge] = []


# ========== 应用初始化 ==========

app = FastAPI(title="FalconMindBuilder Backend (Minimal)")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ========== 持久化存储配置 ==========

# 数据存储目录
DATA_DIR = Path(__file__).parent / "data"
DATA_DIR.mkdir(exist_ok=True)

# 数据文件路径
PROJECTS_FILE = DATA_DIR / "projects.json"
FLOWS_FILE = DATA_DIR / "flows.json"
FLOW_VERSIONS_FILE = DATA_DIR / "flow_versions.json"
COUNTERS_FILE = DATA_DIR / "counters.json"

# ========== 数据存储（支持持久化） ==========

# 节点模板库（内存，启动时初始化）
node_templates: Dict[str, NodeTemplate] = {}

# 工程和流程存储（支持持久化）
projects: Dict[str, ProjectInfo] = {}
flows: Dict[str, FlowDefinition] = {}  # flow_id -> FlowDefinition (当前版本)
flow_versions: Dict[str, List[FlowDefinition]] = {}  # flow_id -> [FlowDefinition, ...] (版本历史)
project_flows: Dict[str, List[str]] = {}  # project_id -> [flow_id, ...]

project_counter = 0
flow_counter = 0


# ========== 持久化存储函数 ==========

def save_projects():
    """保存项目数据到文件"""
    try:
        data = {
            "projects": {pid: p.model_dump() for pid, p in projects.items()},
            "project_flows": project_flows
        }
        with open(PROJECTS_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
    except Exception as e:
        print(f"Warning: Failed to save projects: {e}")


def load_projects():
    """从文件加载项目数据"""
    global projects, project_flows
    if not PROJECTS_FILE.exists():
        return
    
    try:
        with open(PROJECTS_FILE, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        # 加载项目
        projects.clear()
        for pid, p_data in data.get("projects", {}).items():
            projects[pid] = ProjectInfo(**p_data)
        
        # 加载项目-流程映射
        project_flows = data.get("project_flows", {})
    except Exception as e:
        print(f"Warning: Failed to load projects: {e}")


def save_flows():
    """保存Flow数据到文件"""
    try:
        data = {
            "flows": {fid: f.model_dump() for fid, f in flows.items()}
        }
        with open(FLOWS_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
    except Exception as e:
        print(f"Warning: Failed to save flows: {e}")


def load_flows():
    """从文件加载Flow数据"""
    global flows
    if not FLOWS_FILE.exists():
        return
    
    try:
        with open(FLOWS_FILE, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        flows.clear()
        for fid, f_data in data.get("flows", {}).items():
            flows[fid] = FlowDefinition(**f_data)
    except Exception as e:
        print(f"Warning: Failed to load flows: {e}")


def save_flow_versions():
    """保存Flow版本历史到文件"""
    try:
        data = {
            "flow_versions": {
                fid: [v.model_dump() for v in versions]
                for fid, versions in flow_versions.items()
            }
        }
        with open(FLOW_VERSIONS_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
    except Exception as e:
        print(f"Warning: Failed to save flow versions: {e}")


def load_flow_versions():
    """从文件加载Flow版本历史"""
    global flow_versions
    if not FLOW_VERSIONS_FILE.exists():
        return
    
    try:
        with open(FLOW_VERSIONS_FILE, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        flow_versions.clear()
        for fid, versions_data in data.get("flow_versions", {}).items():
            flow_versions[fid] = [FlowDefinition(**v_data) for v_data in versions_data]
    except Exception as e:
        print(f"Warning: Failed to load flow versions: {e}")


def save_counters():
    """保存计数器到文件"""
    try:
        data = {
            "project_counter": project_counter,
            "flow_counter": flow_counter
        }
        with open(COUNTERS_FILE, 'w', encoding='utf-8') as f:
            json.dump(data, f, indent=2)
    except Exception as e:
        print(f"Warning: Failed to save counters: {e}")


def load_counters():
    """从文件加载计数器"""
    global project_counter, flow_counter
    if not COUNTERS_FILE.exists():
        return
    
    try:
        with open(COUNTERS_FILE, 'r', encoding='utf-8') as f:
            data = json.load(f)
        
        project_counter = data.get("project_counter", 0)
        flow_counter = data.get("flow_counter", 0)
    except Exception as e:
        print(f"Warning: Failed to load counters: {e}")


def load_all_data():
    """加载所有持久化数据"""
    load_projects()
    load_flows()
    load_flow_versions()
    load_counters()
    print(f"✅ 已加载持久化数据: {len(projects)}个项目, {len(flows)}个Flow, {sum(len(v) for v in flow_versions.values())}个版本")


# ========== 初始化默认节点模板 ==========

def init_default_node_templates():
    """初始化默认节点模板（基于 SDK 中已实现的节点）"""
    global node_templates

    # FlightStateSourceNode
    node_templates["flight_state_source"] = NodeTemplate(
        template_id="flight_state_source",
        name="Flight State Source",
        category=NodeCategory.FLIGHT,
        description="从飞控读取飞行状态（位置、姿态、电池等）",
        input_ports=[],
        output_ports=[
            PortDefinition(name="state", type=PortType.FLIGHT_STATE, description="飞行状态")
        ],
        parameters_schema={
            "type": "object",
            "properties": {
                "connection_config": {
                    "type": "object",
                    "properties": {
                        "address": {"type": "string", "default": "127.0.0.1"},
                        "port": {"type": "integer", "default": 14540},
                    }
                }
            }
        },
        sdk_class_name="FlightStateSourceNode",
    )

    # FlightCommandSinkNode
    node_templates["flight_command_sink"] = NodeTemplate(
        template_id="flight_command_sink",
        name="Flight Command Sink",
        category=NodeCategory.FLIGHT,
        description="发送飞控命令（ARM、TAKEOFF、LAND、RTL 等）",
        input_ports=[
            PortDefinition(name="command", type=PortType.FLIGHT_COMMAND, description="飞控命令")
        ],
        output_ports=[],
        parameters_schema={
            "type": "object",
            "properties": {
                "connection_config": {
                    "type": "object",
                    "properties": {
                        "address": {"type": "string", "default": "127.0.0.1"},
                        "port": {"type": "integer", "default": 14540},
                    }
                }
            }
        },
        sdk_class_name="FlightCommandSinkNode",
    )

    # CameraSourceNode
    node_templates["camera_source"] = NodeTemplate(
        template_id="camera_source",
        name="Camera Source",
        category=NodeCategory.SENSORS,
        description="从相机获取视频流（USB/MIPI/RTSP/UDP）",
        input_ports=[],
        output_ports=[
            PortDefinition(name="frame", type=PortType.IMAGE, description="视频帧")
        ],
        parameters_schema={
            "type": "object",
            "properties": {
                "source_type": {
                    "type": "string",
                    "enum": ["USB_CAMERA", "MIPI_CAMERA", "RTSP", "UDP"],
                    "default": "USB_CAMERA"
                },
                "source_uri": {"type": "string", "default": "/dev/video0"},
            }
        },
        sdk_class_name="CameraSourceNode",
    )

    # DummyDetectionNode
    node_templates["dummy_detection"] = NodeTemplate(
        template_id="dummy_detection",
        name="Dummy Detection",
        category=NodeCategory.PERCEPTION,
        description="简单的目标检测节点（用于测试）",
        input_ports=[
            PortDefinition(name="frame", type=PortType.IMAGE, description="输入图像")
        ],
        output_ports=[
            PortDefinition(name="detections", type=PortType.DETECTION, description="检测结果")
        ],
        parameters_schema={
            "type": "object",
            "properties": {
                "backend_type": {
                    "type": "string",
                    "enum": ["ONNX", "RKNN", "TensorRT", "CPU"],
                    "default": "CPU"
                }
            }
        },
        sdk_class_name="DummyDetectionNode",
    )

    # TrackingTransformNode
    node_templates["tracking"] = NodeTemplate(
        template_id="tracking",
        name="Tracking",
        category=NodeCategory.PERCEPTION,
        description="目标跟踪节点",
        input_ports=[
            PortDefinition(name="detections", type=PortType.DETECTION, description="检测结果")
        ],
        output_ports=[
            PortDefinition(name="tracks", type=PortType.TRACKING, description="跟踪结果")
        ],
        parameters_schema={
            "type": "object",
            "properties": {
                "tracker_type": {
                    "type": "string",
                    "enum": ["SIMPLE", "SORT", "DeepSORT", "ByteTrack"],
                    "default": "SIMPLE"
                }
            }
        },
        sdk_class_name="TrackingTransformNode",
    )

    # SearchPathPlannerNode
    node_templates["search_path_planner"] = NodeTemplate(
        template_id="search_path_planner",
        name="Search Path Planner",
        category=NodeCategory.MISSION,
        description="搜索路径规划节点，根据搜索区域和参数生成搜索路径（航点列表）",
        input_ports=[],
        output_ports=[
            PortDefinition(name="waypoints", type=PortType.ANY, description="航点列表")
        ],
        parameters_schema={
            "type": "object",
            "properties": {
                "search_area": {
                    "type": "object",
                    "properties": {
                        "polygon": {
                            "type": "array",
                            "items": {
                                "type": "object",
                                "properties": {
                                    "lat": {"type": "number"},
                                    "lon": {"type": "number"},
                                    "alt": {"type": "number"}
                                }
                            },
                            "description": "搜索区域多边形顶点（至少3个点）"
                        },
                        "min_altitude": {"type": "number", "default": 30.0},
                        "max_altitude": {"type": "number", "default": 100.0}
                    }
                },
                "search_params": {
                    "type": "object",
                    "properties": {
                        "pattern": {
                            "type": "string",
                            "enum": ["LAWN_MOWER", "SPIRAL", "ZIGZAG", "SECTOR", "WAYPOINT_LIST"],
                            "default": "LAWN_MOWER",
                            "description": "搜索模式：LAWN_MOWER(网格), SPIRAL(螺旋), ZIGZAG(Z字形), SECTOR(扇形), WAYPOINT_LIST(航点列表)"
                        },
                        "altitude": {"type": "number", "default": 50.0, "description": "飞行高度（米）"},
                        "speed": {"type": "number", "default": 5.0, "description": "飞行速度（m/s）"},
                        "spacing": {"type": "number", "default": 20.0, "description": "搜索线间距（米）"},
                        "loiter_time": {"type": "number", "default": 2.0, "description": "每个航点悬停时间（秒）"},
                        "enable_detection": {"type": "boolean", "default": True},
                        "detection_classes": {
                            "type": "array",
                            "items": {"type": "string"},
                            "default": ["person", "vehicle"],
                            "description": "关注的检测类别"
                        }
                    }
                }
            }
        },
        sdk_class_name="SearchPathPlannerNode",
    )

    # EventReporterNode
    node_templates["event_reporter"] = NodeTemplate(
        template_id="event_reporter",
        name="Event Reporter",
        category=NodeCategory.MISSION,
        description="事件上报节点，上报搜索事件、搜索进度和检测结果",
        input_ports=[
            PortDefinition(name="events", type=PortType.ANY, description="事件数据")
        ],
        output_ports=[],
        parameters_schema={
            "type": "object",
            "properties": {
                "uav_id": {"type": "string", "default": "uav_001", "description": "UAV ID"},
                "mission_id": {"type": "string", "default": "mission_unknown", "description": "任务 ID"}
            }
        },
        sdk_class_name="EventReporterNode",
    )


# 加载持久化数据（在初始化模板之前）
load_all_data()

# 初始化默认模板
init_default_node_templates()


# ========== API 接口 ==========

@app.get("/health")
async def health_check() -> dict:
    return {"status": "ok"}


# ========== 节点模板接口 ==========

@app.get("/templates")
async def list_templates(category: Optional[str] = None) -> dict:
    """列出所有节点模板"""
    templates = list(node_templates.values())
    if category:
        templates = [t for t in templates if t.category.value == category]
    return {"templates": [t.model_dump() for t in templates]}


@app.get("/templates/{template_id}")
async def get_template(template_id: str) -> Optional[NodeTemplate]:
    """获取节点模板详情"""
    return node_templates.get(template_id)


# ========== 工程管理接口 ==========

@app.get("/projects")
async def list_projects() -> dict:
    """列出所有工程"""
    return {
        "projects": [
            {
                "project_id": p.project_id,
                "name": p.name,
                "description": p.description,
                "created_at": p.created_at,
                "updated_at": p.updated_at,
            }
            for p in projects.values()
        ]
    }


@app.post("/projects")
async def create_project(request: ProjectCreateRequest) -> dict:
    """创建工程"""
    global project_counter
    project_counter += 1
    project_id = f"project_{project_counter:04d}"
    
    now = datetime.utcnow().isoformat() + "Z"
    
    # 创建完整的 ProjectInfo
    project = ProjectInfo(
        project_id=project_id,
        name=request.name,
        description=request.description,
        created_at=now,
        updated_at=now,
    )
    
    projects[project_id] = project
    project_flows[project_id] = []
    
    # 保存到文件
    save_projects()
    save_counters()
    
    return {"project_id": project_id, "project": project.model_dump()}


@app.get("/projects/{project_id}")
async def get_project(project_id: str) -> Optional[ProjectInfo]:
    """获取工程详情"""
    return projects.get(project_id)


# ========== 流程管理接口 ==========

@app.get("/projects/{project_id}/flows")
async def list_flows(project_id: str) -> dict:
    """获取工程内所有流程"""
    if project_id not in project_flows:
        raise HTTPException(status_code=404, detail="Project not found")
    
    flow_ids = project_flows[project_id]
    flow_list = []
    for flow_id in flow_ids:
        if flow_id in flows:
            flow = flows[flow_id]
            flow_list.append({
                "flow_id": flow.flow_id,
                "name": flow.name,
                "description": flow.description,
                "node_count": len(flow.nodes),
                "edge_count": len(flow.edges),
                "created_at": flow.created_at,
                "updated_at": flow.updated_at,
            })
    
    return {"flows": flow_list}


@app.get("/projects/{project_id}/flows/{flow_id}/export")
async def export_flow(project_id: str, flow_id: str) -> dict:
    """
    导出Flow定义为JSON格式（用于模式3：零代码动态执行）
    返回Flow定义JSON，包含nodes和edges信息，可直接用于FlowExecutor
    """
    if project_id not in project_flows:
        raise HTTPException(status_code=404, detail="Project not found")
    if flow_id not in flows:
        raise HTTPException(status_code=404, detail="Flow not found")
    if flow_id not in project_flows[project_id]:
        raise HTTPException(status_code=400, detail="Flow does not belong to this project")
    
    flow = flows[flow_id]
    
    # 转换为Flow定义JSON格式（用于FlowExecutor）
    flow_definition = {
        "flow_id": flow.flow_id,
        "name": flow.name,
        "description": flow.description,
        "version": flow.version,  # 使用实际版本号
        "nodes": [
            {
                "node_id": node.node_id,
                "template_id": node.template_id,
                "parameters": node.parameters if hasattr(node, 'parameters') else {}
            }
            for node in flow.nodes
        ],
        "edges": [
            {
                "edge_id": edge.edge_id,
                "from_node_id": edge.from_node_id,
                "from_port": edge.from_port,
                "to_node_id": edge.to_node_id,
                "to_port": edge.to_port
            }
            for edge in flow.edges
        ]
    }
    
    return flow_definition


@app.get("/projects/{project_id}/flows/{flow_id}")
async def get_flow(project_id: str, flow_id: str) -> Optional[FlowDefinition]:
    """获取流程定义"""
    if project_id not in project_flows:
        raise HTTPException(status_code=404, detail="Project not found")
    if flow_id not in flows:
        raise HTTPException(status_code=404, detail="Flow not found")
    if flow_id not in project_flows[project_id]:
        raise HTTPException(status_code=400, detail="Flow does not belong to this project")
    
    return flows[flow_id]


@app.put("/projects/{project_id}/flows/{flow_id}")
async def save_flow(project_id: str, flow_id: str, request: FlowCreateRequest) -> dict:
    """保存/更新流程定义"""
    if project_id not in project_flows:
        raise HTTPException(status_code=404, detail="Project not found")
    
    now = datetime.utcnow().isoformat() + "Z"
    
    # 如果是新流程
    if flow_id not in flows:
        # 创建新流程（版本1.0）
        flow = FlowDefinition(
            flow_id=flow_id,
            name=request.name,
            description=request.description or "",
            version="1.0",
            nodes=request.nodes or [],
            edges=request.edges or [],
            created_at=now,
            updated_at=now,
        )
        project_flows[project_id].append(flow_id)
        flow_versions[flow_id] = [flow]  # 初始化版本历史
    else:
        # 更新现有流程（创建新版本）
        existing_flow = flows[flow_id]
        
        # 计算新版本号（简单递增：1.0 -> 1.1 -> 1.2 ...）
        current_version = existing_flow.version
        try:
            version_parts = current_version.split(".")
            if len(version_parts) == 2:
                major, minor = int(version_parts[0]), int(version_parts[1])
                new_version = f"{major}.{minor + 1}"
            else:
                new_version = "1.1"  # 默认递增
        except:
            new_version = "1.1"  # 如果版本号格式错误，使用默认值
        
        # 创建新版本
        flow = FlowDefinition(
            flow_id=flow_id,
            name=request.name,
            description=request.description or "",
            version=new_version,
            nodes=request.nodes or [],
            edges=request.edges or [],
            created_at=existing_flow.created_at,  # 保留原始创建时间
            updated_at=now,
        )
        
        # 保存到版本历史
        if flow_id not in flow_versions:
            flow_versions[flow_id] = []
        flow_versions[flow_id].append(flow)
    
    # 更新当前版本
    flows[flow_id] = flow
    
    # 保存到文件
    save_flows()
    save_flow_versions()
    save_projects()  # 更新project_flows映射
    
    return {"status": "saved", "flow": flow.model_dump(), "version": flow.version}


@app.post("/projects/{project_id}/flows")
async def create_flow(project_id: str, request: FlowCreateRequest) -> dict:
    """创建新流程"""
    global flow_counter
    flow_counter += 1
    flow_id = f"flow_{flow_counter:04d}"
    
    if project_id not in project_flows:
        raise HTTPException(status_code=404, detail="Project not found")
    
    now = datetime.utcnow().isoformat() + "Z"
    
    # 创建完整的 FlowDefinition（版本1.0）
    flow = FlowDefinition(
        flow_id=flow_id,
        name=request.name,
        description=request.description or "",
        version="1.0",
        nodes=request.nodes or [],
        edges=request.edges or [],
        created_at=now,
        updated_at=now,
    )
    
    flows[flow_id] = flow
    flow_versions[flow_id] = [flow]  # 初始化版本历史
    project_flows[project_id].append(flow_id)
    
    # 保存到文件
    save_flows()
    save_flow_versions()
    save_projects()
    save_counters()
    
    return {"flow_id": flow_id, "flow": flow.model_dump()}


# ========== 代码生成接口 ==========

class CodeGenerationRequest(BaseModel):
    """代码生成请求"""
    project_name: str
    output_directory: str = "./generated"
    flow_name: str = ""


@app.post("/projects/{project_id}/flows/{flow_id}/generate")
async def generate_code(project_id: str, flow_id: str, request: CodeGenerationRequest) -> dict:
    """生成 SDK 工程骨架代码"""
    if flow_id not in flows:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    flow = flows[flow_id]
    
    # 使用请求中的项目名称，如果没有则使用流程名称
    project_name = request.project_name or flow.name.replace(" ", "_").lower()
    output_dir = request.output_directory or "./generated"
    
    # 生成 main.cpp 代码
    main_cpp = generate_main_cpp(flow, project_name)
    
    # 生成 CMakeLists.txt
    cmake_txt = generate_cmake_txt(flow, project_name)
    
    return {
        "status": "generated",
        "project_name": project_name,
        "output_directory": output_dir,
        "files": {
            "main.cpp": main_cpp,
            "CMakeLists.txt": cmake_txt,
        }
    }


def generate_main_cpp(flow: FlowDefinition, project_name: str = "generated_pipeline") -> str:
    """生成 main.cpp 代码骨架"""
    lines = []
    lines.append("// Generated by FalconMindBuilder")
    lines.append("// Flow: " + flow.name)
    lines.append("")
    lines.append("#include <falconmind/sdk/core/Pipeline.h>")
    lines.append("#include <falconmind/sdk/core/Node.h>")
    lines.append("#include <falconmind/sdk/flight/FlightConnectionService.h>")
    lines.append("#include <falconmind/sdk/sensors/VideoSourceConfig.h>")
    lines.append("")
    lines.append("// 根据模板 ID 包含对应的节点头文件")
    
    # 收集所有使用的模板
    used_templates = set(node.template_id for node in flow.nodes)
    template_includes = {
        "flight_state_source": "#include \"falconmind/sdk/flight/FlightNodes.h\"",
        "flight_command_sink": "#include \"falconmind/sdk/flight/FlightNodes.h\"",
        "camera_source": "#include \"falconmind/sdk/sensors/CameraSourceNode.h\"",
        "dummy_detection": "#include \"falconmind/sdk/perception/DummyDetectionNode.h\"",
        "tracking": "#include \"falconmind/sdk/perception/TrackingTransformNode.h\"",
        "search_path_planner": "#include \"falconmind/sdk/mission/SearchPathPlannerNode.h\"",
        "event_reporter": "#include \"falconmind/sdk/mission/EventReporterNode.h\"",
    }
    
    included = set()
    for template_id in used_templates:
        if template_id in template_includes and template_includes[template_id] not in included:
            lines.append(template_includes[template_id])
            included.add(template_includes[template_id])
    
    lines.append("")
    lines.append("#include <iostream>")
    lines.append("#include <thread>")
    lines.append("#include <chrono>")
    lines.append("")
    lines.append("using namespace falconmind::sdk;")
    lines.append("")
    lines.append("int main() {")
    lines.append("    // 1. 准备 Pipeline 配置")
    lines.append(f"    core::PipelineConfig cfg;")
    lines.append(f"    cfg.pipelineId = \"{project_name}\";")
    lines.append(f"    cfg.name = \"{flow.name}\";")
    lines.append(f"    cfg.description = \"{flow.description or 'Generated by FalconMindBuilder'}\";")
    lines.append("")
    lines.append(f"    core::Pipeline pipeline(cfg);")
    lines.append("")
    
    # 检查是否需要 FlightConnectionService
    needs_flight = any(node.template_id in ["flight_state_source", "flight_command_sink"] for node in flow.nodes)
    if needs_flight:
        lines.append("    // 2. 创建 FlightConnectionService（用于飞控通信）")
        lines.append("    flight::FlightConnectionConfig flightCfg;")
        lines.append("    flightCfg.address = \"127.0.0.1\";")
        lines.append("    flightCfg.port = 14540;")
        lines.append("    auto flightService = std::make_shared<flight::FlightConnectionService>(flightCfg);")
        lines.append("")
    
    # 创建节点
    lines.append("    // 2. 创建节点")
    node_vars = {}
    for node in flow.nodes:
        template = node_templates.get(node.template_id)
        if not template:
            continue
        
        var_name = f"node_{node.node_id.replace('-', '_')}"
        node_vars[node.node_id] = var_name
        
        # 根据模板类型创建节点
        if template.template_id == "flight_state_source":
            lines.append(f"    auto {var_name} = std::make_shared<flight::FlightStateSourceNode>(*flightService);")
        elif template.template_id == "flight_command_sink":
            lines.append(f"    auto {var_name} = std::make_shared<flight::FlightCommandSinkNode>(*flightService);")
        elif template.template_id == "camera_source":
            lines.append("    sensors::VideoSourceConfig cameraCfg;")
            lines.append("    cameraCfg.sensorId = \"cam0\";")
            lines.append("    cameraCfg.device = \"/dev/video0\";")
            lines.append("    cameraCfg.width = 640;")
            lines.append("    cameraCfg.height = 480;")
            lines.append("    cameraCfg.fps = 30.0;")
            lines.append(f"    auto {var_name} = std::make_shared<sensors::CameraSourceNode>(cameraCfg);")
        elif template.template_id == "dummy_detection":
            lines.append(f"    auto {var_name} = std::make_shared<perception::DummyDetectionNode>();")
        elif template.template_id == "tracking":
            lines.append(f"    auto {var_name} = std::make_shared<perception::TrackingTransformNode>();")
        elif template.template_id == "search_path_planner":
            lines.append(f"    auto {var_name} = std::make_shared<mission::SearchPathPlannerNode>();")
            # 配置搜索区域和参数（从节点参数中读取）
            params = node.parameters or {}
            search_area = params.get("search_area", {})
            search_params = params.get("search_params", {})
            
            if search_area and search_area.get("polygon"):
                lines.append(f"    // 配置搜索区域")
                lines.append(f"    mission::SearchArea searchArea_{var_name};")
                lines.append(f"    searchArea_{var_name}.polygon = {{")
                for point in search_area["polygon"]:
                    lat = point.get("lat", 0.0)
                    lon = point.get("lon", 0.0)
                    alt = point.get("alt", 0.0)
                    lines.append(f"        mission::GeoPoint{{{lat}, {lon}, {alt}}},")
                lines.append(f"    }};")
                if "min_altitude" in search_area:
                    lines.append(f"    searchArea_{var_name}.minAltitude = {search_area['min_altitude']};")
                if "max_altitude" in search_area:
                    lines.append(f"    searchArea_{var_name}.maxAltitude = {search_area['max_altitude']};")
                lines.append(f"    {var_name}->setSearchArea(searchArea_{var_name});")
            
            if search_params:
                lines.append(f"    // 配置搜索参数")
                lines.append(f"    mission::SearchParams searchParams_{var_name};")
                if "pattern" in search_params:
                    pattern = search_params["pattern"].upper()
                    lines.append(f"    searchParams_{var_name}.pattern = mission::SearchPattern::{pattern};")
                if "altitude" in search_params:
                    lines.append(f"    searchParams_{var_name}.altitude = {search_params['altitude']};")
                if "speed" in search_params:
                    lines.append(f"    searchParams_{var_name}.speed = {search_params['speed']};")
                if "spacing" in search_params:
                    lines.append(f"    searchParams_{var_name}.spacing = {search_params['spacing']};")
                if "loiter_time" in search_params:
                    lines.append(f"    searchParams_{var_name}.loiterTime = {search_params['loiter_time']};")
                if "enable_detection" in search_params:
                    enable = "true" if search_params["enable_detection"] else "false"
                    lines.append(f"    searchParams_{var_name}.enableDetection = {enable};")
                if "detection_classes" in search_params:
                    classes = search_params["detection_classes"]
                    lines.append(f"    searchParams_{var_name}.detectionClasses = {{")
                    for cls in classes:
                        lines.append(f"        \"{cls}\",")
                    lines.append(f"    }};")
                lines.append(f"    {var_name}->setSearchParams(searchParams_{var_name});")
        elif template.template_id == "event_reporter":
            lines.append(f"    auto {var_name} = std::make_shared<mission::EventReporterNode>();")
            # 配置 UAV ID 和 Mission ID（从节点参数中读取）
            params = node.parameters or {}
            uav_id = params.get("uav_id", "uav_001")
            mission_id = params.get("mission_id", "mission_unknown")
            lines.append(f"    std::unordered_map<std::string, std::string> {var_name}Params{{")
            lines.append(f"        {{\"uav_id\", \"{uav_id}\"}}, {{\"mission_id\", \"{mission_id}\"}}")
            lines.append(f"    }};")
            lines.append(f"    {var_name}->configure({var_name}Params);")
        else:
            lines.append(f"    // TODO: 创建 {template.sdk_class_name} 节点")
            lines.append(f"    // auto {var_name} = std::make_shared<...>();")
            continue
        
        lines.append(f"    if (!pipeline.addNode({var_name})) {{")
        lines.append(f"        std::cerr << \"Failed to add node {var_name}\" << std::endl;")
        lines.append(f"        return 1;")
        lines.append(f"    }}")
        lines.append("")
    
    # 连接节点
    if flow.edges:
        lines.append("    // 3. 连接节点")
        for edge in flow.edges:
            from_var = node_vars.get(edge.from_node_id)
            to_var = node_vars.get(edge.to_node_id)
            if not from_var or not to_var:
                continue
            lines.append(f"    if (!pipeline.link({from_var}->id(), \"{edge.from_port}\", {to_var}->id(), \"{edge.to_port}\")) {{")
            lines.append(f"        std::cerr << \"Failed to link {from_var} to {to_var}\" << std::endl;")
            lines.append(f"        return 1;")
            lines.append(f"    }}")
        lines.append("")
    
    # 配置和启动节点
    lines.append("    // 4. 配置并启动各个节点")
    for node in flow.nodes:
        var_name = node_vars.get(node.node_id)
        if not var_name:
            continue
        template = node_templates.get(node.template_id)
        if template and template.template_id == "camera_source":
            lines.append(f"    std::unordered_map<std::string, std::string> {var_name}Params{{")
            lines.append(f"        {{\"device\", \"/dev/video0\"}}")
            lines.append(f"    }};")
            lines.append(f"    {var_name}->configure({var_name}Params);")
        elif template and template.template_id == "dummy_detection":
            lines.append(f"    {var_name}->configure({{\"modelName\", \"dummy-yolo\"}});")
        lines.append(f"    {var_name}->start();")
    lines.append("")
    
    lines.append("    // 5. 启动 Pipeline")
    lines.append("    pipeline.setState(core::PipelineState::Ready);")
    lines.append("    pipeline.setState(core::PipelineState::Playing);")
    lines.append("")
    lines.append("    // 6. 运行主循环")
    lines.append("    std::cout << \"Pipeline started. Press Ctrl+C to stop.\" << std::endl;")
    lines.append("    while (true) {")
    for node in flow.nodes:
        var_name = node_vars.get(node.node_id)
        if var_name:
            lines.append(f"        {var_name}->process();")
    lines.append("        std::this_thread::sleep_for(std::chrono::milliseconds(100));")
    lines.append("    }")
    lines.append("")
    lines.append("    return 0;")
    lines.append("}")
    
    return "\n".join(lines)


def generate_cmake_txt(flow: FlowDefinition, project_name: str = "generated_pipeline") -> str:
    """生成 CMakeLists.txt"""
    lines = []
    lines.append("cmake_minimum_required(VERSION 3.16)")
    lines.append(f"project({project_name})")
    lines.append("")
    lines.append("set(CMAKE_CXX_STANDARD 17)")
    lines.append("set(CMAKE_CXX_STANDARD_REQUIRED ON)")
    lines.append("")
    lines.append("# 查找 FalconMindSDK")
    lines.append("# 方案1: 如果SDK已安装，使用find_package")
    lines.append("find_package(FalconMindSDK QUIET)")
    lines.append("")
    lines.append("# 方案2: 如果SDK未安装，直接链接SDK库文件（用于测试）")
    lines.append("if(NOT FalconMindSDK_FOUND)")
    lines.append("    # 假设SDK在相对路径 ../FalconMindSDK")
    lines.append("    set(SDK_DIR \"${CMAKE_CURRENT_SOURCE_DIR}/../../FalconMindSDK\")")
    lines.append("    set(SDK_BUILD_DIR \"${SDK_DIR}/build\")")
    lines.append("    ")
    lines.append("    # 包含SDK头文件")
    lines.append("    include_directories(${SDK_DIR}/include)")
    lines.append("    ")
    lines.append("    # 链接SDK静态库")
    lines.append("    set(SDK_LIB \"${SDK_BUILD_DIR}/libfalconmind_sdk.a\")")
    lines.append("    ")
    lines.append("    if(EXISTS ${SDK_LIB})")
    lines.append("        message(STATUS \"Using SDK library from: ${SDK_LIB}\")")
    lines.append("        set(SDK_LIB_FOUND TRUE)")
    lines.append("    else()")
    lines.append("        # 尝试绝对路径（用于测试环境）")
    lines.append("        set(SDK_DIR_ABS \"/home/shook/work/FalconMind/FalconMindSDK\")")
    lines.append("        set(SDK_BUILD_DIR_ABS \"${SDK_DIR_ABS}/build\")")
    lines.append("        set(SDK_LIB_ABS \"${SDK_BUILD_DIR_ABS}/libfalconmind_sdk.a\")")
    lines.append("        if(EXISTS ${SDK_LIB_ABS})")
    lines.append("            message(STATUS \"Using SDK library from absolute path: ${SDK_LIB_ABS}\")")
    lines.append("            set(SDK_DIR ${SDK_DIR_ABS})")
    lines.append("            set(SDK_BUILD_DIR ${SDK_BUILD_DIR_ABS})")
    lines.append("            set(SDK_LIB ${SDK_LIB_ABS})")
    lines.append("            set(SDK_LIB_FOUND TRUE)")
    lines.append("        else()")
    lines.append("            message(WARNING \"SDK library not found\")")
    lines.append("            message(WARNING \"Please build SDK first: cd ${SDK_DIR} && mkdir -p build && cd build && cmake .. && make\")")
    lines.append("            set(SDK_LIB_FOUND FALSE)")
    lines.append("        endif()")
    lines.append("    endif()")
    lines.append("else()")
    lines.append("    set(SDK_LIB_FOUND TRUE)")
    lines.append("endif()")
    lines.append("")
    lines.append("# 可执行文件")
    lines.append("add_executable(${PROJECT_NAME} main.cpp)")
    lines.append("")
    lines.append("# 链接SDK库")
    lines.append("if(FalconMindSDK_FOUND)")
    lines.append("    target_link_libraries(${PROJECT_NAME} PRIVATE falconmind_sdk)")
    lines.append("elseif(SDK_LIB_FOUND)")
    lines.append("    target_link_libraries(${PROJECT_NAME} PRIVATE ${SDK_LIB})")
    lines.append("    # 包含SDK头文件目录")
    lines.append("    target_include_directories(${PROJECT_NAME} PRIVATE ${SDK_DIR}/include)")
    lines.append("    # 链接SDK依赖的库")
    lines.append("    target_link_libraries(${PROJECT_NAME} PRIVATE pthread)")
    lines.append("    # 链接nlohmann/json（如果SDK使用了）")
    lines.append("    find_package(nlohmann_json QUIET)")
    lines.append("    if(nlohmann_json_FOUND)")
    lines.append("        target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)")
    lines.append("    endif()")
    lines.append("else()")
    lines.append("    message(FATAL_ERROR \"Cannot find FalconMindSDK library\")")
    lines.append("endif()")
    lines.append("")
    
    return "\n".join(lines)


# ========== Flow版本管理接口 ==========

@app.get("/projects/{project_id}/flows/{flow_id}/versions")
async def get_flow_versions(project_id: str, flow_id: str) -> dict:
    """获取Flow的所有版本历史"""
    if project_id not in project_flows:
        raise HTTPException(status_code=404, detail="Project not found")
    
    if flow_id not in flows:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    if flow_id not in project_flows[project_id]:
        raise HTTPException(status_code=404, detail="Flow does not belong to this project")
    
    # 获取版本历史
    versions = flow_versions.get(flow_id, [])
    
    # 返回版本列表（按版本号排序，最新的在前）
    version_list = []
    for v in sorted(versions, key=lambda x: x.version, reverse=True):
        version_list.append({
            "version": v.version,
            "name": v.name,
            "description": v.description,
            "updated_at": v.updated_at,
            "version_comment": v.version_comment,
            "node_count": len(v.nodes),
            "edge_count": len(v.edges),
        })
    
    return {
        "flow_id": flow_id,
        "current_version": flows[flow_id].version,
        "versions": version_list,
        "total_versions": len(versions)
    }


@app.get("/projects/{project_id}/flows/{flow_id}/versions/{version}")
async def get_flow_version(project_id: str, flow_id: str, version: str) -> dict:
    """获取指定版本的Flow定义"""
    if project_id not in project_flows:
        raise HTTPException(status_code=404, detail="Project not found")
    
    if flow_id not in flows:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    if flow_id not in project_flows[project_id]:
        raise HTTPException(status_code=404, detail="Flow does not belong to this project")
    
    # 查找指定版本
    versions = flow_versions.get(flow_id, [])
    target_version = None
    for v in versions:
        if v.version == version:
            target_version = v
            break
    
    if not target_version:
        raise HTTPException(status_code=404, detail=f"Version {version} not found")
    
    return {
        "flow_id": flow_id,
        "version": target_version.version,
        "flow": target_version.model_dump()
    }


@app.post("/projects/{project_id}/flows/{flow_id}/versions/{version}/rollback")
async def rollback_flow_version(project_id: str, flow_id: str, version: str) -> dict:
    """回滚Flow到指定版本（创建新版本）"""
    if project_id not in project_flows:
        raise HTTPException(status_code=404, detail="Project not found")
    
    if flow_id not in flows:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    if flow_id not in project_flows[project_id]:
        raise HTTPException(status_code=404, detail="Flow does not belong to this project")
    
    # 查找指定版本
    versions = flow_versions.get(flow_id, [])
    target_version = None
    for v in versions:
        if v.version == version:
            target_version = v
            break
    
    if not target_version:
        raise HTTPException(status_code=404, detail=f"Version {version} not found")
    
    # 计算新版本号
    current_version = flows[flow_id].version
    try:
        version_parts = current_version.split(".")
        if len(version_parts) == 2:
            major, minor = int(version_parts[0]), int(version_parts[1])
            new_version = f"{major}.{minor + 1}"
        else:
            new_version = "1.1"
    except:
        new_version = "1.1"
    
    now = datetime.utcnow().isoformat() + "Z"
    
    # 基于目标版本创建新版本
    rolled_back_flow = FlowDefinition(
        flow_id=flow_id,
        name=target_version.name,
        description=target_version.description,
        version=new_version,
        nodes=target_version.nodes.copy(),
        edges=target_version.edges.copy(),
        created_at=target_version.created_at,
        updated_at=now,
        version_comment=f"Rolled back from version {current_version} to {version}"
    )
    
    # 保存到版本历史
    flow_versions[flow_id].append(rolled_back_flow)
    
    # 更新当前版本
    flows[flow_id] = rolled_back_flow
    
    # 保存到文件
    save_flows()
    save_flow_versions()
    
    return {
        "status": "rolled_back",
        "flow": rolled_back_flow.model_dump(),
        "from_version": current_version,
        "to_version": version,
        "new_version": new_version
    }


@app.get("/projects/{project_id}/flows/{flow_id}/versions/compare")
async def compare_flow_versions(
    project_id: str, 
    flow_id: str, 
    version1: str, 
    version2: str
) -> dict:
    """比较两个Flow版本的差异"""
    if project_id not in project_flows:
        raise HTTPException(status_code=404, detail="Project not found")
    
    if flow_id not in flows:
        raise HTTPException(status_code=404, detail="Flow not found")
    
    if flow_id not in project_flows[project_id]:
        raise HTTPException(status_code=404, detail="Flow does not belong to this project")
    
    # 查找两个版本
    versions = flow_versions.get(flow_id, [])
    v1 = None
    v2 = None
    
    for v in versions:
        if v.version == version1:
            v1 = v
        if v.version == version2:
            v2 = v
    
    if not v1:
        raise HTTPException(status_code=404, detail=f"Version {version1} not found")
    if not v2:
        raise HTTPException(status_code=404, detail=f"Version {version2} not found")
    
    # 比较差异
    differences = {
        "name_changed": v1.name != v2.name,
        "description_changed": v1.description != v2.description,
        "node_count_changed": len(v1.nodes) != len(v2.nodes),
        "edge_count_changed": len(v1.edges) != len(v2.edges),
        "nodes_added": [],
        "nodes_removed": [],
        "nodes_modified": [],
        "edges_added": [],
        "edges_removed": [],
        "edges_modified": [],
    }
    
    # 比较节点
    v1_node_ids = {n.node_id for n in v1.nodes}
    v2_node_ids = {n.node_id for n in v2.nodes}
    
    differences["nodes_added"] = list(v2_node_ids - v1_node_ids)
    differences["nodes_removed"] = list(v1_node_ids - v2_node_ids)
    
    # 检查修改的节点
    common_nodes = v1_node_ids & v2_node_ids
    for node_id in common_nodes:
        v1_node = next(n for n in v1.nodes if n.node_id == node_id)
        v2_node = next(n for n in v2.nodes if n.node_id == node_id)
        
        if v1_node.template_id != v2_node.template_id or v1_node.parameters != v2_node.parameters:
            differences["nodes_modified"].append(node_id)
    
    # 比较边
    v1_edge_ids = {e.edge_id for e in v1.edges}
    v2_edge_ids = {e.edge_id for e in v2.edges}
    
    differences["edges_added"] = list(v2_edge_ids - v1_edge_ids)
    differences["edges_removed"] = list(v1_edge_ids - v2_edge_ids)
    
    # 检查修改的边
    common_edges = v1_edge_ids & v2_edge_ids
    for edge_id in common_edges:
        v1_edge = next(e for e in v1.edges if e.edge_id == edge_id)
        v2_edge = next(e for e in v2.edges if e.edge_id == edge_id)
        
        if (v1_edge.from_node_id != v2_edge.from_node_id or
            v1_edge.from_port != v2_edge.from_port or
            v1_edge.to_node_id != v2_edge.to_node_id or
            v1_edge.to_port != v2_edge.to_port):
            differences["edges_modified"].append(edge_id)
    
    return {
        "flow_id": flow_id,
        "version1": version1,
        "version2": version2,
        "differences": differences,
        "summary": {
            "has_changes": any([
                differences["name_changed"],
                differences["description_changed"],
                differences["node_count_changed"],
                differences["edge_count_changed"],
                len(differences["nodes_added"]) > 0,
                len(differences["nodes_removed"]) > 0,
                len(differences["nodes_modified"]) > 0,
                len(differences["edges_added"]) > 0,
                len(differences["edges_removed"]) > 0,
                len(differences["edges_modified"]) > 0,
            ])
        }
    }


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("main:app", host="0.0.0.0", port=9001, reload=True)
