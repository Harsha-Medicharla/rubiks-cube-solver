#!/bin/bash

# Rubik's Cube Solver - Quick Setup Script
# Usage: ./setup.sh [port]

set -e

echo "=========================================="
echo "Rubik's Cube Solver - Setup"
echo "=========================================="
echo ""

# Check for required tools
echo "Checking dependencies..."

if ! command -v cmake &> /dev/null; then
    echo "❌ CMake not found. Please install CMake 3.16 or higher."
    echo "   Ubuntu/Debian: sudo apt install cmake"
    echo "   macOS: brew install cmake"
    exit 1
fi

if ! command -v g++ &> /dev/null && ! command -v clang++ &> /dev/null; then
    echo "❌ C++ compiler not found. Please install g++ or clang++."
    echo "   Ubuntu/Debian: sudo apt install build-essential"
    echo "   macOS: xcode-select --install"
    exit 1
fi

echo "✓ CMake found: $(cmake --version | head -n1)"
if command -v g++ &> /dev/null; then
    echo "✓ g++ found: $(g++ --version | head -n1)"
elif command -v clang++ &> /dev/null; then
    echo "✓ clang++ found: $(clang++ --version | head -n1)"
fi
echo ""

# Create build directory
echo "Creating build directory..."
if [ -d "build" ]; then
    echo "⚠ Build directory exists. Cleaning..."
    rm -rf build
fi
mkdir build
cd build
echo "✓ Build directory created"
echo ""

# Configure with CMake
echo "Configuring project with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..
echo "✓ Configuration complete"
echo ""

# Build
echo "Building project..."
CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)
make -j${CORES}
echo "✓ Build complete"
echo ""

# Run tests
echo "Running tests..."
if [ -f "./test_solver" ]; then
    ./test_solver
    echo ""
else
    echo "⚠ Tests not found (this is OK if BUILD_TESTS=OFF)"
    echo ""
fi

echo "=========================================="
echo "Setup Complete! 🎉"
echo "=========================================="
echo ""
echo "To start the server:"
echo "  cd build"
echo "  ./rubiks_solver [port]"
echo ""
echo "Default port: 8080"
echo ""
echo "API will be available at:"
echo "  http://localhost:8080"
echo ""
echo "Example: ./rubiks_solver 3000"
echo "=========================================="