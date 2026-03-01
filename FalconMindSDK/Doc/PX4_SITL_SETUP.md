# PX4 SITL Setup Guide for FalconMindSDK

## Overview

This guide explains how to set up PX4 Software-In-The-Loop (SITL) simulation for testing FalconMindSDK's MAVLink integration.

## Prerequisites

- Ubuntu 20.04 or later (recommended)
- 8GB+ RAM
- Git, cmake, and build tools

## Quick Setup

### 1. Install Dependencies

```bash
# Update package lists
sudo apt-get update

# Install base dependencies
sudo apt-get install -y \
    git \
    cmake \
    build-essential \
    python3-pip \
    python3-dev \
    ninja-build \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgtest-dev

# Install Python dependencies
pip3 install -U \
    empy==3.3.4 \
    pyros-genmsg \
    setuptools \
    packaging \
    numpy \
    toml \
    jinja2
```

### 2. Clone PX4-Autopilot

```bash
cd ~
git clone https://github.com/PX4/PX4-Autopilot.git --recursive
cd PX4-Autopilot
git checkout v1.14.0  # Use stable version
```

### 3. Build PX4 SITL

```bash
# For headless simulation (no GUI)
make px4_sitl_default none

# Or with Gazebo simulator
make px4_sitl_default gazebo
```

This will take 10-30 minutes depending on your system.

## Running PX4 SITL

### Basic Headless Mode

```bash
cd ~/PX4-Autopilot

# Start SITL with no simulator (just MAVLink)
make px4_sitl_default none

# Or start with Gazebo
make px4_sitl_default gazebo_iris
```

### Using Our Mock Server (For Testing)

If you don't want to install PX4, use our mock server:

```bash
cd FalconMindSDK/test
python3 mavlink_mock_server.py 14550
```

This simulates a PX4 vehicle with:
- Heartbeat messages (1Hz)
- Telemetry data (10Hz)
- Response to arm/disarm commands
- Takeoff/Land/RTL support

### Testing with FalconMindSDK

1. **Start the mock server** (or real PX4 SITL):
   ```bash
   python3 test/mavlink_mock_server.py
   ```

2. **Run the MAVLink test example**:
   ```bash
   cd examples/43_test_mavlink_client/x86/build
   ./test_mavlink_client_x86
   ```

3. **Or use the Easy API**:
   ```cpp
   auto result = FlightPipeline::create()
       .withConnection("udp://127.0.0.1:14550")
       .build();
       
   if (result) {
       auto flight = result.value();
       flight->connect();
       flight->arm();
       flight->takeoff(50.0f);
   }
   ```

## MAVLink Connection Details

### Default Ports

| Component | UDP Port | Protocol |
|-----------|----------|----------|
| PX4 SITL | 14550 | MAVLink (offboard) |
| PX4 SITL | 14540 | MAVLink (ground station) |
| QGroundControl | 14550 | MAVLink |
| MAVROS | 14540 | MAVLink |

### Connection Strings

```cpp
// UDP connection (SITL)
"udp://127.0.0.1:14550"

// Serial connection (real hardware)
"/dev/ttyUSB0"  // with baud rate 57600 or 921600

// UDP specific port
"udp://192.168.1.100:14550"
```

## Advanced Configuration

### Running Multiple Vehicles

```bash
# Terminal 1: First vehicle
PX4_SYS_AUTOSTART=4001 PX4_GZ_MODEL=iris ./build/px4_sitl_default/bin/px4 -i 1

# Terminal 2: Second vehicle  
PX4_SYS_AUTOSTART=4001 PX4_GZ_MODEL=iris ./build/px4_sitl_default/bin/px4 -i 2
```

Each vehicle will use different ports:
- Vehicle 1: 14550, 14540
- Vehicle 2: 14551, 14541

### Custom Simulation World

```bash
# Create custom world file
mkdir -p ~/PX4-Autopilot/Tools/simulation/gazebo/worlds/my_world

# Run with custom world
make px4_sitl_default gazebo_my_world
```

### Headless with JMAVSim

```bash
# Lighter weight simulator
make px4_sitl_default jmavsim
```

## Troubleshooting

### Port Already in Use

```bash
# Find process using port 14550
sudo lsof -i :14550

# Kill process
kill -9 <PID>
```

### Build Errors

```bash
# Clean build
make clean
make distclean

# Rebuild
make px4_sitl_default none
```

### Missing Dependencies

```bash
# Install all PX4 dependencies
bash ./Tools/setup/ubuntu.sh
```

## Testing Checklist

Before using with real hardware:

- [ ] Mock server responds to arm/disarm
- [ ] Takeoff command acknowledged
- [ ] Position updates received
- [ ] Mode changes work correctly
- [ ] Battery status reported
- [ ] GPS fix status correct
- [ ] Connection timeout handling works
- [ ] Auto-reconnect works

## Integration with CI/CD

For automated testing in CI:

```bash
#!/bin/bash
# ci_test_mavlink.sh

# Start mock server in background
python3 test/mavlink_mock_server.py &
SERVER_PID=$!

# Wait for server
sleep 2

# Run tests
./examples/43_test_mavlink_client/x86/build/test_mavlink_client_x86

# Cleanup
kill $SERVER_PID
```

## Next Steps

1. Test with mock server to verify client code
2. Install PX4 SITL for more realistic simulation
3. Connect to real flight controller (Pixhawk, etc.)
4. Test in hardware-in-the-loop (HITL) mode

## Resources

- [PX4 User Guide](https://docs.px4.io/main/en/)
- [PX4 SITL Documentation](https://docs.px4.io/main/en/simulation/)
- [MAVLink Protocol](https://mavlink.io/en/)
- [QGroundControl](https://docs.qgroundcontrol.com/master/en/)

## Support

For issues with:
- **Mock server**: File issue in FalconMindSDK repository
- **PX4 SITL**: Check [PX4 Discord](https://discord.gg/dronecode) or [GitHub](https://github.com/PX4/PX4-Autopilot)
- **MAVLink**: See [MAVLink documentation](https://mavlink.io/en/)
