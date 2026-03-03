#!/bin/bash
# Run Builder Backend Tests Script

set -e

echo "================================"
echo "FalconMindBuilder Backend Tests"
echo "================================"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if in backend directory
if [ ! -f "app/main.py" ]; then
    echo -e "${RED}Error: Please run from backend directory${NC}"
    exit 1
fi

# Install test dependencies if needed
echo -e "${YELLOW}Installing test dependencies...${NC}"
pip install -q pytest pytest-cov pytest-benchmark pytest-asyncio

# Run unit tests
echo -e "${YELLOW}Running unit tests...${NC}"
pytest tests/test_services.py -v --tb=short -m "not slow and not benchmark"

# Run API tests
echo -e "${YELLOW}Running API tests...${NC}"
pytest tests/test_api.py -v --tb=short -m "not slow and not benchmark"

# Run model tests
echo -e "${YELLOW}Running model tests...${NC}"
pytest tests/test_models.py -v --tb=short -m "not slow and not benchmark"

# Run all tests with coverage
echo -e "${YELLOW}Running all tests with coverage...${NC}"
pytest tests/ -v --tb=short \
    --cov=app \
    --cov-report=term-missing \
    --cov-report=html:htmlcov \
    --cov-fail-under=80

# Check coverage
echo -e "${YELLOW}Generating coverage report...${NC}"
python -m coverage report --fail-under=80

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}All tests passed!${NC}"
echo -e "${GREEN}================================${NC}"
echo ""
echo "Coverage report available at:"
echo "  - Terminal (above)"
echo "  - htmlcov/index.html"
echo "  - coverage.xml"
