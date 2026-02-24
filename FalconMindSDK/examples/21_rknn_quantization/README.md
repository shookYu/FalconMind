# Example 21: RKNN Model Quantization (INT8)

## 验证目标

验证 RKNN Toolkit2 的 INT8 量化功能，将 FP32 模型转换为 INT8 格式，实现 2-4 倍的推理加速。

## 验证内容

1. **动态量化** - 无需校准数据集的快速量化
2. **量化感知训练** - 使用校准数据集的高精度量化
3. **模型验证** - 量化前后精度对比
4. **性能测试** - 测量 INT8 模型的推理延迟

## 验证方法

### 软件依赖

```bash
# 必需依赖
- Python >= 3.6
- RKNN Toolkit2

# 安装
pip install rknn-toolkit2

# 验证安装
python3 -c "from rknn.api import RKNN; print('RKNN Toolkit2 OK')"
```

### 硬件依赖

| 设备 | 型号 | 必需 |
|------|------|------|
| 开发机 | x86 PC | 是（模型转换） |
| 目标设备 | RK3588/RK3576 | 是（部署测试） |

### 模型准备

```bash
# 下载 YOLOv8n ONNX 模型
wget https://github.com/ultralytics/assets/releases/download/v8.1.0/yolov8n.onnx

# 或使用自己的模型
# 支持格式: ONNX, PyTorch, TensorFlow, Caffe
```

## 编译步骤

```bash
# 1. 进入示例目录
cd FalconMindSDK/examples/21_rknn_quantization/x86

# 2. 创建构建目录
mkdir -p build && cd build

# 3. 配置
cmake ..

# 4. 编译
make -j4

# 5. 生成 Python 量化脚本
./21_rknn_quantization_x86
```

## 运行步骤

### 方法1: 动态量化（快速）

```bash
# 1. 生成量化脚本
./21_rknn_quantization_x86

# 2. 运行量化
python3 quantize.py yolov8n.onnx yolov8n_int8.rknn

# 预期输出：
[RKNN] Loading ONNX: yolov8n.onnx
[RKNN] Building INT8 model...
[RKNN] Config:
  - mean_values: [[127.5, 127.5, 127.5]]
  - std_values: [[127.5, 127.5, 127.5]]
  - target_platform: rk3588
[RKNN] Building quantization model...
✓ Quantized: yolov8n_int8.rknn

# 3. 验证模型
python3 -c "
from rknn.api import RKNN
rknn = RKNN()
rknn.load_rknn('yolov8n_int8.rknn')
rknn.init_runtime()
print('Model loaded successfully')
rknn.release()
"
```

### 方法2: 使用校准数据集（高精度）

```bash
# 1. 准备校准数据集（每行一个图片路径）
ls /path/to/calibration/images/*.jpg > dataset.txt

# 2. 运行量化
cat > quantize_with_dataset.py <> 'EOF'
from rknn.api import RKNN

rknn = RKNN()
rknn.config(
    mean_values=[[127.5, 127.5, 127.5]],
    std_values=[[127.5, 127.5, 127.5]],
    target_platform='rk3588'
)
rknn.load_onnx(model='yolov8n.onnx')
rknn.build(
    do_quantization=True,
    dataset='dataset.txt'
)
rknn.export_rknn('yolov8n_int8_calib.rknn')
print('Quantization with calibration complete')
EOF

python3 quantize_with_dataset.py
```

### 方法3: 完整性能测试

```bash
# 1. 创建测试脚本
cat > benchmark.py <> 'EOF'
from rknn.api import RKNN
import numpy as np
import time

rknn = RKNN()
rknn.load_rknn('yolov8n_int8.rknn')
rknn.init_runtime()

# 生成测试数据
dummy_input = np.random.randn(1, 3, 640, 640).astype(np.float32)

# Warmup
for _ in range(10):
    rknn.inference(inputs=[dummy_input])

# Benchmark
times = []
for _ in range(100):
    start = time.time()
    rknn.inference(inputs=[dummy_input])
    times.append((time.time() - start) * 1000)

avg_time = sum(times) / len(times)
print(f"Average inference time: {avg_time:.2f}ms")
print(f"FPS: {1000/avg_time:.1f}")

rknn.release()
EOF

# 2. 在 RK3588 上运行
adb push benchmark.py /data/local/tmp/
adb push yolov8n_int8.rknn /data/local/tmp/
adb shell "cd /data/local/tmp && python3 benchmark.py"

# 预期输出：
# Average inference time: 15.23ms
# FPS: 65.7
```

## 期望结果

### 量化前 (FP32)
- 模型大小：~13 MB
- RK3588 推理延迟：~40ms
- 精度：mAP 37.3

### 量化后 (INT8)
- 模型大小：~3.5 MB（4x 减小）
- RK3588 推理延迟：~15ms（2.7x 加速）
- 精度：mAP 36.8（损失 <1%）

### 性能对比

| 模型 | 大小 | RK3588延迟 | 精度 | 加速比 |
|------|------|------------|------|--------|
| FP32 | 13 MB | 40ms | 37.3 | 1.0x |
| INT8 | 3.5 MB | 15ms | 36.8 | 2.7x |

## 故障排除

**问题**: RKNN Toolkit2 安装失败
```bash
# 使用国内镜像
pip install rknn-toolkit2 -i https://pypi.tuna.tsinghua.edu.cn/simple

# 或使用离线安装包
pip install rknn_toolkit2-1.5.0-cp38-cp38-linux_x86_64.whl
```

**问题**: 量化后精度下降严重
```bash
# 使用更多校准数据
# 推荐使用 100-500 张代表性图片

# 调整预处理参数
rknn.config(
    mean_values=[[0, 0, 0]],  # 根据训练配置调整
    std_values=[[255, 255, 255]]
)
```

**问题**: 模型转换失败
```bash
# 检查 ONNX opset
python3 -c "
import onnx
model = onnx.load('model.onnx')
print(f'Opset: {model.opset_import[0].version}')
"

# 使用 opset 11 或 12
python3 -m onnxsim model.onnx model_sim.onnx --opset 12
```

## 参考文档

- [RKNN Toolkit2 文档](https://github.com/rockchip-linux/rknn-toolkit2)
- [量化原理介绍](https://arxiv.org/abs/2004.09602)
- [RKNN 模型 Zoo](https://github.com/rockchip-linux/rknpu2)
