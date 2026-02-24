# Example 07: RKNN Backend - Rockchip NPU Inference

This example demonstrates using RknnDetectorBackend for real NPU inference on Rockchip platforms.

## Overview

- **Platform**: Rockchip RK3588/RK3576/RV1126B
- **Backend**: RknnDetectorBackend
- **Model Format**: RKNN (.rknn)
- **Inference**: Real NPU hardware with RKNN Toolkit2

## Directory Structure

```
07_rknn_backend/
├── rk3588/              # RK3588 platform example
│   ├── src/
│   ├── CMakeLists.txt   # Build configuration
│   └── toolchain.cmake  # Cross-compilation toolchain
├── models/              # Place RKNN models here
├── images/              # Place test images here
├── convert_model.py     # ONNX to RKNN conversion script
├── check_rknn.sh        # RKNN Toolkit2 installation check
└── README.md            # This file
```

## Prerequisites

### 1. RKNN Toolkit2 (Required)

RKNN Toolkit2 provides the runtime library for NPU inference.

**Installation:**

```bash
# Option 1: Install via pip (recommended)
pip install rknn-toolkit2

# Option 2: Download from GitHub
# https://github.com/rockchip-linux/rknn-toolkit2/releases

# Verify installation
./check_rknn.sh
```

### 2. Rockchip NPU Board

- RK3588/RK3576/RV1126B with NPU
- RKNN Toolkit2 Runtime library installed

### 3. Cross Compiler (Optional)

For cross-compilation on x86 host:

```bash
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

## Building

### Step 1: Check RKNN Toolkit2

```bash
cd FalconMindSDK/examples/07_rknn_backend
./check_rknn.sh
```

If RKNN Toolkit2 is not installed, follow the [Prerequisites](#prerequisites) section.

### Step 2: Build on Target Board

```bash
cd FalconMindSDK/examples/07_rknn_backend/rk3588
mkdir -p build && cd build
cmake ..
make -j4
```

### Step 3: Cross-Compile (x86 host)

```bash
cd FalconMindSDK/examples/07_rknn_backend/rk3588
mkdir -p build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain.cmake ..
make -j4
```

## Running

### On Real Hardware (RK3588/RK3576/RV1126B)

```bash
# Copy to target board (if cross-compiled)
scp -r 07_rknn_backend/ root@rk3588-board:/root/

# Run on target board
ssh root@rk3588-board
cd 07_rknn_backend/rk3588/build

# Run with model
./07_rknn_backend_rk3588 /path/to/model.rknn

# Or run with default simulation
./07_rknn_backend_rk3588
```

## Model Conversion

### Convert ONNX to RKNN

```bash
# Install RKNN Toolkit2
pip install rknn-toolkit2

# Convert YOLOv8 model
python3 convert_model.py \
    --input yolov8n.onnx \
    --output models/yolov8n.rknn \
    --target rk3588 \
    --width 640 \
    --height 640
```

### Supported Models

- YOLOv5/v8/v11 series
- MobileNet
- ResNet
- Custom models (via ONNX export)

## Usage

```bash
# Default (simulation mode)
./07_rknn_backend_rk3588

# With specific model
./07_rknn_backend_rk3588 models/yolov8n.rknn

# With model and image
./07_rknn_backend_rk3588 models/yolov8n.rknn images/test.jpg
```

## Expected Output

```
================================================================================
            FalconMindSDK Example 07: RKNN Backend Inference
            Platform: Rockchip RK3588/RK3576/RV1126B
================================================================================

[Platform Detection] Rockchip ARM Platform
  Target: RK3588/RK3576/RV1126B NPU

[1] Creating RKNN Detector Backend
    Backend type: RKNN
    Backend created successfully

[2] Configuring detector parameters
    Model: ../models/yolov8n.rknn
    Mode: Real RKNN NPU inference
    Input size: 640x640
    Classes: 80

[3] Loading model
    Load result: success
    Model loaded: ../models/yolov8n.rknn
    Backend ready for NPU inference

...

[6] Detection Results
    Frame ID: frame_001
    Objects detected: 3

    +------------------------------------------------------------------+
    |  ID |     Class     |  Confidence  |       Bounding Box          |
    +------------------------------------------------------------------+
    |  1  | car           |     0.8700   | ( 280, 320,  80,  80)       |
    |  2  | person        |     0.7200   | ( 100, 426,  30, 150)       |
    |  3  | traffic light |     0.6500   | ( 500, 100,  30,  80)       |
    +------------------------------------------------------------------+
```

## Troubleshooting

### Build Errors

**Error**: `RKNN Toolkit2 not found!`
```bash
# Install RKNN Toolkit2
pip install rknn-toolkit2

# Verify installation
./check_rknn.sh
```

**Error**: `aarch64-linux-gnu-g++ not found`
```bash
sudo apt-get install g++-aarch64-linux-gnu
```

**Error**: `falconmind_sdk library not found`
```bash
# Build FalconMindSDK first
cd FalconMindSDK/build
cmake ..
make -j4
sudo make install
```

### Runtime Errors

**Error**: `cannot open shared library librknn_api.so`
```bash
# Find the library location
find /usr -name "librknn_api.so" 2>/dev/null

# Add to LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/path/to/rknn/lib:$LD_LIBRARY_PATH
```

**Error**: `rknn_init failed`
```bash
# Check model file exists and is valid
ls -la models/yolov8n.rknn

# Ensure target platform matches (rk3588/rk3576/rv1126b)
# Re-convert model if needed
python3 convert_model.py --input yolov8n.onnx --output models/yolov8n.rknn --target rk3588
```

**Error**: `Model platform not match` or similar
```bash
# The RKNN model must match the target platform
# Convert with correct target:
python3 convert_model.py --input model.onnx --output model.rknn --target rk3588
```

## Performance (RK3588 NPU)

| Model | Input Size | NPU Cores | Latency | FPS |
|-------|-----------|-----------|---------|-----|
| **YOLOv8n** | 640x640 | 3 | ~15-25ms | ~40-66 |
| **YOLOv8s** | 640x640 | 3 | ~30-50ms | ~20-33 |
| **YOLOv8m** | 640x640 | 3 | ~60-100ms | ~10-16 |
| **YOLOv8l** | 640x640 | 3 | ~120-200ms | ~5-8 |

### Optimization Tips

1. **Use INT8 Quantization**: Reduces latency by 2-3x
2. **Batch Inference**: Process multiple images simultaneously
3. **Multi-NPU**: RK3588 has 3 NPU cores, can run multiple models
4. **Model Pruning**: Use smaller models (YOLOv8n vs YOLOv8x)

## References

- [RKNN Toolkit2 Documentation](https://github.com/rockchip-linux/rknn-toolkit2)
- [Rockchip NPU Guide](https://opensource.rock-chips.com/wiki_NPU)
- [FalconMindSDK Perception Module](../../docs/perception.md)

## License

Apache License 2.0
