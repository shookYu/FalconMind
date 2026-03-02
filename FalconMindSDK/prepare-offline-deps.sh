#!/bin/bash
#
# prepare-offline-deps.sh - Prepare dependencies for offline build
# 
# Usage: ./prepare-offline-deps.sh [target_dir]
# Default target: 3rd/
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TARGET_DIR="${1:-${SCRIPT_DIR}/3rd}"

echo "=== FalconMindSDK Offline Dependencies Preparation ==="
echo "Target directory: ${TARGET_DIR}"
echo ""

mkdir -p "${TARGET_DIR}"
cd "${TARGET_DIR}"

# Function to download and extract
download_dep() {
    local name="$1"
    local url="$2"
    local output="$3"
    
    echo "Downloading ${name}..."
    if command -v curl &> /dev/null; then
        curl -L -o "${output}" "${url}"
    elif command -v wget &> /dev/null; then
        wget -O "${output}" "${url}"
    else
        echo "Error: curl or wget required"
        exit 1
    fi
}

# 1. nlohmann/json
echo "[1/4] Preparing nlohmann/json..."
if [ ! -d "json" ]; then
    download_dep "nlohmann/json" \
        "https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz" \
        "json.tar.xz"
    tar -xf json.tar.xz
    rm json.tar.xz
    echo "  ✓ nlohmann/json ready"
else
    echo "  ✓ nlohmann/json already exists (skipping)"
fi

# 2. cpp-httplib
echo "[2/4] Preparing cpp-httplib..."
if [ ! -f "cpp-httplib/httplib.h" ]; then
    mkdir -p cpp-httplib
    download_dep "cpp-httplib" \
        "https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.14.3/httplib.h" \
        "cpp-httplib/httplib.h"
    echo "  ✓ cpp-httplib ready"
else
    echo "  ✓ cpp-httplib already exists (skipping)"
fi

# 3. pybind11 (optional, for Python bindings)
echo "[3/4] Preparing pybind11..."
if [ ! -d "pybind11" ]; then
    download_dep "pybind11" \
        "https://github.com/pybind/pybind11/archive/refs/tags/v2.11.1.tar.gz" \
        "pybind11.tar.gz"
    tar -xzf pybind11.tar.gz
    mv pybind11-2.11.1 pybind11
    rm pybind11.tar.gz
    echo "  ✓ pybind11 ready"
else
    echo "  ✓ pybind11 already exists (skipping)"
fi

# 4. Create dependency marker
echo "[4/4] Creating dependency marker..."
cat > "${TARGET_DIR}/.deps_prepared" << EOF
FalconMindSDK Offline Dependencies
Generated: $(date -Iseconds)

Components:
- nlohmann/json v3.11.3
- cpp-httplib v0.14.3
- pybind11 v2.11.1

Usage:
  cmake .. -DFALCONMINDSDK_OFFLINE_BUILD=ON
EOF

echo "  ✓ Marker file created"
echo ""
echo "=== Preparation Complete ==="
echo ""
echo "To build in offline mode:"
echo "  cd build"
echo "  cmake .. -DFALCONMINDSDK_OFFLINE_BUILD=ON"
echo "  make -j4"
