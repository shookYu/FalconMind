#!/usr/bin/env python3
"""
flow_executor_cli.py

Flow Executor 命令行工具
用于从 Builder JSON 文件执行 Flow

用法:
    python flow_executor_cli.py <flow_json_file> [--dry-run]
    
示例:
    python flow_executor_cli.py /path/to/flow.json
    python flow_executor_cli.py /path/to/flow.json --dry-run
"""

import argparse
import json
import sys
import os

# Add parent directory to path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

try:
    from falconmind_sdk import FlowExecutor, PipelineState
    SDK_AVAILABLE = True
except ImportError as e:
    print(f"Warning: falconmind_sdk module not available: {e}")
    SDK_AVAILABLE = False


def validate_flow_json(flow_data):
    """Validate flow JSON structure"""
    required_fields = ['id', 'nodes', 'edges']
    
    for field in required_fields:
        if field not in flow_data:
            return False, f"Missing required field: {field}"
    
    if not isinstance(flow_data['nodes'], list):
        return False, "'nodes' must be a list"
    
    if not isinstance(flow_data['edges'], list):
        return False, "'edges' must be a list"
    
    return True, "Valid"


def print_flow_info(flow_data):
    """Print flow information"""
    print(f"Flow ID: {flow_data.get('id', 'N/A')}")
    print(f"Flow Name: {flow_data.get('name', 'N/A')}")
    print(f"Description: {flow_data.get('description', 'N/A')}")
    print(f"Nodes: {len(flow_data.get('nodes', []))}")
    print(f"Edges: {len(flow_data.get('edges', []))}")
    print()
    
    print("Nodes:")
    for node in flow_data.get('nodes', []):
        node_id = node.get('id', 'N/A')
        node_type = node.get('type', 'N/A')
        node_label = node.get('data', {}).get('label', 'N/A')
        print(f"  - {node_id}: {node_type} ({node_label})")
    
    print()
    print("Connections:")
    for edge in flow_data.get('edges', []):
        source = edge.get('source', 'N/A')
        target = edge.get('target', 'N/A')
        print(f"  {source} → {target}")


def execute_flow(flow_file, dry_run=False):
    """Execute flow from JSON file"""
    # Load flow JSON
    try:
        with open(flow_file, 'r') as f:
            flow_data = json.load(f)
    except FileNotFoundError:
        print(f"Error: File not found: {flow_file}")
        return False
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON: {e}")
        return False
    
    # Validate
    valid, message = validate_flow_json(flow_data)
    if not valid:
        print(f"Error: Invalid flow JSON: {message}")
        return False
    
    # Print flow info
    print("=" * 60)
    print("Flow Information")
    print("=" * 60)
    print_flow_info(flow_data)
    print()
    
    if dry_run:
        print("Dry run mode - not executing flow")
        return True
    
    if not SDK_AVAILABLE:
        print("Error: SDK not available. Cannot execute flow.")
        return False
    
    # Execute flow
    print("=" * 60)
    print("Flow Execution")
    print("=" * 60)
    
    try:
        executor = FlowExecutor()
        
        print(f"Loading flow from: {flow_file}")
        success = executor.load_flow_from_file(flow_file)
        
        if not success:
            print("Error: Failed to load flow")
            return False
        
        print(f"Flow loaded successfully")
        print(f"Flow ID: {executor.get_flow_id()}")
        print(f"Flow Name: {executor.get_flow_name()}")
        print()
        
        # Start execution
        print("Starting flow execution...")
        executor.start()
        
        # Get pipeline state
        pipeline = executor.get_pipeline()
        if pipeline:
            state = pipeline.state()
            print(f"Pipeline state: {state}")
        
        print("Flow execution started successfully!")
        print()
        
        # In a real scenario, you would wait for completion or user interrupt
        print("Press Ctrl+C to stop...")
        try:
            import time
            while executor.is_running():
                time.sleep(1)
        except KeyboardInterrupt:
            print("\nStopping flow...")
            executor.stop()
        
        print("Flow execution stopped")
        return True
        
    except Exception as e:
        print(f"Error during execution: {e}")
        return False


def main():
    parser = argparse.ArgumentParser(
        description='Flow Executor CLI - Execute FalconMind Flow from JSON'
    )
    parser.add_argument('flow_file', help='Path to flow JSON file')
    parser.add_argument('--dry-run', action='store_true',
                        help='Validate flow without executing')
    parser.add_argument('--validate-only', action='store_true',
                        help='Only validate the flow JSON')
    args = parser.parse_args()
    
    if args.validate_only:
        # Just validate
        try:
            with open(args.flow_file, 'r') as f:
                flow_data = json.load(f)
            valid, message = validate_flow_json(flow_data)
            if valid:
                print(f"✓ Valid flow JSON: {args.flow_file}")
                print_flow_info(flow_data)
                sys.exit(0)
            else:
                print(f"✗ Invalid flow JSON: {message}")
                sys.exit(1)
        except Exception as e:
            print(f"✗ Error: {e}")
            sys.exit(1)
    
    # Execute or dry run
    success = execute_flow(args.flow_file, dry_run=args.dry_run)
    sys.exit(0 if success else 1)


if __name__ == '__main__':
    main()
