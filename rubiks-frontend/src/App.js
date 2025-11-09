// Updated rubiks-frontend/src/App.js
import React, { useState, useEffect } from 'react';
import { Shuffle, RotateCw, Play, Zap, Clock, Cpu } from 'lucide-react';
import { cubeAPI } from './api';

const App = () => {
  const [cubeState, setCubeState] = useState(null);
  const [solution, setSolution] = useState([]);
  const [isSolving, setIsSolving] = useState(false);
  const [isScrambling, setIsScrambling] = useState(false);
  const [serverStatus, setServerStatus] = useState('checking...');
  const [stats, setStats] = useState(null);
  const [scrambleMoves, setScrambleMoves] = useState(7);
  const [availableSolvers, setAvailableSolvers] = useState([]);
  const [currentSolver, setCurrentSolver] = useState('sequential');
  const [solverHistory, setSolverHistory] = useState([]);

  const COLOR_MAP = {
    'W': 'bg-white border-gray-300',
    'Y': 'bg-yellow-400',
    'G': 'bg-green-500',
    'B': 'bg-blue-500',
    'R': 'bg-red-500',
    'O': 'bg-orange-500'
  };

  const FACE_NAMES = {
    'U': 'Up (White)',
    'D': 'Down (Yellow)',
    'F': 'Front (Green)',
    'B': 'Back (Blue)',
    'L': 'Left (Orange)',
    'R': 'Right (Red)'
  };

  const SOLVER_DESCRIPTIONS = {
    'sequential': 'Single-threaded brute-force DFS',
    'openmp': 'Parallel DFS with OpenMP',
    'mpi': 'Distributed DFS with MPI',
    'hybrid': 'MPI + OpenMP combined'
  };

  useEffect(() => {
    checkBackend();
    loadCube();
    loadSolvers();
  }, []);

  const checkBackend = async () => {
    try {
      const response = await fetch('http://localhost:8080/status');
      if (response.ok) {
        setServerStatus('connected');
      } else {
        setServerStatus('error');
      }
    } catch (err) {
      setServerStatus('disconnected');
    }
  };

  const loadCube = async () => {
    try {
      const data = await cubeAPI.getCube();
      setCubeState(data);
    } catch (err) {
      console.error('Failed to load cube:', err);
    }
  };

  const loadSolvers = async () => {
    try {
      const response = await fetch('http://localhost:8080/solvers');
      const data = await response.json();
      setAvailableSolvers(data.solvers);
      setCurrentSolver(data.current);
    } catch (err) {
      console.error('Failed to load solvers:', err);
      setAvailableSolvers(['sequential']);
    }
  };

  const handleSolverChange = async (solver) => {
    try {
      const response = await fetch('http://localhost:8080/solver/select', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ solver })
      });
      
      if (response.ok) {
        const data = await response.json();
        setCurrentSolver(solver);
        console.log('Switched to:', data.solver);
      }
    } catch (err) {
      console.error('Failed to switch solver:', err);
    }
  };

  const handleScramble = async () => {
    setIsScrambling(true);
    setSolution([]);
    setStats(null);
    try {
      const data = await cubeAPI.scramble(scrambleMoves);
      setCubeState(data);
    } catch (err) {
      console.error('Scramble failed:', err);
      alert('Backend not responding. Make sure the C++ server is running!');
    } finally {
      setIsScrambling(false);
    }
  };

  const handleSolve = async () => {
    setIsSolving(true);
    setSolution([]);
    setStats(null);
    
    const startTime = performance.now();
    
    try {
      if (!cubeState.isSolved) {
        console.log(`Solving with ${currentSolver}...`);
      }
      
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 120000);
      
      const response = await fetch('http://localhost:8080/cube/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ maxDepth: 15 }),
        signal: controller.signal
      });
      
      clearTimeout(timeoutId);
      
      if (!response.ok) {
        throw new Error('Solve request failed');
      }
      
      const data = await response.json();
      const endTime = performance.now();
      const clientTime = (endTime - startTime) / 1000;
      
      if (data.solution && data.solution.length > 0) {
        setSolution(data.solution);
        const newStats = {
          moves: data.moves || 0,
          nodes: data.nodes || 0,
          time: data.time || 0,
          solver: data.solver || currentSolver,
          clientTime: clientTime
        };
        setStats(newStats);
        setCubeState(data.cube);
        
        // Add to history for comparison
        setSolverHistory(prev => [...prev, {
          solver: data.solver,
          scramble: scrambleMoves,
          time: data.time,
          moves: data.moves,
          nodes: data.nodes,
          timestamp: new Date().toLocaleTimeString()
        }]);
      } else {
        alert('No solution found within depth limit. Try scrambling with fewer moves.');
      }
    } catch (err) {
      if (err.name === 'AbortError') {
        alert('Solving timed out (120s). The cube is too scrambled.');
      } else {
        console.error('Solve failed:', err);
        alert('Solve failed. Backend might be busy or cube is too complex.');
      }
    } finally {
      setIsSolving(false);
    }
  };

  const handleReset = async () => {
    setSolution([]);
    setStats(null);
    try {
      const data = await fetch('http://localhost:8080/cube/reset', {
        method: 'POST'
      }).then(r => r.json());
      setCubeState(data);
    } catch (err) {
      console.error('Reset failed:', err);
    }
  };

  const handleMove = async (move) => {
    try {
      const data = await cubeAPI.applyMove(move);
      setCubeState(data);
    } catch (err) {
      console.error('Move failed:', err);
    }
  };

  const clearHistory = () => {
    setSolverHistory([]);
  };

  const renderFace = (faceName, faceData) => {
    if (!faceData) return null;

    return (
      <div className="inline-block m-2">
        <div className="text-xs font-bold mb-1 text-center text-gray-700">
          {FACE_NAMES[faceName]}
        </div>
        <div className="grid grid-cols-3 gap-1 border-4 border-gray-800 p-1 bg-gray-800 rounded-lg shadow-xl">
          {faceData.map((color, idx) => (
            <div
              key={idx}
              className={`w-10 h-10 ${COLOR_MAP[color] || 'bg-gray-300'} 
                border-2 border-gray-600 rounded shadow-md 
                flex items-center justify-center text-xs font-bold text-gray-700`}
            >
              {color}
            </div>
          ))}
        </div>
      </div>
    );
  };

  const availableMoves = ['U', "U'", 'U2', 'D', "D'", 'D2', 
                          'F', "F'", 'F2', 'B', "B'", 'B2',
                          'L', "L'", 'L2', 'R', "R'", 'R2'];

  if (!cubeState) {
    return (
      <div className="min-h-screen bg-gradient-to-br from-purple-50 to-blue-50 flex items-center justify-center">
        <div className="text-2xl font-bold text-gray-600">Loading cube...</div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-purple-50 to-blue-50 p-8">
      <div className="max-w-7xl mx-auto">
        <div className="bg-white rounded-2xl shadow-2xl p-8">
          
          {/* Header */}
          <div className="flex items-center justify-between mb-8">
            <div>
              <h1 className="text-4xl font-bold bg-gradient-to-r from-purple-600 to-blue-600 bg-clip-text text-transparent">
                Rubik's Cube Solver - Parallel Computing
              </h1>
              <p className="text-sm text-gray-600 mt-2">
                Backend: <span className={`font-bold ${
                  serverStatus === 'connected' ? 'text-green-600' : 'text-red-600'
                }`}>
                  {serverStatus === 'connected' ? '● Connected' : '● Disconnected'}
                </span>
              </p>
            </div>
            
            {cubeState.isSolved && (
              <div className="px-6 py-3 bg-green-100 border-2 border-green-500 rounded-xl">
                <span className="text-green-700 font-bold text-lg">✓ SOLVED!</span>
              </div>
            )}
          </div>

          {/* Solver Selection */}
          <div className="mb-6 bg-gradient-to-r from-blue-50 to-purple-50 rounded-xl p-4 border-2 border-blue-200">
            <h3 className="font-bold text-gray-700 mb-3 flex items-center gap-2">
              <Cpu size={20} />
              Algorithm Selection
            </h3>
            <div className="grid grid-cols-2 md:grid-cols-5 gap-3">
              {availableSolvers.map(solver => (
                <button
                  key={solver}
                  onClick={() => handleSolverChange(solver)}
                  disabled={isSolving || isScrambling}
                  className={`px-4 py-3 rounded-lg font-bold text-sm transition ${
                    currentSolver === solver
                      ? 'bg-blue-600 text-white shadow-lg scale-105'
                      : 'bg-white text-gray-700 hover:bg-blue-50 border-2 border-gray-300'
                  } disabled:opacity-50 disabled:cursor-not-allowed`}
                >
                  <div>{solver.toUpperCase()}</div>
                  <div className="text-xs font-normal mt-1 opacity-80">
                    {SOLVER_DESCRIPTIONS[solver]?.split(' ').slice(0, 2).join(' ')}
                  </div>
                </button>
              ))}
            </div>
          </div>

          <div className="grid lg:grid-cols-2 gap-8">
            
            {/* Left: Cube Visualization */}
            <div>
              <div className="bg-gray-50 rounded-xl p-6">
                <h2 className="text-xl font-bold mb-4 text-gray-700">Cube State</h2>
                <div className="flex flex-col items-center space-y-2">
                  <div>{renderFace('U', cubeState.faces?.U)}</div>
                  <div className="flex space-x-2">
                    {renderFace('L', cubeState.faces?.L)}
                    {renderFace('F', cubeState.faces?.F)}
                    {renderFace('R', cubeState.faces?.R)}
                  </div>
                  <div>{renderFace('D', cubeState.faces?.D)}</div>
                  <div>{renderFace('B', cubeState.faces?.B)}</div>
                </div>
              </div>

              <div className="mt-4 bg-white p-4 rounded-lg border-2 border-gray-200">
                <label className="flex items-center justify-between mb-2">
                  <span className="font-bold text-gray-700">Scramble Difficulty:</span>
                  <span className="text-blue-600 font-bold text-lg">{scrambleMoves} moves</span>
                </label>
                <input
                  type="range"
                  min="3"
                  max="12"
                  value={scrambleMoves}
                  onChange={(e) => setScrambleMoves(parseInt(e.target.value))}
                  className="w-full h-2 bg-gray-200 rounded-lg appearance-none cursor-pointer accent-purple-600"
                  disabled={isSolving || isScrambling}
                />
                <div className="flex justify-between text-xs text-gray-500 mt-1">
                  <span>Easy (3)</span>
                  <span>Medium (7)</span>
                  <span>Hard (12)</span>
                </div>
                <p className="text-xs text-gray-600 mt-2">
                  💡 Recommended: 3-7 for testing parallel performance
                </p>
              </div>

              {/* Control Buttons */}
              <div className="mt-6 flex gap-3">
                <button
                  onClick={handleScramble}
                  disabled={isScrambling || isSolving}
                  className="flex-1 flex items-center justify-center gap-2 px-6 py-4 bg-gradient-to-r from-purple-500 to-purple-600 text-white rounded-xl hover:from-purple-600 hover:to-purple-700 disabled:opacity-50 disabled:cursor-not-allowed transition shadow-lg font-bold text-lg"
                >
                  {isScrambling ? <Zap size={24} className="animate-spin" /> : <Shuffle size={24} />}
                  {isScrambling ? 'Scrambling...' : 'Scramble'}
                </button>
                
                <button
                  onClick={handleSolve}
                  disabled={cubeState.isSolved || isSolving || isScrambling}
                  className="flex-1 flex items-center justify-center gap-2 px-6 py-4 bg-gradient-to-r from-blue-500 to-blue-600 text-white rounded-xl hover:from-blue-600 hover:to-blue-700 disabled:opacity-50 disabled:cursor-not-allowed transition shadow-lg font-bold text-lg"
                >
                  {isSolving ? <Zap size={24} className="animate-pulse" /> : <Play size={24} />}
                  {isSolving ? 'Solving...' : 'Solve'}
                </button>
                
                <button
                  onClick={handleReset}
                  disabled={isSolving || isScrambling}
                  className="px-6 py-4 bg-gray-500 text-white rounded-xl hover:bg-gray-600 transition shadow-lg disabled:opacity-50"
                >
                  <RotateCw size={24} />
                </button>
              </div>
            </div>

            {/* Right: Controls & Info */}
            <div className="space-y-6">
              
              {/* Manual Moves */}
              <div className="bg-gray-50 rounded-xl p-6">
                <h2 className="text-xl font-bold mb-4 text-gray-700">Manual Moves</h2>
                <div className="grid grid-cols-6 gap-2">
                  {availableMoves.map(move => (
                    <button
                      key={move}
                      onClick={() => handleMove(move)}
                      disabled={isSolving || isScrambling}
                      className="px-3 py-2 bg-white border-2 border-gray-300 rounded-lg hover:bg-blue-50 hover:border-blue-400 disabled:opacity-50 transition font-mono font-bold text-sm"
                    >
                      {move}
                    </button>
                  ))}
                </div>
              </div>

              {/* Solution Display */}
              {solution.length > 0 && (
                <div className="bg-gradient-to-r from-green-50 to-emerald-50 rounded-xl p-6 border-2 border-green-300">
                  <h2 className="text-xl font-bold mb-3 text-green-800">
                    Solution Found! 🎉
                  </h2>
                  <div className="bg-white p-4 rounded-lg mb-3">
                    <div className="flex flex-wrap gap-2">
                      {solution.map((move, idx) => (
                        <span
                          key={idx}
                          className="px-3 py-1 bg-green-100 border border-green-300 rounded font-mono font-bold text-sm"
                        >
                          {move}
                        </span>
                      ))}
                    </div>
                  </div>
                  
                  {stats && (
                    <div className="grid grid-cols-2 gap-3 text-sm">
                      <div className="bg-white p-3 rounded-lg">
                        <div className="font-bold text-sm text-gray-600 mb-1">Algorithm</div>
                        <div className="font-bold text-lg text-purple-600">{stats.solver}</div>
                      </div>
                      <div className="bg-white p-3 rounded-lg">
                        <div className="font-bold text-sm text-gray-600 mb-1">Moves</div>
                        <div className="font-bold text-2xl text-green-600">{stats.moves}</div>
                      </div>
                      <div className="bg-white p-3 rounded-lg">
                        <div className="font-bold text-sm text-gray-600 mb-1">Time</div>
                        <div className="font-bold text-2xl text-blue-600 flex items-center gap-1">
                          <Clock size={20} />
                          {stats.time.toFixed(3)}s
                        </div>
                      </div>
                      <div className="bg-white p-3 rounded-lg">
                        <div className="font-bold text-sm text-gray-600 mb-1">Nodes</div>
                        <div className="font-bold text-2xl text-orange-600">{stats.nodes}</div>
                      </div>
                    </div>
                  )}
                </div>
              )}

              {/* Performance History */}
              {solverHistory.length > 0 && (
                <div className="bg-white rounded-xl p-6 border-2 border-gray-200">
                  <div className="flex justify-between items-center mb-3">
                    <h3 className="font-bold text-gray-700">Performance History</h3>
                    <button
                      onClick={clearHistory}
                      className="text-xs text-red-600 hover:text-red-800 font-bold"
                    >
                      Clear
                    </button>
                  </div>
                  <div className="space-y-2 max-h-64 overflow-y-auto">
                    {solverHistory.slice(-10).reverse().map((entry, idx) => (
                      <div key={idx} className="bg-gray-50 p-3 rounded-lg text-sm">
                        <div className="flex justify-between items-center">
                          <span className="font-bold text-purple-600">{entry.solver}</span>
                          <span className="text-xs text-gray-500">{entry.timestamp}</span>
                        </div>
                        <div className="grid grid-cols-4 gap-2 mt-2 text-xs">
                          <div>
                            <span className="text-gray-600">Scramble:</span> 
                            <span className="font-bold ml-1">{entry.scramble}</span>
                          </div>
                          <div>
                            <span className="text-gray-600">Time:</span> 
                            <span className="font-bold ml-1">{entry.time.toFixed(2)}s</span>
                          </div>
                          <div>
                            <span className="text-gray-600">Moves:</span> 
                            <span className="font-bold ml-1">{entry.moves}</span>
                          </div>
                          <div>
                            <span className="text-gray-600">Nodes:</span> 
                            <span className="font-bold ml-1">{entry.nodes}</span>
                          </div>
                        </div>
                      </div>
                    ))}
                  </div>
                </div>
              )}

              {/* Instructions */}
              <div className="bg-blue-50 rounded-xl p-6 border border-blue-200">
                <h3 className="font-bold text-blue-900 mb-2">Parallel Computing Project:</h3>
                <ol className="text-sm text-blue-800 space-y-1 list-decimal list-inside">
                  <li>Select an <strong>algorithm</strong> (Sequential, OpenMP, MPI, Hybrid)</li>
                  <li>Click <strong>Scramble</strong> with desired difficulty</li>
                  <li>Click <strong>Solve</strong> and compare metrics</li>
                  <li>Check <strong>Performance History</strong> to compare algorithms</li>
                  <li><strong>Key Metrics:</strong> Time (speedup), Nodes (work done)</li>
                </ol>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default App;