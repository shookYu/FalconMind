#!/bin/bash
# FalconMindSDK Coverage Report Script
# Usage: ./coverage-report.sh [html|xml|summary]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build-coverage"
REPORT_DIR="${SCRIPT_DIR}/coverage-report"
MODE="${1:-html}"

echo "FalconMindSDK Coverage Report"
echo "============================="
echo "Mode: ${MODE}"
echo ""

# Check dependencies
if ! command -v gcov &> /dev/null; then
    echo "Error: gcov is not installed"
    echo "Install with: sudo apt-get install gcc"
    exit 1
fi

if ! command -v lcov &> /dev/null; then
    echo "Error: lcov is not installed"
    echo "Install with: sudo apt-get install lcov"
    exit 1
fi

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure with coverage
echo "Configuring with coverage enabled..."
cmake -DCMAKE_BUILD_TYPE=Debug \
      -DFALCONMINDSDK_ENABLE_COVERAGE=ON \
      -DFALCONMINDSDK_BUILD_TESTS=ON \
      ..

# Build
echo "Building..."
make -j$(nproc)

# Run tests
echo "Running tests..."
ctest --output-on-failure

# Generate coverage report
echo "Generating coverage report..."

# Capture coverage data
lcov --capture --directory . --output-file coverage.info \
     --rc lcov_branch_coverage=1 2>&1 | grep -v "ignoring data for external file"

# Remove 3rd party and test code from report
lcov --remove coverage.info \
     '*/3rd/*' \
     '*/tests/*' \
     '*/build*/*' \
     '/usr/*' \
     --output-file coverage-filtered.info \
     --rc lcov_branch_coverage=1 2>&1 | grep -v "ignoring data for external file"

# Generate report based on mode
case "${MODE}" in
    html)
        mkdir -p "${REPORT_DIR}"
        genhtml coverage-filtered.info \
            --output-directory "${REPORT_DIR}" \
            --branch-coverage \
            --highlight \
            --legend \
            --title "FalconMindSDK Coverage Report"
        echo ""
        echo "✅ HTML report generated: ${REPORT_DIR}/index.html"
        echo "Open with: xdg-open ${REPORT_DIR}/index.html"
        ;;
    xml)
        if command -v gcovr &> /dev/null; then
            gcovr --xml --output coverage.xml --root .. .
            echo "✅ XML report generated: ${BUILD_DIR}/coverage.xml"
        else
            echo "Error: gcovr not installed. Install with: pip install gcovr"
            exit 1
        fi
        ;;
    summary)
        echo ""
        echo "Coverage Summary:"
        echo "----------------"
        lcov --summary coverage-filtered.info --rc lcov_branch_coverage=1 2>&1 | tail -20
        ;;
    *)
        echo "Usage: $0 [html|xml|summary]"
        echo ""
        echo "  html    - Generate HTML report (default)"
        echo "  xml     - Generate XML report (for CI integration)"
        echo "  summary - Print coverage summary to console"
        exit 1
        ;;
esac

# Extract overall percentage
if [ "${MODE}" != "xml" ]; then
    echo ""
    TOTAL_LINES=$(lcov --summary coverage-filtered.info 2>&1 | grep "lines" | grep -oP '\d+\.?\d*%' | head -1)
    TOTAL_FUNCS=$(lcov --summary coverage-filtered.info 2>&1 | grep "functions" | grep -oP '\d+\.?\d*%' | head -1)
    echo "Overall Coverage:"
    echo "  Lines:     ${TOTAL_LINES}"
    echo "  Functions: ${TOTAL_FUNCS}"
fi
