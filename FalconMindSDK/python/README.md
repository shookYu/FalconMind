# FalconMind SDK Python Bindings

This directory contains Python bindings for FalconMind SDK, allowing Builder Flow to call SDK functionality via Python API.

## Structure

```
python/
├── CMakeLists.txt           # Python bindings CMake configuration (included from main CMakeLists.txt)
├── falconmind_sdk.cpp       # Core SDK Python bindings (Pipeline, FlowExecutor, Nodes)
├── denied_env_nodes.cpp     # Denied environment nodes (GPSDefender, IBVSController, etc.)
├── test_gps_defender.py     # Unit tests for GPS Defender
├── test_ibvs_controller.py  # Unit tests for IBVS Controller
├── tools/                   # Utility scripts
│   ├── gps_defender_demo.py     # GPS Defender demo
│   ├── ibvs_controller_demo.py  # IBVS Controller demo
│   └── flow_executor_cli.py     # Flow execution CLI
└── README.md                # This file
```

## Building

Python bindings are built automatically when `FALCONMINDSDK_BUILD_PYTHON` is ON (default):

```bash
cd FalconMindSDK/build
cmake .. -DFALCONMINDSDK_BUILD_PYTHON=ON
make -j4
```

The compiled modules will be in:
- `FalconMindSDK/python/falconmind_sdk.so` (or `.pyd` on Windows)
- `FalconMindSDK/python/denied_env_nodes.so`

## Usage

### Importing Modules

```python
# Core SDK
from falconmind_sdk import FlowExecutor, Pipeline, NodeFactory

# Denied environment nodes
from denied_env_nodes.navigation import GPSDefender, create_gps_defender
from denied_env_nodes.control import IBVSController, create_ibvs_controller
```

### Example: GPS Defender

```python
from denied_env_nodes.navigation import create_gps_defender, GNSSMeasurement

# Create defender
defender = create_gps_defender()
defender.initialize()

# Process GNSS data
gnss = GNSSMeasurement()
gnss.latitude = 39.9042
gnss.longitude = 116.4074
gnss.velocity_north = 5.0
gnss.velocity_east = 3.0

# Check for spoofing
defender.process_gnss(gnss)
if defender.get_alert_level().value > 0:
    print("Spoofing detected!")
```

### Example: IBVS Controller

```python
from denied_env_nodes.control import create_ibvs_controller, ImageSpaceTarget

# Create controller
controller = create_ibvs_controller()
controller.initialize()

# Target in image space (normalized coordinates)
target = ImageSpaceTarget()
target.u = 10.0  # pixels from center
target.v = 5.0
target.area_ratio = 0.1  # target area / image area

# Compute control command
current_distance = 15.0
current_height = 10.0
cmd = controller.compute_control(target, current_distance, current_height)

print(f"Velocity command: Vx={cmd.vx}, Vy={cmd.vy}, Vz={cmd.vz}")
```

### Example: Flow Executor

```python
from falconmind_sdk import FlowExecutor

# Load and execute flow
executor = FlowExecutor()
executor.load_flow_from_file("/path/to/flow.json")
executor.start()

# Monitor execution
while executor.is_running():
    time.sleep(1)

executor.stop()
```

## Testing

Run unit tests:

```bash
cd FalconMindSDK/python
python test_gps_defender.py
python test_ibvs_controller.py
```

Or with CTest:

```bash
cd FalconMindSDK/build
ctest -R python
```

## Tools

### GPS Defender Demo

```bash
python tools/gps_defender_demo.py --strict
```

### IBVS Controller Demo

```bash
python tools/ibvs_controller_demo.py --conservative
```

### Flow Executor CLI

```bash
# Validate flow JSON
python tools/flow_executor_cli.py flow.json --validate-only

# Execute flow
python tools/flow_executor_cli.py flow.json

# Dry run (validate without executing)
python tools/flow_executor_cli.py flow.json --dry-run
```

## API Reference

### falconmind_sdk Module

**Classes:**
- `FlowExecutor`: Main flow execution engine
- `Pipeline`: Pipeline management
- `NodeFactory`: Node creation factory
- `PipelineConfig`, `PipelineState`, `LinkInfo`: Configuration and state

**Mission Classes:**
- `SearchPathPlannerNode`: Generate search waypoints
- `EventReporterNode`: Report mission events
- `GeoPoint`, `SearchArea`, `SearchParams`: Mission data structures
- `SearchPattern`: Enum for search patterns (LAWN_MOWER, SPIRAL, etc.)

### denied_env_nodes Module

**navigation Submodule:**
- `GPSDefender`: GPS anti-spoofing detection
- `GPSDefenderConfig`: Configuration for GPS Defender
- `GNSSMeasurement`, `IMUMeasurement`: Sensor data structures
- `VisualPosition`: Visual odometry position
- `SpoofingReport`, `SpoofingAlertLevel`: Spoofing detection results
- `create_gps_defender()`, `create_strict_gps_defender()`: Factory functions

**control Submodule:**
- `IBVSController`: Image-based visual servoing controller
- `IBVSConfig`: Controller configuration
- `CameraParameters`: Camera intrinsics
- `ImageSpaceTarget`: Target in image coordinates
- `VelocityCommand`: Output velocity command
- `TrackingQuality`: Tracking quality metrics
- `create_ibvs_controller()`, `create_conservative_ibvs_controller()`, `create_aggressive_ibvs_controller()`: Factory functions

**perception Submodule:**
- `MonocularDistanceEstimator`: Distance estimation from monocular camera
- `ObjectDimensions`, `DistanceEstimate`: Data structures

**high_level Submodule:**
- `DeniedEnvMission`: High-level denied environment mission
- `DeniedEnvMissionConfig`, `DeniedEnvMissionCallbacks`: Mission configuration
- `SearchArea`, `TargetSelection`, `TrackingParameters`: Mission parameters

## Notes

- Python bindings use pybind11 for C++/Python interoperability
- All SDK memory management is handled automatically
- Callbacks can be registered from Python (see `DeniedEnvMissionCallbacks`)
- The bindings support both sync and async operations

## Troubleshooting

### Import Error

If you get `ImportError: cannot import name 'denied_env_nodes'`:

1. Make sure the bindings are built:
   ```bash
   cd FalconMindSDK/build && make -j4
   ```

2. Check the `.so` files exist:
   ```bash
   ls FalconMindSDK/python/*.so
   ```

3. Make sure you're importing from the correct directory

### Segmentation Fault

- Ensure you're not mixing different SDK versions
- Check that all dependencies are properly linked
- Run with `PYTHONFAULTHANDLER=1` for better error messages

## License

Apache License 2.0 - See LICENSE file in project root
