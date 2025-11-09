# Quick Start Guide

Get your Rubik's Cube Solver backend running in 5 minutes!

## 🚀 Super Fast Setup

### Option 1: Using Setup Script (Recommended)
```bash
chmod +x setup.sh
./setup.sh
cd build
./rubiks_solver
```

### Option 2: Using Makefile
```bash
make build
make run
```

### Option 3: Manual CMake
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
./rubiks_solver
```

## 📋 Prerequisites

**Linux:**
```bash
sudo apt update
sudo apt install build-essential cmake git
```

**macOS:**
```bash
xcode-select --install
brew install cmake
```

## ✅ Verify Installation

### 1. Run Tests
```bash
cd build
./test_solver
```

Expected output:
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
...
All tests passed! ✓
```

### 2. Start Server
```bash
./rubiks_solver
```

Expected output:
```
==================================
Rubik's Cube Solver Backend
==================================

Testing cube implementation...
Created solved cube: ✓
Scrambled cube: ✓

Starting HTTP server...
Server started on port 8080
API Endpoints:
  GET  /status
  GET  /cube
  POST /cube/reset
  ...
```

### 3. Test API
In another terminal:
```bash
# Test status endpoint
curl http://localhost:8080/status

# Should return:
# {"status":"running","solver":"IDA*"}
```

Or use the testing script:
```bash
chmod +x test_api.sh
./test_api.sh
```

## 🎯 Quick API Examples

### Get Cube State
```bash
curl http://localhost:8080/cube
```

### Scramble the Cube
```bash
curl -X POST http://localhost:8080/cube/scramble \
  -H "Content-Type: application/json" \
  -d '{"moves": 15}'
```

### Apply a Move
```bash
curl -X POST http://localhost:8080/cube/move \
  -H "Content-Type: application/json" \
  -d '{"move": "R"}'
```

### Solve the Cube
```bash
curl -X POST http://localhost:8080/cube/solve \
  -H "Content-Type: application/json" \
  -d '{"maxDepth": 20}'
```

## 🔧 Common Issues

### Port Already in Use
```bash
./rubiks_solver 3000  # Use different port
```

### Compilation Fails
```bash
# Check compiler version
g++ --version  # Need 7.0+

# Clean and rebuild
make clean
make build
```

### Server Won't Start
```bash
# Check if port is available
lsof -i :8080

# Check firewall
sudo ufw allow 8080
```

## 🎨 React Frontend Setup

Create a new React app and add this API client:

```javascript
// src/api/cube.js
const API_URL = 'http://localhost:8080';

export const cubeAPI = {
  getStatus: () => 
    fetch(`${API_URL}/status`).then(r => r.json()),
  
  getCube: () => 
    fetch(`${API_URL}/cube`).then(r => r.json()),
  
  reset: () => 
    fetch(`${API_URL}/cube/reset`, { method: 'POST' })
      .then(r => r.json()),
  
  scramble: (moves = 20) => 
    fetch(`${API_URL}/cube/scramble`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ moves })
    }).then(r => r.json()),
  
  move: (move) => 
    fetch(`${API_URL}/cube/move`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ move })
    }).then(r => r.json()),
  
  solve: (maxDepth = 20) => 
    fetch(`${API_URL}/cube/solve`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ maxDepth })
    }).then(r => r.json())
};
```

Use in component:
```javascript
import { useState, useEffect } from 'react';
import { cubeAPI } from './api/cube';

function App() {
  const [cube, setCube] = useState(null);
  const [loading, setLoading] = useState(false);
  
  useEffect(() => {
    cubeAPI.getCube().then(setCube);
  }, []);
  
  const handleScramble = async () => {
    setLoading(true);
    const newCube = await cubeAPI.scramble(20);
    setCube(newCube);
    setLoading(false);
  };
  
  const handleSolve = async () => {
    setLoading(true);
    const result = await cubeAPI.solve(20);
    console.log('Solution:', result.solution);
    setCube(result.cube);
    setLoading(false);
  };
  
  if (!cube) return <div>Loading...</div>;
  
  return (
    <div>
      <h1>Rubik's Cube Solver</h1>
      <div>Status: {cube.isSolved ? '✓ Solved' : '✗ Scrambled'}</div>
      <button onClick={handleScramble} disabled={loading}>
        Scramble
      </button>
      <button onClick={handleSolve} disabled={loading}>
        Solve
      </button>
    </div>
  );
}
```

## 📊 Performance Tips

### For Faster Solving
```bash
# Build with optimizations
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
make
```

### For Easy Testing
```bash
# Use fewer scramble moves during development
curl -X POST http://localhost:8080/cube/scramble -d '{"moves": 7}'

# Use lower maxDepth for faster results
curl -X POST http://localhost:8080/cube/solve -d '{"maxDepth": 12}'
```

## 📚 Next Steps

1. ✅ Server running? → Build your React frontend
2. ✅ API working? → Add cube visualization
3. ✅ Solving works? → Add move animations
4. 📖 Read full [README.md](README.md) for details
5. 🔧 Customize solver parameters for your needs

## 🆘 Need Help?

- **Build issues**: Check `README.md` troubleshooting section
- **API not responding**: Verify server is running with `curl localhost:8080/status`
- **Slow solving**: Reduce scramble moves or maxDepth
- **Frontend CORS**: Server has CORS enabled by default

## 🎉 You're Ready!

Your backend is running at `http://localhost:8080`

Now go build an awesome React frontend! 🚀