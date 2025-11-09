#!/bin/bash

# API Testing Script for Rubik's Cube Solver
# Usage: ./test_api.sh [port]

PORT=${1:-8080}
BASE_URL="http://localhost:$PORT"

echo "=========================================="
echo "Testing Rubik's Cube Solver API"
echo "Base URL: $BASE_URL"
echo "=========================================="
echo ""

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test function
test_endpoint() {
    local name=$1
    local method=$2
    local endpoint=$3
    local data=$4
    
    echo -e "${YELLOW}Testing: $name${NC}"
    echo "  $method $endpoint"
    
    if [ -z "$data" ]; then
        response=$(curl -s -w "\n%{http_code}" -X $method "$BASE_URL$endpoint" 2>&1)
    else
        response=$(curl -s -w "\n%{http_code}" -X $method "$BASE_URL$endpoint" \
            -H "Content-Type: application/json" \
            -d "$data" 2>&1)
    fi
    
    http_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n-1)
    
    if [ "$http_code" -eq 200 ] || [ "$http_code" -eq 201 ]; then
        echo -e "  ${GREEN}✓ Success (HTTP $http_code)${NC}"
        echo "  Response: $(echo $body | head -c 100)..."
    else
        echo -e "  ${RED}✗ Failed (HTTP $http_code)${NC}"
        echo "  Response: $body"
    fi
    echo ""
}

# Check if server is running
echo "Checking if server is running..."
if ! curl -s "$BASE_URL/status" > /dev/null 2>&1; then
    echo -e "${RED}✗ Server is not running on port $PORT${NC}"
    echo "Start the server with: cd build && ./rubiks_solver $PORT"
    exit 1
fi
echo -e "${GREEN}✓ Server is running${NC}"
echo ""

# Run tests
test_endpoint "Get Status" "GET" "/status"
test_endpoint "Get Cube State" "GET" "/cube"
test_endpoint "Reset Cube" "POST" "/cube/reset"
test_endpoint "Scramble Cube (10 moves)" "POST" "/cube/scramble" '{"moves": 10}'
test_endpoint "Apply Move (R)" "POST" "/cube/move" '{"move": "R"}'
test_endpoint "Apply Move (U)" "POST" "/cube/move" '{"move": "U"}'
test_endpoint "Solve Cube" "POST" "/cube/solve" '{"maxDepth": 15}'

echo "=========================================="
echo "Testing complete!"
echo "=========================================="
echo ""
echo "Manual testing URLs:"
echo "  Status:     curl $BASE_URL/status"
echo "  Cube state: curl $BASE_URL/cube"
echo "  Scramble:   curl -X POST $BASE_URL/cube/scramble -H 'Content-Type: application/json' -d '{\"moves\": 20}'"
echo "  Solve:      curl -X POST $BASE_URL/cube/solve -H 'Content-Type: application/json' -d '{\"maxDepth\": 20}'"