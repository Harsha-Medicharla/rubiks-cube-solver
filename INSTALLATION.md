# Complete Installation & Running Guide

This guide will help you install, build, and run the Rubik's Cube Solver backend step-by-step.

## 📋 Table of Contents
1. [Prerequisites](#prerequisites)
2. [Installation](#installation)
3. [Building](#building)
4. [Running](#running)
5. [Testing](#testing)
6. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### System Requirements
- Operating System: Linux, macOS, or Windows (with WSL)
- RAM: 2GB minimum
- Disk Space: 100MB
- Internet connection (for initial setup only)

### Required Software

#### Linux (Ubuntu/Debian)
```bash
# Update package list
sudo apt update

# Install build tools
sudo apt install -y build-essential cmake git

# Verify installations
gcc --version    # Should be 7.0+
cmake --version  # Should be 3.16+
```

#### macOS
```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install CMake
brew install cmake

# Verify installations
clang++ --version  # Should support C++17
cmake --version    # Should be 3.16+
```

#### Windows (WSL)
```bash
# First, install WSL from PowerShell (as Administrator):
# wsl --install

# Then in WSL Ubuntu terminal:
sudo apt update
sudo apt install -y build-essential cmake git
```

---

## Installation

### Step 1: Get the Code

#### Option A: Clone from Git
```bash
# Clone the repository
git clone <your-repository-url>
cd rubiks-cube-solver
```

#### Option B: Download and Extract
```bash
# If you have a zip file
unzip rubiks-cube-solver.zip
cd rubiks-cube-solver
```

### Step 2: Verify Files
```bash
# Check that all files are present
ls -la

# You should see:
# CMakeLists.txt, Makefile, README.md, setup.sh
# include/, src/, tests/
```

### Step 3: Make Scripts Executable
```bash
chmod +x setup.sh test_api.sh
```

---

## Building

### Method 1: Automated Setup (Recommended)
```bash
./setup.sh
```

**What it does:**
1. Checks for required dependencies
2. Creates build directory
3. Runs CMake configuration
4. Compiles the project
5. Runs tests automatically
6. Provides next steps

**Expected output:**
```
==========================================
Rubik's Cube Solver - Setup
==========================================

Checking dependencies...
✓ CMake found: cmake version 3.22.1
✓ g++ found: g++ (Ubuntu 11.3.0-1ubuntu1) 11.3.0

Creating build directory...
✓ Build directory created

Configuring project with CMake...
✓ Configuration complete

Building project...
✓ Build complete

Running tests...
========================================
Running Rubik's Cube Solver Tests
========================================
...
All tests passed! ✓

==========================================
Setup Complete! 🎉
==========================================
```

### Method 2: Using Makefile
```bash
# Build in release mode
make build

# Or build in debug mode
make debug

# Or build with optimizations
make release
```

### Method 3: Manual CMake
```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build (use all CPU cores)
make -j$(nproc)

# Or on macOS:
make -j$(sysctl -n hw.ncpu)
```

### Build Variants

#### Debug Build (for development)
```bash
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

#### Optimized Release Build (for production)
```bash
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
make
```

#### Build Without Tests
```bash
cmake -DBUILD_TESTS=OFF ..
make
```

---

## Running

### Starting the Server

#### Method 1: Default Port (8080)
```bash
cd build
./rubiks_solver
```

#### Method 2: Custom Port
```bash
cd build
./rubiks_solver 3000
```

#### Method 3: Using Makefile
```bash
# Run on port 8080
make run

# Run on custom port (will prompt)
make run-port
```

### Expected Startup Output
```
==================================
Rubik's Cube Solver Backend
==================================

Testing cube implementation...
Created solved cube: ✓
Scrambled cube: ✓
Testing solver on easy scramble...
Solver test passed! Found solution with 2 moves

Starting HTTP server...
Server started on port 8080
API Endpoints:
  GET  /status         - Server status
  GET  /cube           - Current cube state
  POST /cube/reset     - Reset to solved state
  POST /cube/scramble  - Scramble the cube
  POST /cube/move      - Apply a move
  POST /cube/solve     - Solve the cube
  POST /cube/state     - Set cube state
```

### Stopping the Server
- Press `Ctrl+C` in the terminal
- The server will shut down gracefully

### Running in Background
```bash
# Start in background
cd build
./rubiks_solver &

# Check if running
ps aux | grep rubiks_solver

# Stop background process
pkill rubiks_solver
```

### Running as a Service (Linux)

Create `/etc/systemd/system/rubiks-solver.service`:
```ini
[Unit]
Description=Rubik's Cube Solver API
After=network.target

[Service]
Type=simple
User=youruser
WorkingDirectory=/path/to/rubiks-cube-solver/build
ExecStart=/path/to/rubiks-cube-solver/build/rubiks_solver 8080
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

Enable and start:
```bash
sudo systemctl enable rubiks-solver
sudo systemctl start rubiks-solver
sudo systemctl status rubiks-solver
```

---

## Testing

### Running Unit Tests

#### Automatic (during setup)
```bash
./setup.sh  # Tests run automatically
```

#### Manual
```bash
cd build
./test_solver
```

**Expected test output:**
```
========================================
Running Rubik's Cube Solver Tests
========================================

Testing cube initialization...
  ✓ Cube initialized as solved
Testing cube moves...
  ✓ U and U' moves work correctly
  ✓ All basic moves and inverses work
  ✓ Double moves work correctly
Testing cube serialization...
  ✓ Serialization and deserialization work
Testing scramble...
  ✓ Scramble produces non-solved state
Testing solver on already solved cube...
  ✓ Solver returns empty solution for solved cube
Testing solver on easy case...
  ✓ Solver found solution with 2 moves
Testing move sequence...
  ✓ Move sequence and inverse restore solved state
Testing getAllMoves...
  ✓ getAllMoves returns 18 moves
Testing JSON output...
  ✓ JSON output contains expected fields

========================================
All tests passed! ✓
========================================
```

### Testing the API

#### Using test script
```bash
# Make sure server is running first
./test_api.sh
```

#### Manual API testing
```bash
# Test status endpoint
curl http://localhost:8080/status

# Get cube state
curl http://localhost:8080/cube

# Scramble
curl -X POST http://localhost:8080/cube/scramble \
  -H "Content-Type: application/json" \
  -d '{"moves": 15}'

# Apply a move
curl -X POST http://localhost:8080/cube/move \
  -H "Content-Type: application/json" \
  -d '{"move": "R"}'

# Solve
curl -X POST http://localhost:8080/cube/solve \
  -H "Content-Type: application/json" \
  -d '{"maxDepth": 20}'
```

### Testing with Frontend

Once your React frontend is ready:
```javascript
// Test connection
fetch('http://localhost:8080/status')
  .then(r => r.json())
  .then(console.log)
  .catch(console.error);
```

---

## Troubleshooting

### Build Issues

#### CMake not found
```bash
# Ubuntu/Debian
sudo apt install cmake

# macOS
brew install cmake

# Verify
cmake --version
```

#### Compiler not found or too old
```bash
# Ubuntu/Debian - install newer gcc
sudo apt install g++-11
export CXX=g++-11

# macOS - update Xcode
xcode-select --install
```

#### "No such file or directory" errors
```bash
# Make sure you're in the right directory
pwd  # Should be in rubiks-cube-solver/

# Check file structure
ls -la include/ src/ tests/
```

#### CMake configuration fails
```bash
# Clean and retry
rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Runtime Issues

#### "Port already in use"
```bash
# Check what's using the port
lsof -i :8080

# Kill the process
kill -9 <PID>

# Or use a different port
./rubiks_solver 8081
```

#### "Cannot connect to server"
```bash
# Check if server is running
ps aux | grep rubiks_solver

# Check if port is listening
netstat -an | grep 8080
# or
ss -tulpn | grep 8080

# Try connecting locally
curl http://localhost:8080/status
```

#### "Address already in use"
```bash
# Wait a few seconds for port to be released
sleep 5
./rubiks_solver

# Or use SO_REUSEADDR flag (already in code)
```

#### Firewall blocking connections
```bash
# Ubuntu/Debian
sudo ufw allow 8080

# Check firewall status
sudo ufw status

# macOS - allow in System Preferences > Security > Firewall
```

### Performance Issues

#### Solver is too slow
```bash
# Use fewer scramble moves for testing
curl -X POST http://localhost:8080/cube/scramble -d '{"moves": 7}'

# Use lower maxDepth
curl -X POST http://localhost:8080/cube/solve -d '{"maxDepth": 12}'

# Build with optimizations
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
make
./rubiks_solver
```

#### High memory usage
```bash
# Monitor memory
top -p $(pgrep rubiks_solver)

# Reduce maxDepth to use less memory
```

### Testing Issues

#### Tests fail
```bash
# Run tests with verbose output
cd build
./test_solver 2>&1 | tee test_output.log

# Check specific test
gdb ./test_solver
```

#### API tests fail
```bash
# Make sure server is running
curl http://localhost:8080/status

# Check server logs
# (Server outputs to stdout)

# Test individual endpoints
curl -v http://localhost:8080/status
```

### System-Specific Issues

#### macOS: "Developer tools not installed"
```bash
xcode-select --install
```

#### Linux: "Permission denied"
```bash
# Make scripts executable
chmod +x setup.sh test_api.sh

# Don't need sudo to build/run
```

#### Windows WSL: "Command not found"
```bash
# Make sure you're in WSL, not PowerShell
wsl

# Install build tools
sudo apt update
sudo apt install build-essential cmake
```

---

## Verification Checklist

After installation, verify everything works:

- [ ] Code is downloaded/cloned
- [ ] Dependencies installed (gcc/clang, cmake)
- [ ] Project builds successfully
- [ ] Unit tests pass
- [ ] Server starts without errors
- [ ] Can access http://localhost:8080/status
- [ ] API endpoints respond correctly
- [ ] Test script runs successfully

---

## Next Steps

1. ✅ **Installation complete** → Start building your React frontend
2. 📚 **Read API docs** → Check README.md for endpoint details
3. 🎨 **Frontend integration** → Use the API client code provided
4. 🚀 **Deploy** → Consider Docker or systemd service
5. 📈 **Optimize** → Profile and improve performance as needed

---

## Quick Reference

### File Locations
- **Executable**: `build/rubiks_solver`
- **Tests**: `build/test_solver`
- **Library**: `build/librubiks_core.a`
- **Source**: `src/` and `include/`

### Important Commands
```bash
# Build
make build

# Run tests
make test

# Start server
make run

# Clean
make clean

# Rebuild
make clean build
```

### API Base URL
```
http://localhost:8080
```

### Default Port
```
8080
```

---

**Installation complete! Your backend is ready for frontend development! 🎉**

For detailed API documentation, see [README.md](README.md)  
For quick start, see [QUICKSTART.md](QUICKSTART.md)  
For file details, see [FILES.md](FILES.md)