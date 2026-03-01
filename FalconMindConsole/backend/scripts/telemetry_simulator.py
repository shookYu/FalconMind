#!/usr/bin/env python3
"""
Telemetry Simulator
Simulates UAV telemetry data for development and testing
"""
import asyncio
import aiohttp
import random
import math
from datetime import datetime
from typing import Dict, List


class UAVSimulator:
    """Simulates a single UAV"""
    
    def __init__(self, uav_id: str, name: str, start_lat: float, start_lon: float):
        self.uav_id = uav_id
        self.name = name
        self.latitude = start_lat
        self.longitude = start_lon
        self.altitude = 100.0
        self.heading = random.uniform(0, 360)
        self.speed = 0.0
        self.battery = 100.0
        self.satellites = 12
        self.status = "online"
        
        # Waypoints for autonomous flight
        self.waypoints: List[Dict] = []
        self.current_waypoint = 0
        self.is_flying = False
        
    def add_waypoints(self, waypoints: List[Dict]):
        """Add flight waypoints"""
        self.waypoints = waypoints
        
    def update_position(self, dt: float = 1.0):
        """Update UAV position based on current state"""
        if not self.is_flying:
            return
            
        # Simple flight logic towards next waypoint
        if self.current_waypoint < len(self.waypoints):
            target = self.waypoints[self.current_waypoint]
            
            # Calculate distance and bearing to target
            distance = self._haversine_distance(
                self.latitude, self.longitude,
                target['lat'], target['lon']
            )
            
            if distance < 10:  # Reached waypoint
                self.current_waypoint += 1
                if self.current_waypoint >= len(self.waypoints):
                    self.is_flying = False
                    self.speed = 0.0
                    return
                target = self.waypoints[self.current_waypoint]
            
            # Calculate heading to target
            bearing = self._calculate_bearing(
                self.latitude, self.longitude,
                target['lat'], target['lon']
            )
            self.heading = bearing
            
            # Move towards target
            speed = 15.0  # m/s
            self.speed = speed
            
            # Convert speed to lat/lon change
            # Approximate: 1 degree lat ~ 111km, 1 degree lon varies
            distance_moved = speed * dt
            self.latitude += (distance_moved * math.cos(math.radians(bearing))) / 111320
            self.longitude += (distance_moved * math.sin(math.radians(bearing))) / (111320 * math.cos(math.radians(self.latitude)))
            
            # Update altitude gradually
            if 'alt' in target:
                alt_diff = target['alt'] - self.altitude
                self.altitude += alt_diff * 0.1 * dt
        else:
            # Hover in place
            self.speed = 0.0
            
        # Consume battery
        self.battery -= 0.05 * dt
        if self.battery < 0:
            self.battery = 0
            
    def _haversine_distance(self, lat1: float, lon1: float, lat2: float, lon2: float) -> float:
        """Calculate distance between two points in meters"""
        R = 6371000  # Earth radius in meters
        
        phi1 = math.radians(lat1)
        phi2 = math.radians(lat2)
        delta_phi = math.radians(lat2 - lat1)
        delta_lambda = math.radians(lon2 - lon1)
        
        a = math.sin(delta_phi/2)**2 + math.cos(phi1) * math.cos(phi2) * math.sin(delta_lambda/2)**2
        c = 2 * math.atan2(math.sqrt(a), math.sqrt(1-a))
        
        return R * c
    
    def _calculate_bearing(self, lat1: float, lon1: float, lat2: float, lon2: float) -> float:
        """Calculate bearing from point 1 to point 2"""
        lat1 = math.radians(lat1)
        lat2 = math.radians(lat2)
        diff_long = math.radians(lon2 - lon1)
        
        x = math.sin(diff_long) * math.cos(lat2)
        y = math.cos(lat1) * math.sin(lat2) - (math.sin(lat1) * math.cos(lat2) * math.cos(diff_long))
        
        initial_bearing = math.atan2(x, y)
        initial_bearing = math.degrees(initial_bearing)
        compass_bearing = (initial_bearing + 360) % 360
        
        return compass_bearing
        
    def get_telemetry(self) -> Dict:
        """Get current telemetry data"""
        return {
            "uav_id": self.uav_id,
            "latitude": self.latitude,
            "longitude": self.longitude,
            "altitude": round(self.altitude, 2),
            "heading": round(self.heading, 2),
            "speed": round(self.speed, 2),
            "battery": round(self.battery, 1),
            "satellites": self.satellites,
            "status": self.status,
            "timestamp": datetime.utcnow().isoformat()
        }


class TelemetrySimulator:
    """Manages multiple UAV simulators"""
    
    def __init__(self, api_base_url: str = "http://localhost:9000/api/v1"):
        self.api_base_url = api_base_url
        self.uavs: Dict[str, UAVSimulator] = {}
        self.running = False
        
    def add_uav(self, uav_id: str, name: str, lat: float, lon: float):
        """Add a UAV to simulate"""
        self.uavs[uav_id] = UAVSimulator(uav_id, name, lat, lon)
        
    def set_uav_mission(self, uav_id: str, waypoints: List[Dict]):
        """Set waypoints for a UAV"""
        if uav_id in self.uavs:
            self.uavs[uav_id].add_waypoints(waypoints)
            self.uavs[uav_id].is_flying = True
            self.uavs[uav_id].status = "active"
            
    async def send_telemetry(self, session: aiohttp.ClientSession, uav: UAVSimulator):
        """Send telemetry to backend"""
        try:
            telemetry = uav.get_telemetry()
            async with session.post(
                f"{self.api_base_url}/telemetry/telemetry/{uav.uav_id}",
                json=telemetry
            ) as resp:
                if resp.status != 200:
                    print(f"Failed to send telemetry for {uav.uav_id}: {resp.status}")
        except Exception as e:
            print(f"Error sending telemetry for {uav.uav_id}: {e}")
            
    async def run(self, update_interval: float = 1.0):
        """Run the simulator"""
        self.running = True
        
        async with aiohttp.ClientSession() as session:
            while self.running:
                tasks = []
                
                for uav in self.uavs.values():
                    # Update UAV position
                    uav.update_position(update_interval)
                    
                    # Send telemetry
                    tasks.append(self.send_telemetry(session, uav))
                    
                await asyncio.gather(*tasks, return_exceptions=True)
                await asyncio.sleep(update_interval)
                
    def stop(self):
        """Stop the simulator"""
        self.running = False


async def main():
    """Example usage"""
    simulator = TelemetrySimulator()
    
    # Add UAVs around Beijing
    simulator.add_uav("UAV-001", "侦察无人机-01", 39.9042, 116.4074)
    simulator.add_uav("UAV-002", "侦察无人机-02", 39.9142, 116.4174)
    simulator.add_uav("UAV-003", "巡检无人机-01", 39.8942, 116.3974)
    
    # Set missions (rectangular patterns)
    simulator.set_uav_mission("UAV-001", [
        {"lat": 39.9142, "lon": 116.4074, "alt": 150},
        {"lat": 39.9142, "lon": 116.4274, "alt": 150},
        {"lat": 39.8942, "lon": 116.4274, "alt": 150},
        {"lat": 39.8942, "lon": 116.4074, "alt": 150},
        {"lat": 39.9042, "lon": 116.4074, "alt": 100},  # Return to start
    ])
    
    print("🚁 Starting telemetry simulator...")
    print(f"Simulating {len(simulator.uavs)} UAVs")
    print("Press Ctrl+C to stop")
    
    try:
        await simulator.run(update_interval=1.0)
    except KeyboardInterrupt:
        print("\n🛑 Stopping simulator...")
        simulator.stop()


if __name__ == "__main__":
    asyncio.run(main())
