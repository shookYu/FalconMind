#!/usr/bin/env python3
"""
GPS Defender Python Bindings Test

测试 denied_env_nodes.navigation 模块的 GPS Defender 功能
"""

import unittest
import sys
import os

# Add build directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

try:
    from denied_env_nodes.navigation import (
        GPSDefender, GPSDefenderConfig, GNSSMeasurement, IMUMeasurement,
        VisualPosition, SpoofingReport, SpoofingAlertLevel,
        create_gps_defender, create_strict_gps_defender
    )
    BINDINGS_AVAILABLE = True
except ImportError as e:
    print(f"Warning: denied_env_nodes module not available: {e}")
    BINDINGS_AVAILABLE = False


@unittest.skipUnless(BINDINGS_AVAILABLE, "Python bindings not available")
class TestGPSDefender(unittest.TestCase):
    """Test GPS Defender Python bindings"""
    
    def test_config_default(self):
        """Test default configuration"""
        config = GPSDefenderConfig()
        self.assertIsNotNone(config)
        # Check default values
        self.assertGreater(config.raim_threshold, 0)
        self.assertGreater(config.velocity_diff_threshold, 0)
    
    def test_config_custom(self):
        """Test custom configuration"""
        config = GPSDefenderConfig()
        config.raim_threshold = 5.0
        config.velocity_diff_threshold = 3.0
        config.position_diff_threshold = 10.0
        
        self.assertEqual(config.raim_threshold, 5.0)
        self.assertEqual(config.velocity_diff_threshold, 3.0)
        self.assertEqual(config.position_diff_threshold, 10.0)
    
    def test_gps_defender_creation(self):
        """Test GPS Defender creation"""
        config = GPSDefenderConfig()
        defender = GPSDefender(config)
        self.assertIsNotNone(defender)
    
    def test_gps_defender_factory(self):
        """Test GPS Defender factory functions"""
        defender1 = create_gps_defender()
        self.assertIsNotNone(defender1)
        
        defender2 = create_strict_gps_defender()
        self.assertIsNotNone(defender2)
    
    def test_gnss_measurement(self):
        """Test GNSS measurement structure"""
        gnss = GNSSMeasurement()
        gnss.latitude = 39.9042
        gnss.longitude = 116.4074
        gnss.altitude = 100.0
        gnss.velocity_north = 5.0
        gnss.velocity_east = 3.0
        gnss.velocity_down = -0.5
        gnss.num_satellites = 12
        gnss.hdop = 1.2
        gnss.vdop = 2.1
        
        self.assertAlmostEqual(gnss.latitude, 39.9042, places=4)
        self.assertAlmostEqual(gnss.longitude, 116.4074, places=4)
        self.assertEqual(gnss.num_satellites, 12)
    
    def test_imu_measurement(self):
        """Test IMU measurement structure"""
        imu = IMUMeasurement()
        imu.accel = [0.0, 0.0, 9.81]
        imu.gyro = [0.01, 0.02, 0.03]
        imu.temperature = 25.0
        
        self.assertEqual(len(imu.accel), 3)
        self.assertEqual(len(imu.gyro), 3)
    
    def test_visual_position(self):
        """Test visual position structure"""
        pos = VisualPosition()
        pos.north = 10.0
        pos.east = 20.0
        pos.down = -5.0
        pos.confidence = 0.95
        
        self.assertAlmostEqual(pos.north, 10.0)
        self.assertAlmostEqual(pos.confidence, 0.95)
    
    def test_spoofing_alert_levels(self):
        """Test spoofing alert level enum"""
        self.assertEqual(SpoofingAlertLevel.NONE.value, 0)
        self.assertEqual(SpoofingAlertLevel.SUSPECTED.value, 1)
        self.assertEqual(SpoofingAlertLevel.DETECTED.value, 2)
        self.assertEqual(SpoofingAlertLevel.CRITICAL.value, 3)
    
    def test_spoofing_report(self):
        """Test spoofing report structure"""
        report = SpoofingReport()
        report.level = SpoofingAlertLevel.SUSPECTED
        report.confidence = 0.75
        report.reason = "Velocity mismatch detected"
        report.recommended_action = "Continue monitoring"
        
        self.assertEqual(report.level, SpoofingAlertLevel.SUSPECTED)
        self.assertAlmostEqual(report.confidence, 0.75)
    
    def test_defender_methods(self):
        """Test defender methods"""
        defender = create_gps_defender()
        
        # Test initialization
        result = defender.initialize()
        # Just check it doesn't crash
        
        # Test reset
        defender.reset()
        
        # Check initial state
        level = defender.get_alert_level()
        self.assertEqual(level, SpoofingAlertLevel.NONE)
        
        reliability = defender.is_gnss_reliable()
        # Should be True initially
        self.assertTrue(reliability)


@unittest.skipUnless(BINDINGS_AVAILABLE, "Python bindings not available")
class TestGPSDefenderIntegration(unittest.TestCase):
    """Integration tests for GPS Defender"""
    
    def test_normal_gnss_processing(self):
        """Test normal GNSS data processing"""
        defender = create_gps_defender()
        defender.initialize()
        
        # Simulate normal GNSS readings
        for i in range(10):
            gnss = GNSSMeasurement()
            gnss.latitude = 39.9042 + i * 0.0001
            gnss.longitude = 116.4074 + i * 0.0001
            gnss.altitude = 100.0
            gnss.velocity_north = 5.0
            gnss.velocity_east = 3.0
            gnss.velocity_down = 0.0
            gnss.num_satellites = 12
            gnss.hdop = 1.0
            
            defender.process_gnss(gnss)
        
        # Should still be reliable
        self.assertTrue(defender.is_gnss_reliable())
        self.assertEqual(defender.get_alert_level(), SpoofingAlertLevel.NONE)
    
    def test_spoofing_detection(self):
        """Test spoofing detection with anomalous data"""
        defender = create_strict_gps_defender()
        defender.initialize()
        
        # First, provide normal data
        for i in range(5):
            gnss = GNSSMeasurement()
            gnss.latitude = 39.9042
            gnss.longitude = 116.4074
            gnss.velocity_north = 5.0
            gnss.velocity_east = 3.0
            gnss.num_satellites = 12
            defender.process_gnss(gnss)
        
        # Then, simulate spoofing with jump in position
        gnss_spoofed = GNSSMeasurement()
        gnss_spoofed.latitude = 39.9142  # 1km jump
        gnss_spoofed.longitude = 116.4174
        gnss_spoofed.velocity_north = 50.0  # Unrealistic velocity
        gnss_spoofed.velocity_east = 30.0
        gnss_spoofed.num_satellites = 12
        
        defender.process_gnss(gnss_spoofed)
        
        # Alert level should increase
        level = defender.get_alert_level()
        self.assertIn(level, [SpoofingAlertLevel.SUSPECTED, SpoofingAlertLevel.DETECTED])


def run_tests():
    """Run all tests"""
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    # Add all test classes
    suite.addTests(loader.loadTestsFromTestCase(TestGPSDefender))
    suite.addTests(loader.loadTestsFromTestCase(TestGPSDefenderIntegration))
    
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)
    
    return result.wasSuccessful()


if __name__ == '__main__':
    success = run_tests()
    sys.exit(0 if success else 1)
