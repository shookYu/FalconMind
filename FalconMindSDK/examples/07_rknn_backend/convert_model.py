#!/usr/bin/env python3
"""
RKNN Model Conversion Script

Converts ONNX models to RKNN format for Rockchip NPU deployment.

Usage:
    python3 convert_model.py --input model.onnx --output model.rknn --target rk3588
    
Requirements:
    - RKNN Toolkit2: pip install rknn-toolkit2
    - ONNX model file

Supported platforms:
    - rk3588 (default)
    - rk3576
    - rv1126b
"""

import argparse
import sys
import os

def check_rknn_toolkit():
    """Check if RKNN Toolkit2 is installed."""
    try:
        from rknn.api import RKNN
        return True
    except ImportError:
        print("Error: RKNN Toolkit2 not found")
        print("Please install: pip install rknn-toolkit2")
        return False

def convert_onnx_to_rknn(input_path, output_path, target_platform='rk3588', 
                         input_size=(640, 640), quant=False):
    """
    Convert ONNX model to RKNN format.
    
    Args:
        input_path: Path to ONNX model
        output_path: Path to save RKNN model
        target_platform: Target platform (rk3588, rk3576, rv1126b)
        input_size: Model input size (width, height)
        quant: Enable quantization (requires dataset)
    """
    from rknn.api import RKNN
    
    print(f"Converting {input_path} to RKNN format...")
    print(f"Target platform: {target_platform}")
    print(f"Input size: {input_size}")
    
    # Create RKNN object
    rknn = RKNN(verbose=True)
    
    # Configure model
    print("\n[1/5] Configuring model...")
    rknn.config(
        mean_values=[[0, 0, 0]],
        std_values=[[255, 255, 255]],
        target_platform=target_platform,
        optimization_level=3,
        quant_img_RGB2BGR=False
    )
    
    # Load ONNX model
    print("\n[2/5] Loading ONNX model...")
    width, height = input_size
    ret = rknn.load_onnx(
        model=input_path,
        inputs=['images'],
        input_size_list=[[1, 3, height, width]],
        outputs=['output0']
    )
    if ret != 0:
        print("Error: Failed to load ONNX model")
        return False
    print("ONNX model loaded successfully")
    
    # Build model
    print("\n[3/5] Building RKNN model...")
    dataset = None
    if quant:
        dataset = 'dataset.txt'
        print(f"Quantization enabled with dataset: {dataset}")
    
    ret = rknn.build(
        do_quantization=quant,
        dataset=dataset if quant else None,
        rknn_batch_size=1
    )
    if ret != 0:
        print("Error: Failed to build RKNN model")
        return False
    print("RKNN model built successfully")
    
    # Export RKNN model
    print("\n[4/5] Exporting RKNN model...")
    ret = rknn.export_rknn(output_path)
    if ret != 0:
        print("Error: Failed to export RKNN model")
        return False
    print(f"RKNN model saved to: {output_path}")
    
    # Initialize runtime (optional, for verification)
    print("\n[5/5] Initializing runtime for verification...")
    ret = rknn.init_runtime(target=target_platform)
    if ret != 0:
        print("Warning: Failed to initialize runtime (this is normal without NPU)")
    else:
        print("Runtime initialized successfully")
    
    # Release
    rknn.release()
    
    print("\n" + "="*80)
    print("Conversion complete!")
    print(f"Output: {output_path}")
    print(f"Target: {target_platform}")
    print(f"Input size: {width}x{height}")
    print("="*80)
    
    return True

def create_test_model(output_path='test.rknn'):
    """Create a minimal test RKNN model for validation."""
    print("Creating test RKNN model...")
    
    try:
        from rknn.api import RKNN
        
        rknn = RKNN()
        
        # Configure
        rknn.config(target_platform='rk3588')
        
        # For testing, we'll create a simple model
        # In practice, you'd use an actual ONNX model
        print("Note: This creates a dummy model for testing only")
        print("For real inference, convert an actual YOLO model:")
        print("  python3 convert_model.py --input yolov8n.onnx --output yolov8n.rknn")
        
        # Build with default settings
        rknn.build(do_quantization=False)
        rknn.export_rknn(output_path)
        rknn.release()
        
        print(f"Test model created: {output_path}")
        return True
        
    except Exception as e:
        print(f"Error creating test model: {e}")
        print("\nNote: RKNN Toolkit2 is required for model conversion.")
        print("Install with: pip install rknn-toolkit2")
        return False

def main():
    parser = argparse.ArgumentParser(
        description='Convert ONNX models to RKNN format for Rockchip NPU'
    )
    parser.add_argument('--input', '-i', type=str,
                        help='Input ONNX model path')
    parser.add_argument('--output', '-o', type=str, default='model.rknn',
                        help='Output RKNN model path (default: model.rknn)')
    parser.add_argument('--target', '-t', type=str, default='rk3588',
                        choices=['rk3588', 'rk3576', 'rv1126b'],
                        help='Target platform (default: rk3588)')
    parser.add_argument('--width', '-w', type=int, default=640,
                        help='Input width (default: 640)')
    parser.add_argument('--height', type=int, default=640,
                        help='Input height (default: 640)')
    parser.add_argument('--quant', '-q', action='store_true',
                        help='Enable quantization (requires dataset)')
    parser.add_argument('--test', action='store_true',
                        help='Create a test model (for validation)')
    
    args = parser.parse_args()
    
    # Check if RKNN Toolkit is available
    if not check_rknn_toolkit():
        print("\nRKNN Toolkit2 is required but not installed.")
        print("For QEMU simulation, the example will run in stub mode.")
        print("Install RKNN Toolkit2 for real model conversion:")
        print("  pip install rknn-toolkit2")
        sys.exit(1)
    
    # Create test model if requested
    if args.test:
        success = create_test_model(args.output)
        sys.exit(0 if success else 1)
    
    # Validate input
    if not args.input:
        print("Error: Input model path required (--input)")
        parser.print_help()
        sys.exit(1)
    
    if not os.path.exists(args.input):
        print(f"Error: Input file not found: {args.input}")
        sys.exit(1)
    
    # Convert model
    success = convert_onnx_to_rknn(
        input_path=args.input,
        output_path=args.output,
        target_platform=args.target,
        input_size=(args.width, args.height),
        quant=args.quant
    )
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
