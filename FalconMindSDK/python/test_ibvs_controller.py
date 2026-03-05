#!/usr/bin/env python3
"""
IBVS Controller Python Bindings Test

测试 denied_env_nodes.control 模块的 IBVS Controller 功能
"""

import unittest
import sys
import os
import math

# Add build directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

try:
    from denied_env_nodes.control import (
        IBVSController, IBVSConfig, CameraParameters, ImageSpaceTarget,
        VelocityCommand, TrackingQuality,
        create_ibvs_controller, create_conservative_ibvs_controller,
        create_aggressive_ibvs_controller
    )
    BINDINGS_AVAILABLE = True
except ImportError as e:
    print(f"Warning: denied_env_nodes.control module not available: {e}")
    BINDINGS_AVAILABLE = False


@unittest.skipUnless(BINDINGS_AVAILABLE, "Python bindings not available")
class TestIBVSConfig(unittest.TestCase):
    """Test IBVS configuration"""
    
    def test_default_config(self):
        """Test default configuration values"""
        config = IBVSConfig()
        self.assertIsNotNone(config)
        self.assertGreater(config.desired_distance, 0)
        self.assertGreater(config.desired_height, 0)
        self.assertGreater(config.max_speed, 0)
    
    def test_custom_config(self):
        """Test custom configuration"""
        config = IBVSConfig()
        config.desired_distance = 10.0
        config.desired_height = 5.0
        config.distance_tolerance = 0.5
        config.height_tolerance = 0.3
        config.max_speed = 3.0
        config.max_vertical_speed = 1.0
        config.max_yaw_rate = 0.5
        
        self.assertEqual(config.desired_distance, 10.0)
        self.assertEqual(config.desired_height, 5.0)
        self.assertEqual(config.max_speed, 3.0)


@unittest.skipUnless(BINDINGS_AVAILABLE, "Python bindings not available")
class TestCameraParameters(unittest.TestCase):
    """Test camera parameters"""
    
    def test_camera_parameters(self):
        """Test camera parameter structure"""
        cam = CameraParameters()
        cam.width = 640
        cam.height = 480
        cam.fx = 500.0
        cam.fy = 500.0
        cam.cx = 320.0
        cam.cy = 240.0
        
        self.assertEqual(cam.width, 640)
        self.assertEqual(cam.height, 480)
        self.assertEqual(cam.fx, 500.0)
    
    def test_image_center(self):
        """Test getting image center"""
        cam = CameraParameters()
        cam.width = 640
        cam.height = 480
        cam.cx = 320.0
        cam.cy = 240.0
        
        center = cam.get_image_center()
        self.assertEqual(len(center), 2)
        self.assertEqual(center[0], 320.0)
        self.assertEqual(center[1], 240.0)


@unittest.skipUnless(BINDINGS_AVAILABLE, "Python bindings not available")
class TestImageSpaceTarget(unittest.TestCase):
    """Test image space target"""
    
    def test_target_creation(self):
        """Test target creation"""
        target = ImageSpaceTarget()
        target.u = 320.0
        target.v = 240.0
        target.area_ratio = 0.15
        
        self.assertEqual(target.u, 320.0)
        self.assertEqual(target.v, 240.0)
        self.assertEqual(target.area_ratio, 0.15)
    
    def test_target_from_pixel(self):
        """Test creating target from pixel coordinates"""
        cam = CameraParameters()
        cam.width = 640
        cam.height = 480
        cam.cx = 320.0
        cam.cy = 240.0
        
        target = ImageSpaceTarget.from_pixel(400, 300, cam)
        self.assertIsNotNone(target)


@unittest.skipUnless(BINDINGS_AVAILABLE, "Python bindings not available")
class TestVelocityCommand(unittest.TestCase):
    """Test velocity command"""
    
    def test_velocity_command(self):
        """Test velocity command structure"""
        cmd = VelocityCommand()
        cmd.vx = 1.0
        cmd.vy = 0.5
        cmd.vz = -0.3
        cmd.yaw_rate = 0.1
        
        self.assertEqual(cmd.vx, 1.0)
        self.assertEqual(cmd.vy, 0.5)
        self.assertEqual(cmd.vz, -0.3)
        self.assertEqual(cmd.yaw_rate, 0.1)
    
    def test_velocity_saturate(self):
        """Test velocity saturation"""
        cmd = VelocityCommand()
        cmd.vx = 10.0
        cmd.vy = 10.0
        cmd.vz = 5.0
        cmd.yaw_rate = 2.0
        
        cmd.saturate(max_v_xy=3.0, max_v_z=1.0, max_yaw_rate=0.5)
        
        # After saturation, should be within limits
        self.assertLessEqual(abs(cmd.vx), 3.0)
        self.assertLessEqual(abs(cmd.vy), 3.0)
        self.assertLessEqual(abs(cmd.vz), 1.0)
        self.assertLessEqual(abs(cmd.yaw_rate), 0.5)


@unittest.skipUnless(BINDINGS_AVAILABLE, "Python bindings not available")
class TestIBVSController(unittest.TestCase):
    """Test IBVS controller"""
    
    def test_controller_creation(self):
        """Test controller creation"""
        config = IBVSConfig()
        controller = IBVSController(config)
        self.assertIsNotNone(controller)
    
    def test_controller_factories(self):
        """Test controller factory functions"""
        ctrl1 = create_ibvs_controller()
        self.assertIsNotNone(ctrl1)
        
        ctrl2 = create_conservative_ibvs_controller()
        self.assertIsNotNone(ctrl2)
        
        ctrl3 = create_aggressive_ibvs_controller()
        self.assertIsNotNone(ctrl3)
    
    def test_initialization(self):
        """Test controller initialization"""
        controller = create_ibvs_controller()
        result = controller.initialize()
        # Just check it doesn't crash
    
    def test_get_config(self):
        """Test getting controller configuration"""
        controller = create_ibvs_controller()
        config = controller.get_config()
        self.assertIsNotNone(config)
        self.assertGreater(config.desired_distance, 0)
    
    def test_update_config(self):
        """Test updating controller configuration"""
        controller = create_ibvs_controller()
        
        new_config = IBVSConfig()
        new_config.desired_distance = 15.0
        new_config.desired_height = 8.0
        
        controller.update_config(new_config)
        
        updated_config = controller.get_config()
        self.assertEqual(updated_config.desired_distance, 15.0)
        self.assertEqual(updated_config.desired_height, 8.0)
    
    def test_set_desired_distance(self):
        """Test setting desired distance"""
        controller = create_ibvs_controller()
        controller.set_desired_distance(12.0)
        
        config = controller.get_config()
        self.assertEqual(config.desired_distance, 12.0)
    
    def test_set_desired_height(self):
        """Test setting desired height"""
        controller = create_ibvs_controller()
        controller.set_desired_height(6.0)
        
        config = controller.get_config()
        self.assertEqual(config.desired_height, 6.0)
    
    def test_compute_control(self):
        """Test computing control command"""
        controller = create_ibvs_controller()
        controller.initialize()
        
        # Create target at center of image
        target = ImageSpaceTarget()
        target.u = 0.0  # Center
        target.v = 0.0
        target.area_ratio = 0.1
        
        # Current state
        current_distance = 15.0
        current_height = 10.0
        
        cmd = controller.compute_control(target, current_distance, current_height)
        
        self.assertIsNotNone(cmd)
        # Command should be reasonable (not NaN or infinity)
        self.assertFalse(math.isnan(cmd.vx))
        self.assertFalse(math.isnan(cmd.vy))
        self.assertFalse(math.isnan(cmd.vz))
    
    def test_compute_quality(self):
        """Test computing tracking quality"""
        controller = create_ibvs_controller()
        controller.initialize()
        
        target = ImageSpaceTarget()
        target.u = 0.0
        target.v = 0.0
        target.area_ratio = 0.1
        
        quality = controller.compute_quality(15.0, 10.0, target)
        
        self.assertIsNotNone(quality)
        self.assertGreaterEqual(quality.quality_score, 0.0)
        self.assertLessEqual(quality.quality_score, 1.0)
    
    def test_is_at_target(self):
        """Test checking if at target"""
        controller = create_ibvs_controller()
        controller.initialize()
        
        target = ImageSpaceTarget()
        target.u = 0.0
        target.v = 0.0
        target.area_ratio = 0.1
        
        # Should not be at target initially
        is_at = controller.is_at_target(20.0, 15.0, target)
        self.assertFalse(is_at)
    
    def test_reset(self):
        """Test controller reset"""
        controller = create_ibvs_controller()
        controller.initialize()
        controller.reset()
        # Just check it doesn't crash


@unittest.skipUnless(BINDINGS_AVAILABLE, "Python bindings not available")
class TestTrackingQuality(unittest.TestCase):
    """Test tracking quality structure"""
    
    def test_quality_structure(self):
        """Test tracking quality fields"""
        quality = TrackingQuality()
        quality.distance_error = 2.0
        quality.position_error_u = 10.0
        quality.position_error_v = 5.0
        quality.quality_score = 0.85
        quality.is_distance_good = True
        quality.is_position_good = True
        
        self.assertEqual(quality.distance_error, 2.0)
        self.assertEqual(quality.quality_score, 0.85)
        self.assertTrue(quality.is_distance_good)


@unittest.skipUnless(BINDINGS_AVAILABLE, "Python bindings not available")
class TestIBVSIntegration(unittest.TestCase):
    """Integration tests for IBVS controller"""
    
    def test_tracking_scenario(self):
        """Test a complete tracking scenario"""
        controller = create_ibvs_controller()
        controller.initialize()
        
        # Simulate target moving in image
        for i in range(20):
            target = ImageSpaceTarget()
            # Target moving from right to center
            target.u = 50 - i * 2.5
            target.v = 30 - i * 1.5
            target.area_ratio = 0.08 + i * 0.001
            
            distance = 20.0 - i * 0.3
            height = 15.0
            
            cmd = controller.compute_control(target, distance, height)
            quality = controller.compute_quality(distance, height, target)
            
            # Check command validity
            self.assertFalse(math.isnan(cmd.vx))
            self.assertFalse(math.isnan(cmd.vy))
            
            # Check quality
            self.assertGreaterEqual(quality.quality_score, 0.0)
            self.assertLessEqual(quality.quality_score, 1.0)
            
            # Check if we've reached target
            if controller.is_at_target(distance, height, target):
                break


def run_tests():
    """Run all tests"""
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    # Add all test classes
    suite.addTests(loader.loadTestsFromTestCase(TestIBVSConfig))
    suite.addTests(loader.loadTestsFromTestCase(TestCameraParameters))
    suite.addTests(loader.loadTestsFromTestCase(TestImageSpaceTarget))
    suite.addTests(loader.loadTestsFromTestCase(TestVelocityCommand))
    suite.addTests(loader.loadTestsFromTestCase(TestIBVSController))
    suite.addTests(loader.loadTestsFromTestCase(TestTrackingQuality))
    suite.addTests(loader.loadTestsFromTestCase(TestIBVSIntegration))
    
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return result.wasSuccessful()


if __name__ == '__main__':
    success = run_tests()
    sys.exit(0 if success else 1)
