#!/bin/bash
# FalconMindSDK Code Formatting Script
# Usage: ./format-code.sh [check|format]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SDK_DIR="${SCRIPT_DIR}"
MODE="${1:-format}"

echo "FalconMindSDK Code Formatter"
echo "============================"
echo "Mode: ${MODE}"
echo ""

# Check if clang-format is installed
if ! command -v clang-format &> /dev/null; then
    echo "Error: clang-format is not installed"
    echo "Install with: sudo apt-get install clang-format"
    exit 1
fi

# Find all C++ files (excluding 3rd party and build directories)
find_cpp_files() {
    find "${SDK_DIR}" \
        -type f \
        \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) \
        ! -path "*/3rd/*" \
        ! -path "*/build/*" \
        ! -path "*/.git/*" \
        ! -path "*/install/*"
}

if [ "${MODE}" == "check" ]; then
    echo "Checking code formatting..."
    ERRORS=0
    
    while IFS= read -r file; do
        if ! clang-format --dry-run --Werror "${file}" > /dev/null 2>&1; then
            echo "  ❌ ${file}"
            ERRORS=$((ERRORS + 1))
        fi
    done <<(find_cpp_files)
    
    if [ ${ERRORS} -eq 0 ]; then
        echo ""
        echo "✅ All files are properly formatted!"
        exit 0
    else
        echo ""
        echo "❌ ${ERRORS} file(s) need formatting"
        echo "Run: ./format-code.sh format"
        exit 1
    fi
elif [ "${MODE}" == "format" ]; then
    echo "Formatting code..."
    COUNT=0
    
    while IFS= read -r file; do
        clang-format -i "${file}"
        echo "  ✓ ${file}"
        COUNT=$((COUNT + 1))
    done <<(find_cpp_files)
    
    echo ""
    echo "✅ Formatted ${COUNT} file(s)"
else
    echo "Usage: $0 [check|format]"
    echo ""
    echo "  check  - Check if code is properly formatted (CI mode)"
    echo "  format - Format all code files (development mode)"
    exit 1
fi
