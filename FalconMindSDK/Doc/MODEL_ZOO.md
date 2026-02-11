# 检测模型获取与配置说明

> 与 `demo/detectors_demo.yaml` 及 `README_DEPENDENCIES.md` 配合使用。  
> **更新日期**：2025-02-06  
> **主平台**：RK1126B、RK3576、RK3588，推理以 **RKNN** 与 **.rknn** 模型为主。

## 1. 配置文件中的路径

`detectors_demo.yaml` 使用占位路径。**主平台**以 **.rknn** 为主：

- **RKNN（主）**：`model_path: /opt/models/yolo_v26_640.rknn`、`label_path: /opt/models/coco80.txt`
- ONNX：仅在不使用 RK 平台时可选，如 `model_path: /opt/models/yolo_v26_640.onnx`

请将上述路径改为本机或板端的**实际路径**，或按下方方式准备模型后再修改 yaml。

## 2. RKNN 模型（主平台：RK1126B / RK3576 / RK3588）

- 使用 **RKNN-Toolkit2** 将 ONNX 转为 `.rknn`，并按目标芯片选择 RK1126B/RK3576/RK3588 的量化与配置。
- 文档与工具见 [Rockchip RKNN Toolkit2](https://github.com/rockchip-linux/rknn-toolkit2)。
- 转换得到 `yolo_xxx.rknn` 后，在 yaml 中设置 `model_path` 为该文件路径；`input_width`/`input_height`、`score_threshold`/`nms_threshold` 与模型及 SDK 后处理一致。

## 3. ONNX 模型（非主路径，可选）

### 3.1 从哪里获取

- **YOLOv8 / YOLOv11**（推荐）  
  - [Ultralytics](https://github.com/ultralytics/ultralytics)：安装后可用 `yolo export model=yolov8n.pt format=onnx imgsz=640` 导出 ONNX。  
  - 导出后得到 `yolov8n.onnx` 等，可重命名为 `yolo_v26_640.onnx` 或保持原名，在 yaml 中写对应路径。
- **ONNX Model Zoo**  
  - [onnx/models](https://github.com/onnx/models)：含部分检测/分类模型，可按需选用并对照输入/输出维度。
- **自训练模型**  
  - 导出为 ONNX，保证输入为 NCHW、float32，输出格式与后处理代码一致（见 SDK 中 OnnxRuntimeDetectorBackend 的说明）。

### 3.2 放置目录建议

可统一放在同一目录，便于配置：

```bash
export MODELS_DIR=/opt/models   # 或 $HOME/models
mkdir -p $MODELS_DIR
# 将 yolo_xxx.onnx、coco80.txt 等放入 $MODELS_DIR
```

在 `detectors_demo.yaml` 中设置：

```yaml
model_path: /opt/models/yolov8n.onnx
label_path: /opt/models/coco80.txt
```

### 3.3 类别标签文件（label_path）

- **COCO 80 类**：可自行创建 `coco80.txt`，每行一个类别名（如 `person`、`car`、…），顺序与模型输出 class_id 一致。
- 若模型使用其他类别集，请按相同格式编写对应 `label_path` 文件。

## 4. 使用下载脚本（可选）

若仓库提供 `scripts/download_models.sh`，可在 SDK 根目录执行：

```bash
./scripts/download_models.sh
```

脚本可将测试用 ONNX 导出到默认目录（如 `./models`），并提示在 `detectors_demo.yaml` 中填写的路径。**主平台**需再将 ONNX 用 RKNN-Toolkit2 转为 .rknn 后配置 `model_path`。

## 5. 与 detectors_demo.yaml 的对应关系

| yaml 字段       | 说明 |
|-----------------|------|
| `model_path`    | 模型文件绝对或相对路径（.onnx / .rknn / .engine） |
| `label_path`    | 类别标签文件路径（可选） |
| `input_width` / `input_height` | 与模型输入尺寸一致（如 640） |
| `score_threshold` / `nms_threshold` | 与 SDK 后处理参数一致 |

修改 yaml 后，重新运行 detector_config_demo 或使用该配置的 Pipeline 即可加载对应模型。
