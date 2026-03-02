#!/bin/bash
#
# build-sdk.sh - Build script for FalconMindSDK
#
# Usage: ./build-sdk.sh [options]
#   Options:
#     --offline       Enable offline build mode
#     --arm64         Cross-compile for ARM64
#     --no-tests      Disable test builds
#     --no-python     Disable Python bindings
#     --clean         Clean build directory first
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# Parse arguments
OFFLINE_MODE=OFF
ARM64_CROSS=OFF
BUILD_TESTS=ON
BUILD_PYTHON=ON
CLEAN_BUILD=OFF

while [[ $# -gt 0 ]]; do
  case $1 in
    --offline)
      OFFLINE_MODE=ON
      shift
      ;;
    --arm64)
      ARM64_CROSS=ON
      shift
      ;;
    --no-tests)
      BUILD_TESTS=OFF
      shift
      ;;
    --no-python)
      BUILD_PYTHON=OFF
      shift
      ;;
    --clean)
      CLEAN_BUILD=ON
      shift
      ;;
    --help)
      echo "Usage: $0 [options]"
      echo "Options:"
      echo "  --offline       Enable offline build mode"
      echo "  --arm64         Cross-compile for ARM64"
      echo "  --no-tests      Disable test builds"
      echo "  --no-python     Disable Python bindings"
      echo "  --clean         Clean build directory first"
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done

echo "=== FalconMindSDK Build Script ==="
echo ""

# Clean build directory if requested
if [ "$CLEAN_BUILD" = "ON" ]; then
  echo "Cleaning build directory..."
  rm -rf "${BUILD_DIR}"
fi

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure CMake
echo "Configuring CMake..."
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"
CMAKE_ARGS="${CMAKE_ARGS} -DFALCONMINDSDK_BUILD_TESTS=${BUILD_TESTS}"
CMAKE_ARGS="${CMAKE_ARGS} -DFALCONMINDSDK_BUILD_PYTHON=${BUILD_PYTHON}"
CMAKE_ARGS="${CMAKE_ARGS} -DFALCONMINDSDK_OFFLINE_BUILD=${OFFLINE_MODE}"

if [ "$ARM64_CROSS" = "ON" ]; then
  CMAKE_ARGS="${CMAKE_ARGS} -DCMAKE_TOOLCHAIN_FILE=../toolchain/aarch64-linux-gnu.cmake"
  echo "  Mode: ARM64 Cross-compilation"
else
  echo "  Mode: x86 Native"
fi

if [ "$OFFLINE_MODE" = "ON" ]; then
  echo "  Offline mode: ENABLED"
  
  # Check if offline dependencies are prepared
  if [ ! -f "${SCRIPT_DIR}/3rd/.deps_prepared" ]; then
    echo ""
    echo "WARNING: Offline dependencies not prepared!"
    echo "Run: ./prepare-offline-deps.sh"
    echo ""
    read -p "Continue anyway? (y/N) " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
      exit 1
    fi
  fi
fi

echo "  Tests: ${BUILD_TESTS}"
echo "  Python: ${BUILD_PYTHON}"
echo ""

# Run CMake
cmake .. ${CMAKE_ARGS}

# Build
echo ""
echo "Building..."
make -j$(nproc)

echo ""
echo "=== Build Complete ==="
echo ""
echo "To run tests:"
echo "  cd ${BUILD_DIR} && ctest --output-on-failure"
echo ""
echo "To install:"
echo "  cd ${BUILD_DIR} && make install"
