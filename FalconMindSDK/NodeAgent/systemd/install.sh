#!/bin/bash
# FalconMind NodeAgent Installation Script
# Usage: sudo ./install.sh [install|uninstall|status|logs]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_DIR="/usr/local"
CONFIG_DIR="/etc/nodeagent"
DATA_DIR="/var/lib/nodeagent"
LOG_DIR="/var/log/nodeagent"
USER_NAME="nodeagent"
GROUP_NAME="nodeagent"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "Please run as root (use sudo)"
        exit 1
    fi
}

create_user() {
    if ! id "$USER_NAME" &> /dev/null; then
        print_info "Creating user: $USER_NAME"
        useradd -r -s /bin/false -d "$DATA_DIR" "$USER_NAME"
    else
        print_info "User $USER_NAME already exists"
    fi
}

create_directories() {
    print_info "Creating directories..."
    
    mkdir -p "$CONFIG_DIR"
    mkdir -p "$DATA_DIR"
    mkdir -p "$LOG_DIR"
    
    chown -R "$USER_NAME:$GROUP_NAME" "$DATA_DIR"
    chown -R "$USER_NAME:$GROUP_NAME" "$LOG_DIR"
    
    chmod 755 "$CONFIG_DIR"
    chmod 755 "$DATA_DIR"
    chmod 755 "$LOG_DIR"
}

install_binaries() {
    print_info "Installing binaries..."
    
    # Copy binaries from build directory
    if [ -f "$SCRIPT_DIR/../build/libnodeagent.so" ]; then
        cp "$SCRIPT_DIR/../build/libnodeagent.so" "$INSTALL_DIR/lib/"
        ldconfig
    fi
    
    if [ -f "$SCRIPT_DIR/../build/nodeagent_demo" ]; then
        cp "$SCRIPT_DIR/../build/nodeagent_demo" "$INSTALL_DIR/bin/"
        chmod +x "$INSTALL_DIR/bin/nodeagent_demo"
    fi
    
    if [ -f "$SCRIPT_DIR/../build/nodeagent_unit_tests" ]; then
        cp "$SCRIPT_DIR/../build/nodeagent_unit_tests" "$INSTALL_DIR/bin/"
        chmod +x "$INSTALL_DIR/bin/nodeagent_unit_tests"
    fi
}

install_systemd_service() {
    print_info "Installing systemd service..."
    
    cp "$SCRIPT_DIR/nodeagent.service" /etc/systemd/system/
    systemctl daemon-reload
    
    print_info "Enabling service..."
    systemctl enable nodeagent.service
}

install_config() {
    print_info "Installing default configuration..."
    
    if [ ! -f "$CONFIG_DIR/config.yaml" ]; then
        cat > "$CONFIG_DIR/config.yaml" << EOF
uav:
  id: "UAV_001"
  name: "Alpha"
  type: "quadcopter"

gcs:
  host: "192.168.1.100"
  port: 8080
  heartbeat_interval_ms: 1000
  heartbeat_timeout_ms: 5000

swarm:
  enabled: true
  swarm_id: "SWARM_001"
  discovery_interval_ms: 5000
  heartbeat_interval_ms: 1000
  heartbeat_timeout_ms: 5000

autonomy:
  enabled: true
  heartbeat_timeout_seconds: 10
  max_offline_duration_minutes: 30
  low_battery_threshold: 30
  critical_battery_threshold: 15

storage:
  db_path: "$DATA_DIR/offline.db"
  max_telemetry_buffer: 1000

logging:
  level: "INFO"
  output: "$LOG_DIR"
  max_file_size: 100
  max_files: 10
  format: "json"

metrics:
  enabled: true
  export_interval_seconds: 30
  prometheus_port: 9090
EOF
        chown "$USER_NAME:$GROUP_NAME" "$CONFIG_DIR/config.yaml"
        chmod 644 "$CONFIG_DIR/config.yaml"
    fi
}

install_deps() {
    print_info "Installing dependencies..."
    
    if command -v apt-get > /dev/null 2>&1; then
        apt-get update
        apt-get install -y sqlite3 libsqlite3-0 libssl3 zlib1g
    elif command -v yum > /dev/null 2>&1; then
        yum install -y sqlite sqlite-libs openssl zlib
    elif command -v pacman > /dev/null 2>&1; then
        pacman -S --noconfirm sqlite openssl zlib
    else
        print_warn "Unknown package manager, please install dependencies manually"
    fi
}

do_install() {
    print_info "Installing FalconMind NodeAgent..."
    
    check_root
    create_user
    create_directories
    install_deps
    install_binaries
    install_config
    install_systemd_service
    
    print_info "Installation complete!"
    print_info "Start with: sudo systemctl start nodeagent"
    print_info "View status: sudo systemctl status nodeagent"
    print_info "View logs: sudo journalctl -u nodeagent -f"
}

do_uninstall() {
    print_info "Uninstalling FalconMind NodeAgent..."
    
    check_root
    
    print_info "Stopping service..."
    systemctl stop nodeagent.service 2>/dev/null || true
    systemctl disable nodeagent.service 2>/dev/null || true
    
    print_info "Removing files..."
    rm -f /etc/systemd/system/nodeagent.service
    rm -f "$INSTALL_DIR/bin/nodeagent_demo"
    rm -f "$INSTALL_DIR/bin/nodeagent_unit_tests"
    rm -f "$INSTALL_DIR/lib/libnodeagent.so"
    
    systemctl daemon-reload
    
    print_warn "Configuration and data NOT removed:"
    print_warn "  - $CONFIG_DIR"
    print_warn "  - $DATA_DIR"
    print_warn "  - $LOG_DIR"
    print_warn "Remove manually if desired"
    
    print_info "Uninstallation complete!"
}

do_status() {
    if systemctl is-active --quiet nodeagent.service; then
        print_info "NodeAgent is running"
        systemctl status nodeagent.service --no-pager
    else
        print_error "NodeAgent is not running"
        systemctl status nodeagent.service --no-pager || true
    fi
}

do_logs() {
    journalctl -u nodeagent.service -f
}

# Main
case "${1:-install}" in
    install)
        do_install
        ;;
    uninstall)
        do_uninstall
        ;;
    status)
        do_status
        ;;
    logs)
        do_logs
        ;;
    *)
        echo "Usage: $0 [install|uninstall|status|logs]"
        exit 1
        ;;
esac
