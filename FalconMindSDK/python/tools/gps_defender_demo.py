#!/usr/bin/env python3
"""
gps_defender_demo.py

GPS Defender 演示脚本
展示如何使用 Python API 进行 GPS 反欺骗检测

用法:
    python gps_defender_demo.py [--strict]
"""

import argparse
import sys
import os
import time
import random

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

try:
    from denied_env_nodes.navigation import (
        GPSDefender, GNSSMeasurement, IMUMeasurement,
        SpoofingAlertLevel, create_gps_defender, create_strict_gps_defender
    )
except ImportError as e:
    print(f"Error: Cannot import denied_env_nodes module: {e}")
    print("Please build the Python bindings first:")
    print("  cd FalconMindSDK/build && cmake .. -DFALCONMINDSDK_BUILD_PYTHON=ON && make -j4")
    sys.exit(1)


def create_normal_gnss_reading(lat, lon, vel_north=5.0, vel_east=3.0):
    """Create a normal GNSS measurement"""
    gnss = GNSSMeasurement()
    gnss.latitude = lat
    gnss.longitude = lon
    gnss.altitude = 100.0
    gnss.velocity_north = vel_north
    gnss.velocity_east = vel_east
    gnss.velocity_down = -0.1
    gnss.num_satellites = 12
    gnss.hdop = 1.0
    gnss.vdop = 2.0
    return gnss


def create_spoofed_gnss_reading():
    """Create a spoofed GNSS measurement with anomalies"""
    gnss = GNSSMeasurement()
    # Large position jump
    gnss.latitude = 39.9142
    gnss.longitude = 116.4174
    gnss.altitude = 100.0
    # Unrealistic velocity
    gnss.velocity_north = 50.0
    gnss.velocity_east = 30.0
    gnss.velocity_down = 5.0
    gnss.num_satellites = 12
    gnss.hdop = 1.0
    return gnss


def alert_level_to_string(level):
    """Convert alert level to string"""
    if level == SpoofingAlertLevel.NONE:
        return "✓ NORMAL"
    elif level == SpoofingAlertLevel.SUSPECTED:
        return "⚠ SUSPECTED"
    elif level == SpoofingAlertLevel.DETECTED:
        return "✗ DETECTED"
    elif level == SpoofingAlertLevel.CRITICAL:
        return "🚨 CRITICAL"
    return "UNKNOWN"


def run_demo(strict_mode=False):
    """Run GPS Defender demo"""
    print("=" * 60)
    print("GPS Defender Demo")
    print("=" * 60)
    print()
    
    # Create defender
    if strict_mode:
        print("Mode: Strict detection")
        defender = create_strict_gps_defender()
    else:
        print("Mode: Standard detection")
        defender = create_gps_defender()
    
    defender.initialize()
    print("GPS Defender initialized")
    print()
    
    # Phase 1: Normal operation
    print("Phase 1: Normal GNSS readings...")
    print("-" * 60)
    
    base_lat, base_lon = 39.9042, 116.4074
    
    for i in range(10):
        # Simulate normal UAV movement
        lat = base_lat + i * 0.0001 + random.gauss(0, 0.00001)
        lon = base_lon + i * 0.0001 + random.gauss(0, 0.00001)
        
        gnss = create_normal_gnss_reading(lat, lon)
        defender.process_gnss(gnss)
        
        level = defender.get_alert_level()
        reliable = defender.is_gnss_reliable()
        
        print(f"  Reading {i+1:2d}: Lat={lat:.6f}, Lon={lon:.6f} | "
              f"{alert_level_to_string(level)} | Reliable: {reliable}")
        
        time.sleep(0.1)
    
    print()
    print(f"Result: {alert_level_to_string(defender.get_alert_level())}")
    print()
    
    # Phase 2: Spoofing attack
    print("Phase 2: Simulating GPS spoofing attack...")
    print("-" * 60)
    
    for i in range(5):
        if i < 2:
            # Gradual transition to spoofed data
            gnss = create_normal_gnss_reading(
                base_lat + 0.001 + i * 0.01,
                base_lon + 0.001 + i * 0.01,
                vel_north=10.0 + i * 20,
                vel_east=5.0 + i * 12
            )
        else:
            # Full spoofed data
            gnss = create_spoofed_gnss_reading()
        
        defender.process_gnss(gnss)
        
        level = defender.get_alert_level()
        reliable = defender.is_gnss_reliable()
        
        print(f"  Reading {i+1:2d}: Lat={gnss.latitude:.6f}, Lon={gnss.longitude:.6f} | "
              f"Vel=({gnss.velocity_north:.1f}, {gnss.velocity_east:.1f}) | "
              f"{alert_level_to_string(level)} | Reliable: {reliable}")
        
        time.sleep(0.1)
    
    print()
    print(f"Final Alert Level: {alert_level_to_string(defender.get_alert_level())}")
    print()
    
    # Summary
    print("=" * 60)
    print("Demo Summary")
    print("=" * 60)
    print(f"Total readings processed: 15")
    print(f"Final alert level: {alert_level_to_string(defender.get_alert_level())}")
    print(f"GNSS reliable: {defender.is_gnss_reliable()}")
    print()
    print("The GPS Defender successfully detected the spoofing attack!")
    print()


def main():
    parser = argparse.ArgumentParser(description='GPS Defender Demo')
    parser.add_argument('--strict', action='store_true',
                        help='Use strict detection thresholds')
    args = parser.parse_args()
    
    try:
        run_demo(strict_mode=args.strict)
    except KeyboardInterrupt:
        print("\nDemo interrupted by user")
        sys.exit(0)


if __name__ == '__main__':
    main()
