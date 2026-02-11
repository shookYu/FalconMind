# FalconMindBuilder

> **最后更新**: 2024-01-30

## 📚 相关文档

- **Doc/05_FalconMindBuilder_Design.md** - Builder 详细设计文档
- **Doc/03_Implementation_Plan.md** - 实施计划
- **Doc/01_System_Architecture_Overview.md** - 系统整体架构文档

## FalconMindBuilder

FalconMindBuilder 是面向行业工程师的零代码/低代码业务流程构建工具。用户通过拖拽和连接节点，即可快速搭建无人机业务流程，并一键生成基于 FalconMindSDK 的可运行工程。

## 功能特性

### 最小可用版（M4.2）

- ✅ **节点库管理**：从 SDK 导入节点定义，显示可用节点模板
- ✅ **可视化编辑器**：拖拽节点到画布，连接节点创建流程
- ✅ **流程存储**：保存和加载流程定义
- ✅ **代码生成**：将流程转换为 SDK 工程骨架（main.cpp + CMakeLists.txt）

## 快速开始

### 1. 启动后端

```bash
cd FalconMindBuilder/backend
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
uvicorn main:app --host 0.0.0.0 --port 9001 --reload
```

后端服务将在 `http://127.0.0.1:9001` 启动。

### 2. 启动前端

```bash
cd FalconMindBuilder/frontend
python3 -m http.server 8001
```

然后在浏览器中打开：`http://127.0.0.1:8001/index.html`

## 使用指南

### 创建流程

1. **从侧边栏拖拽节点**：左侧面板显示所有可用的节点模板，直接拖拽到画布
2. **连接节点**：点击节点的输出端口（蓝色圆点），然后点击目标节点的输入端口（红色圆点）
3. **配置节点**：选中节点后，右侧属性面板可以配置节点参数
4. **保存流程**：点击顶部 "Save Flow" 按钮保存
5. **生成代码**：点击 "Generate Code" 按钮，会打开新窗口显示生成的 C++ 代码

### 当前支持的节点类型

- **Flight State Source**：从飞控读取飞行状态
- **Flight Command Sink**：发送飞控命令
- **Camera Source**：从相机获取视频流
- **Dummy Detection**：简单的目标检测节点
- **Tracking**：目标跟踪节点

## API 接口

### 节点模板

- `GET /templates` - 列出所有节点模板
- `GET /templates/{template_id}` - 获取节点模板详情

### 工程管理

- `GET /projects` - 列出所有工程
- `POST /projects` - 创建工程
- `GET /projects/{project_id}` - 获取工程详情

### 流程管理

- `GET /projects/{project_id}/flows` - 获取工程内所有流程
- `GET /projects/{project_id}/flows/{flow_id}` - 获取流程定义
- `POST /projects/{project_id}/flows` - 创建新流程
- `PUT /projects/{project_id}/flows/{flow_id}` - 保存/更新流程

### 代码生成

- `POST /projects/{project_id}/flows/{flow_id}/generate` - 生成 SDK 工程骨架代码

## 文件结构

```
FalconMindBuilder/
├── backend/
│   ├── main.py              # FastAPI 后端服务
│   └── requirements.txt      # Python 依赖
├── frontend/
│   ├── index.html           # 主 HTML 文件
│   ├── styles.css           # 样式文件
│   └── app.js               # Vue3 应用逻辑
└── README.md                # 本文档
```

## 代码生成示例

生成的 `main.cpp` 示例：

```cpp
#include <falconmind/sdk/core/Pipeline.h>
#include <falconmind/sdk/flight/FlightNodes.h>
#include <falconmind/sdk/sensors/CameraSourceNode.h>

using namespace falconmind::sdk;

int main() {
    core::Pipeline pipeline;
    
    // 创建节点
    auto node_camera = std::make_shared<sensors::CameraSourceNode>(...);
    pipeline.addNode(node_camera);
    
    // 连接节点
    pipeline.link(node_camera, "frame", node_detection, "frame");
    
    // 启动 Pipeline
    pipeline.setState(core::PipelineState::RUNNING);
    
    return 0;
}
```

## 后续扩展

- 支持更多节点类型
- 节点参数配置界面
- 流程验证和仿真
- 模板管理和复用
- 导出 Docker 配置
- SDK 节点定义自动扫描

## 相关文档

- `Doc/FalconMindBuilder_Design.md` - 详细设计文档
- `Doc/Implementation_Plan.md` - 实施计划
