/**
 * Example 21: RKNN INT8 Quantization
 * Converts FP32 models to INT8 for 2-4x faster NPU inference
 */

#include <iostream>
#include <fstream>

const char* QUANT_SCRIPT = R"(
#!/usr/bin/env python3
\"\"\"RKNN INT8 Quantization\"\"\"
from rknn.api import RKNN

def quantize(onnx_path, rknn_path):
    rknn = RKNN()
    rknn.config(mean_values=[[127.5]*3], std_values=[[127.5]*3],
                target_platform='rk3588')
    rknn.load_onnx(model=onnx_path)
    rknn.build(do_quantization=True)
    rknn.export_rknn(rknn_path)
    print(f\"✓ Quantized: {rknn_path}\")

if __name__ == \"__main__\":
    import sys
    quantize(sys.argv[1], sys.argv[2])
)";

int main() {
    std::cout << "RKNN INT8 Quantization Example\n\n";
    std::cout << "Creates Python script for model quantization.\n";
    std::cout << "Usage: python3 quantize.py model.onnx model_int8.rknn\n\n";
    
    std::ofstream f("quantize.py");
    f << QUANT_SCRIPT;
    std::cout << "✓ Created: quantize.py\n";
    return 0;
}
