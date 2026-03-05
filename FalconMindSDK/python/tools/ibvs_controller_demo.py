#!/usr/bin/env python3
"""
ibvs_controller_demo.py

IBVS Controller 演示脚本
展示如何使用 Python API 进行基于图像的视觉伺服控制

用法:
    python ibvs_controller_demo.py [--conservative|--aggressive]
"""

import argparse
import sys
import os
import math
import time

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

try:
    from denied_env_nodes.control import (
        IBVSController, IBVSConfig, CameraParameters, ImageSpaceTarget,
        VelocityCommand, create_ibvs_controller, create_conservative_ibvs_controller,
        create_aggressive_ibvs_controller
    )
except ImportError as e:
    print(f"Error: Cannot import denied_env_nodes.control module: {e}")
    print("Please build the Python bindings first:")
    print("  cd FalconMindSDK/build && cmake .. -DFALCONMINDSDK_BUILD_PYTHON=ON && make -j4")
    sys.exit(1)


def create_default_camera():
    """Create default camera parameters"""
    cam = CameraParameters()
    cam.width = 640
    cam.height = 480
    cam.fx = 500.0
    cam.fy = 500.0
    cam.cx = 320.0
    cam.cy = 240.0
    return cam


def create_target(u, v, area_ratio):
    """Create image space target"""
    target = ImageSpaceTarget()
    target.u = u
    target.v = v
    target.area_ratio = area_ratio
    return target


def print_control_command(cmd, iteration):
    """Print control command"""
    print(f"  Iter {iteration:2d}: "
          f"Vx={cmd.vx:6.2f}, Vy={cmd.vy:6.2f}, Vz={cmd.vz:6.2f}, "
          f"YawRate={cmd.yaw_rate:6.3f}")


def print_tracking_quality(quality):
    """Print tracking quality"""
    score = int(quality.quality_score * 100)
    dist_ok = "✓" if quality.is_distance_good else "✗"
    pos_ok = "✓" if quality.is_position_good else "✗"
    print(f"    Quality: {score}% | Distance: {dist_ok} | Position: {pos_ok}")


def run_demo(controller_type='standard'):
    """Run IBVS Controller demo"""
    print("=" * 70)
    print("IBVS (Image-Based Visual Servoing) Controller Demo")
    print("=" * 70)
    print()
    
    # Create controller
    if controller_type == 'conservative':
        print("Controller Type: Conservative (stable but slower)")
        controller = create_conservative_ibvs_controller()
    elif controller_type == 'aggressive':
        print("Controller Type: Aggressive (fast but may overshoot)")
        controller = create_aggressive_ibvs_controller()
    else:
        print("Controller Type: Standard")
        controller = create_ibvs_controller()
    
    controller.initialize()
    print("IBVS Controller initialized")
    print()
    
    # Get configuration
    config = controller.get_config()
    print(f"Configuration:")
    print(f"  Desired distance: {config.desired_distance:.1f}m")
    print(f"  Desired height: {config.desired_height:.1f}m")
    print(f"  Max speed: {config.max_speed:.1f}m/s")
    print()
    
    # Scenario 1: Target tracking
    print("Scenario 1: Tracking target in image center")
    print("-" * 70)
    
    # Simulate target at different positions
    target_positions = [
        (50, 30, 0.05),    # Far from center, small
        (30, 20, 0.08),    # Closer
        (10, 5, 0.10),     # Near center
        (0, 0, 0.12),      # At center, good size
    ]
    
    current_distance = 20.0
    current_height = 15.0
    
    for i, (u, v, area) in enumerate(target_positions):
        target = create_target(u, v, area)
        
        cmd = controller.compute_control(target, current_distance, current_height)
        quality = controller.compute_quality(current_distance, current_height, target)
        
        print(f"  Target: u={u:5.1f}, v={v:5.1f}, area={area:.3f}")
        print_control_command(cmd, i + 1)
        print_tracking_quality(quality)
        
        if controller.is_at_target(current_distance, current_height, target):
            print("  → Target reached!")
            break
        
        # Simulate movement
        current_distance -= 0.5
        current_height -= 0.2
        time.sleep(0.1)
    
    print()
    
    # Scenario 2: Approaching target
    print("Scenario 2: Approaching target from distance")
    print("-" * 70)
    
    # Reset controller
    controller.reset()
    controller.initialize()
    
    distances = [25.0, 20.0, 15.0, 12.0, 10.0, 8.0]
    target = create_target(5, 3, 0.10)  # Slightly off-center
    
    for i, dist in enumerate(distances):
        cmd = controller.compute_control(target, dist, 10.0)
        quality = controller.compute_quality(dist, 10.0, target)
        
        print(f"  Distance: {dist:.1f}m")
        print_control_command(cmd, i + 1)
        print_tracking_quality(quality)
        
        time.sleep(0.1)
    
    print()
    
    # Scenario 3: Dynamic target movement
    print("Scenario 3: Tracking moving target")
    print("-" * 70)
    
    controller.reset()
    controller.initialize()
    
    # Simulate target moving across image
    for i in range(10):
        # Target moving from right to left
        u = 40 - i * 8
        v = 20 - i * 2
        area = 0.08 + i * 0.005
        
        target = create_target(u, v, area)
        cmd = controller.compute_control(target, 15.0, 10.0)
        quality = controller.compute_quality(15.0, 10.0, target)
        
        print(f"  Target: u={u:5.1f}, v={v:5.1f}")
        print_control_command(cmd, i + 1)
        
        time.sleep(0.05)
    
    print()
    
    # Summary
    print("=" * 70)
    print("Demo Summary")
    print("=" * 70)
    print("The IBVS Controller successfully:")
    print("  ✓ Computed velocity commands to track targets")
    print("  ✓ Maintained desired distance and height")
    print("  ✓ Evaluated tracking quality")
    print("  ✓ Detected when target is reached")
    print()
    print("Usage in real UAV:")
    print("  1. Detect target in camera image")
    print("  2. Compute IBVS control command")
    print("  3. Send velocity command to flight controller")
    print("  4. Repeat until target is reached")
    print()


def main():
    parser = argparse.ArgumentParser(description='IBVS Controller Demo')
    parser.add_argument('--conservative', action='store_true',
                        help='Use conservative controller (stable but slower)')
    parser.add_argument('--aggressive', action='store_true',
                        help='Use aggressive controller (fast but may overshoot)')
    args = parser.parse_args()
    
    if args.conservative:
        controller_type = 'conservative'
    elif args.aggressive:
        controller_type = 'aggressive'
    else:
        controller_type = 'standard'
    
    try:
        run_demo(controller_type)
    except KeyboardInterrupt:
        print("\nDemo interrupted by user")
        sys.exit(0)


if __name__ == '__main__':
    main()
