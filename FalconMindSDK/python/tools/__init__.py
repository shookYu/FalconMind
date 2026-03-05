"""
FalconMind SDK Python Tools

This package contains utility scripts for working with FalconMind SDK Python bindings.

Available Tools:
    - gps_defender_demo.py: Demo GPS anti-spoofing detection
    - ibvs_controller_demo.py: Demo image-based visual servoing control
    - flow_executor_cli.py: CLI tool for executing Flow JSON files

Usage:
    # Run GPS Defender demo
    python -m tools.gps_defender_demo
    
    # Run IBVS Controller demo
    python -m tools.ibvs_controller_demo --conservative
    
    # Execute a flow
    python -m tools.flow_executor_cli /path/to/flow.json
"""

__version__ = "1.0.0"
