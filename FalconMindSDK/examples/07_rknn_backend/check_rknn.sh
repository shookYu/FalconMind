#!/bin/bash
# Check RKNN Toolkit2 Installation

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "Checking RKNN Toolkit2 installation..."
echo ""

# Check Python module
echo -n "Python module 'rknn': "
if python3 -c "from rknn.api import RKNN" 2>/dev/null; then
    echo -e "${GREEN}✓ Installed${NC}"
    
    # Get version
    RKNN_VERSION=$(python3 -c "from rknn.api import RKNN; print(RKNN().get_sdk_version() if hasattr(RKNN(), 'get_sdk_version') else 'unknown')" 2>/dev/null || echo "unknown")
    echo "  Version: $RKNN_VERSION"
else
    echo -e "${RED}✗ Not installed${NC}"
    echo ""
    echo "To install RKNN Toolkit2:"
    echo "  pip install rknn-toolkit2"
    echo ""
    echo "Or download from:"
    echo "  https://github.com/rockchip-linux/rknn-toolkit2/releases"
    exit 1
fi

# Check C/C++ headers
echo ""
echo -n "C/C++ header 'rknn_api.h': "
if find /usr /opt -name "rknn_api.h" 2>/dev/null | grep -q .; then
    RKNN_HEADER=$(find /usr /opt -name "rknn_api.h" 2>/dev/null | head -1)
    echo -e "${GREEN}✓ Found${NC}"
    echo "  Location: $RKNN_HEADER"
else
    echo -e "${YELLOW}⚠ Not found${NC}"
    echo "  Note: C/C++ headers may be in Python package directory"
    echo "  RKNN Toolkit2 Python package includes runtime library"
fi

# Check library
echo ""
echo -n "Runtime library: "
if find /usr /opt -name "librknn_api.so" -o -name "librknn_api.a" 2>/dev/null | grep -q .; then
    RKNN_LIB=$(find /usr /opt -name "librknn_api.so" -o -name "librknn_api.a" 2>/dev/null | head -1)
    echo -e "${GREEN}✓ Found${NC}"
    echo "  Location: $RKNN_LIB"
else
    echo -e "${YELLOW}⚠ Not found in standard locations${NC}"
    echo "  The library may be in Python site-packages"
fi

# Check environment variable
echo ""
echo -n "Environment RKNN_TOOLKIT2_PATH: "
if [ -n "$RKNN_TOOLKIT2_PATH" ]; then
    echo -e "${GREEN}✓ Set${NC}"
    echo "  Value: $RKNN_TOOLKIT2_PATH"
else
    echo -e "${YELLOW}⚠ Not set (optional)${NC}"
    echo "  Set this if RKNN is installed in non-standard location"
fi

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}RKNN Toolkit2 check complete!${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""
echo "You can now build the example:"
echo "  cd rk3588/build"
echo "  cmake .."
echo "  make -j4"
