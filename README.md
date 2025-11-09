# Rubik's Cube Solver - Backend API

A high-performance C++ backend for solving Rubik's Cube puzzles with REST API support for frontend integration.

## Features

- ✅ Complete 3x3x3 Rubik's Cube implementation
- ✅ IDA* (Iterative Deepening A*) solving algorithm
- ✅ REST API server for frontend communication
- ✅ Comprehensive unit tests
- ✅ CORS-enabled for React frontend
- ✅ Cube scrambling and state management
- ✅ Move notation support (U, R, F, B, L, D with ', 2 modifiers)

## Project Structure

```
rubiks-cube-solver/
├── CMakeLists.txt              # Main build configuration
├── README.md                   # This file
├── .gitignore                 # Git ignore rules
├── include/                   # Header files
│   ├── rubiks_cube.hpp        # Cube representation
│   ├── solver.hpp             # Solver interface
│   ├── ida_star_solver.hpp    # IDA* algorithm
│   └── http_server.hpp        # REST API server
├── src/                       # Implementation files
│   ├── rubiks_cube.cpp
│   ├── ida_star_solver.cpp
│   ├── http_server.cpp
│   └── main.cpp               # Entry point
└── tests/                     # Unit tests
    ├── CMakeLists.txt
    └── test_solver.cpp
```

## Requirements

### System Requirements
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.16 or higher
- POSIX-compliant system (Linux, macOS, WSL on Windows)

### Dependencies
All dependencies are header-only or system libraries:
- Standard C++ library (no external dependencies required!)

## Installation

### Linux (Ubuntu/Debian)

```bash
# Install build tools
sudo apt update
sudo apt install build-essential cmake git

# Clone the repository
git clone <your-repo-url>
cd rubiks-cube-solver

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
make -j$(nproc)

# Run tests
./test_solver

# Run the server
./rubiks_solver
```

### macOS

```bash
# Install Xcode Command Line Tools (if not already installed)
xcode-select --install

# Install CMake using Homebrew
brew install cmake

# Clone the repository
git clone <your-repo-url>
cd rubiks-cube-solver

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j$(sysctl -n hw.ncpu)

# Run tests
./test_solver

# Run the server
./rubiks_solver
```

### Windows (WSL)

```bash
# Install WSL (Windows Subsystem for Linux) if not already installed
# Then follow the Linux instructions above

# Or use MinGW/MSYS2:
# Install MSYS2 from https://www.msys2.org/
# In MSYS2 terminal:
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake make

# Then follow standard build process
```

## Building Options

### Debug Build
```bash
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Release Build (Optimized)
```bash
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

### Build Without Tests
```bash
cmake -DBUILD_TESTS=OFF ..
make
```

## Running the Server

### Basic Usage
```bash
# Run on default port (8080)
./rubiks_solver

# Run on custom port
./rubiks_solver 3000
```

The server will start and display:
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

## API Documentation

### Base URL
```
http://localhost:8080
```

### Endpoints

#### 1. Get Server Status
```http
GET /status
```

**Response:**
```json
{
  "status": "running",
  "solver": "IDA*"
}
```

#### 2. Get Current Cube State
```http
GET /cube
```

**Response:**
```json
{
  "faces": {
    "U": ["W","W","W","W","W","W","W","W","W"],
    "D": ["Y","Y","Y","Y","Y","Y","Y","Y","Y"],
    "F": ["G","G","G","G","G","G","G","G","G"],
    "B": ["B","B","B","B","B","B","B","B","B"],
    "L": ["O","O","O","O","O","O","O","O","O"],
    "R": ["R","R","R","R","R","R","R","R","R"]
  },
  "isSolved": true
}
```

#### 3. Reset Cube to Solved State
```http
POST /cube/reset
```

**Response:** Same as GET /cube with solved state

#### 4. Scramble Cube
```http
POST /cube/scramble
Content-Type: application/json

{
  "moves": 20
}
```

**Parameters:**
- `moves` (optional): Number of random moves (default: 20)

**Response:** Updated cube state

#### 5. Apply a Move
```http
POST /cube/move
Content-Type: application/json

{
  "move": "R"
}
```

**Supported Moves:**
- Basic: `U`, `D`, `F`, `B`, `L`, `R`
- Prime (counter-clockwise): `U'`, `D'`, `F'`, `B'`, `L'`, `R'`
- Double (180°): `U2`, `D2`, `F2`, `B2`, `L2`, `R2`

**Response:** Updated cube state

#### 6. Solve Cube
```http
POST /cube/solve
Content-Type: application/json

{
  "maxDepth": 20
}
```

**Parameters:**
- `maxDepth` (optional): Maximum search depth (default: 20)

**Response:**
```json
{
  "solution": ["R", "U", "R'", "U'"],
  "moves": 4,
  "nodes": 1523,
  "time": 0.023,
  "cube": { /* updated cube state */ }
}
```

#### 7. Set Cube State
```http
POST /cube/state
Content-Type: application/json

{
  "state": "WWWWWWWWWYYYYYYYYYYGGGGGGGGGGBBBBBBBBBBOOOOOOOOOORRRRRRRRRR"
}
```

**State Format:** 54 characters representing the cube:
- Positions 0-8: Up face (White)
- Positions 9-17: Down face (Yellow)
- Positions 18-26: Front face (Green)
- Positions 27-35: Back face (Blue)
- Positions 36-44: Left face (Orange)
- Positions 45-53: Right face (Red)

**Response:** Updated cube state

## Testing with curl

```bash
# Get status
curl http://localhost:8080/status

# Get cube state
curl http://localhost:8080/cube

# Reset cube
curl -X POST http://localhost:8080/cube/reset

# Scramble
curl -X POST http://localhost:8080/cube/scramble \
  -H "Content-Type: application/json" \
  -d '{"moves": 15}'

# Apply move
curl -X POST http://localhost:8080/cube/move \
  -H "Content-Type: application/json" \
  -d '{"move": "R"}'

# Solve cube
curl -X POST http://localhost:8080/cube/solve \
  -H "Content-Type: application/json" \
  -d '{"maxDepth": 20}'
```

## React Frontend Integration

Example React code to connect to the backend:

```javascript
// API client
const API_URL = 'http://localhost:8080';

export const cubeAPI = {
  getStatus: async () => {
    const response = await fetch(`${API_URL}/status`);
    return response.json();
  },
  
  getCubeState: async () => {
    const response = await fetch(`${API_URL}/cube`);
    return response.json();
  },
  
  reset: async () => {
    const response = await fetch(`${API_URL}/cube/reset`, {
      method: 'POST'
    });
    return response.json();
  },
  
  scramble: async (moves = 20) => {
    const response = await fetch(`${API_URL}/cube/scramble`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ moves })
    });
    return response.json();
  },
  
  applyMove: async (move) => {
    const response = await fetch(`${API_URL}/cube/move`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ move })
    });
    return response.json();
  },
  
  solve: async (maxDepth = 20) => {
    const response = await fetch(`${API_URL}/cube/solve`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ maxDepth })
    });
    return response.json();
  }
};

// Usage in component
import { useState, useEffect } from 'react';
import { cubeAPI } from './api';

function CubeSolver() {
  const [cube, setCube] = useState(null);
  const [solution, setSolution] = useState(null);
  
  useEffect(() => {
    loadCube();
  }, []);
  
  const loadCube = async () => {
    const state = await cubeAPI.getCubeState();
    setCube(state);
  };
  
  const handleScramble = async () => {
    const state = await cubeAPI.scramble(20);
    setCube(state);
  };
  
  const handleSolve = async () => {
    const result = await cubeAPI.solve();
    setSolution(result.solution);
    setCube(result.cube);
  };
  
  return (
    <div>
      <button onClick={handleScramble}>Scramble</button>
      <button onClick={handleSolve}>Solve</button>
      {solution && <div>Solution: {solution.join(' ')}</div>}
    </div>
  );
}
```

## Algorithm Details

### IDA* (Iterative Deepening A*)

The solver uses IDA* algorithm with the following optimizations:

1. **Manhattan Distance Heuristic**: Estimates the minimum number of moves needed
2. **Move Pruning**: Eliminates redundant move sequences (e.g., U followed by U')
3. **Iterative Deepening**: Gradually increases search depth
4. **Memory Efficient**: Uses depth-first search with minimal memory

### Performance

- **Easy scrambles** (5-7 moves): < 1 second
- **Medium scrambles** (8-12 moves): 1-10 seconds
- **Hard scrambles** (13-20 moves): May take several minutes

**Note:** For deeply scrambled cubes (20+ moves), consider implementing:
- Pattern databases
- Korf's algorithm
- Parallel processing

## Development

### Running Tests
```bash
cd build
./test_solver
```

### Adding New Solvers

1. Create header in `include/` inheriting from `Solver`
2. Implement `solve()` method
3. Add to CMakeLists.txt
4. Update `http_server.cpp` to use new solver

### Debug Mode
```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
gdb ./rubiks_solver
```

## Troubleshooting

### Port Already in Use
```bash
# Check what's using port 8080
lsof -i :8080

# Kill the process or use different port
./rubiks_solver 8081
```

### Compilation Errors
```bash
# Make sure C++17 is supported
g++ --version  # Should be 7.0 or higher

# Clean and rebuild
rm -rf build
mkdir build
cd build
cmake ..
make
```

### Solver Taking Too Long
- Reduce `maxDepth` parameter (try 15 or 12)
- Use fewer scramble moves for testing
- Consider implementing additional optimizations

## Performance Optimization

### Compile with Optimizations
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-O3 -march=native" ..
make
```

### Profile Performance
```bash
# Install profiling tools
sudo apt install valgrind

# Profile memory
valgrind --tool=memcheck ./rubiks_solver

# Profile CPU
valgrind --tool=callgrind ./rubiks_solver
```

## Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature-name`
3. Commit changes: `git commit -am 'Add feature'`
4. Push to branch: `git push origin feature-name`
5. Submit pull request

## License

MIT License - See LICENSE file for details

## Future Enhancements

- [ ] Pattern database implementation
- [ ] Multiple solving algorithms (Kociemba, Thistlethwaite)
- [ ] WebSocket support for real-time updates
- [ ] Cube animation state tracking
- [ ] Move optimization (reduce redundant moves)
- [ ] Database for storing solved states
- [ ] 2x2 and 4x4 cube support
- [ ] Benchmark mode

## Contact

For questions or issues, please open an issue on GitHub.

---

**Happy Solving! 🎲**