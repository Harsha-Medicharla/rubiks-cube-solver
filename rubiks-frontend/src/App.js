import React, { useState, useEffect } from 'react';
import { Shuffle, RotateCw, Play, Download, Code, Zap } from 'lucide-react';

const App = () => {
  const [cubeState, setCubeState] = useState(null);
  const [solution, setSolution] = useState('');
  const [isShuffled, setIsShuffled] = useState(false);
  const [isSolving, setIsSolving] = useState(false);
  const [moveHistory, setMoveHistory] = useState([]);
  const [algorithmType, setAlgorithmType] = useState('sequential');
  const [showCode, setShowCode] = useState(false);

  const FACE_COLORS = {
    0: { name: 'Front', color: 'bg-red-500', initial: 'R' },
    1: { name: 'Back', color: 'bg-orange-500', initial: 'O' },
    2: { name: 'Right', color: 'bg-yellow-500', initial: 'Y' },
    3: { name: 'Left', color: 'bg-green-500', initial: 'G' },
    4: { name: 'Top', color: 'bg-blue-500', initial: 'B' },
    5: { name: 'Bottom', color: 'bg-white', initial: 'W' }
  };

  const initializeCube = () => {
    const cube = Array(6).fill(null).map((_, faceIdx) =>
      Array(3).fill(null).map(() =>
        Array(3).fill(FACE_COLORS[faceIdx].initial)
      )
    );
    setCubeState(cube);
  };

  useEffect(() => {
    initializeCube();
  }, []);

  const shuffleCube = () => {
    if (!cubeState) return;
    
    const moves = Math.floor(Math.random() * 2) + 1;
    let newCube = JSON.parse(JSON.stringify(cubeState));
    const history = [];

    for (let i = 0; i < moves; i++) {
      const moveType = Math.floor(Math.random() * 3);
      const index = Math.floor(Math.random() * 3);
      const direction = Math.floor(Math.random() * 2);

      let moveDesc = '';
      if (moveType === 0) {
        newCube = applyHorizontalTwist(newCube, index, direction);
        moveDesc = `Horizontal row ${index}, ${direction ? 'CW' : 'CCW'}`;
      } else if (moveType === 1) {
        newCube = applyVerticalTwist(newCube, index, direction);
        moveDesc = `Vertical col ${index}, ${direction ? 'CW' : 'CCW'}`;
      } else {
        const face = Math.floor(Math.random() * 6);
        newCube = applySideTwist(newCube, face, direction);
        moveDesc = `Face ${FACE_COLORS[face].name}, ${direction ? 'CW' : 'CCW'}`;
      }
      history.push(moveDesc);
    }

    setCubeState(newCube);
    setMoveHistory(history);
    setIsShuffled(true);
    setSolution('');
  };

  const applyHorizontalTwist = (cube, row, dir) => {
    const newCube = JSON.parse(JSON.stringify(cube));
    const temp = [...newCube[0][row]];
    
    if (dir === 0) {
      newCube[0][row] = [...newCube[3][row]];
      newCube[3][row] = [...newCube[1][row]].reverse();
      newCube[1][row] = [...newCube[2][row]].reverse();
      newCube[2][row] = [...temp];
    } else {
      newCube[0][row] = [...newCube[2][row]];
      newCube[2][row] = [...newCube[1][row]].reverse();
      newCube[1][row] = [...newCube[3][row]].reverse();
      newCube[3][row] = [...temp];
    }

    if (row === 0) {
      newCube[4] = rotateFace(newCube[4], dir !== 0);
    } else if (row === 2) {
      newCube[5] = rotateFace(newCube[5], dir === 0);
    }

    return newCube;
  };

  const applyVerticalTwist = (cube, col, dir) => {
    const newCube = JSON.parse(JSON.stringify(cube));
    const temp = newCube.map(face => face[col] ? face[col][col] : null);
    
    if (dir === 0) {
      for (let i = 0; i < 3; i++) {
        const t = newCube[4][i][col];
        newCube[4][i][col] = newCube[0][i][col];
        newCube[0][i][col] = newCube[5][2-i][col];
        newCube[5][2-i][col] = newCube[1][i][2-col];
        newCube[1][i][2-col] = t;
      }
    } else {
      for (let i = 0; i < 3; i++) {
        const t = newCube[4][i][col];
        newCube[4][i][col] = newCube[1][i][2-col];
        newCube[1][i][2-col] = newCube[5][2-i][col];
        newCube[5][2-i][col] = newCube[0][i][col];
        newCube[0][i][col] = t;
      }
    }

    if (col === 0) {
      newCube[3] = rotateFace(newCube[3], dir !== 0);
    } else if (col === 2) {
      newCube[2] = rotateFace(newCube[2], dir === 0);
    }

    return newCube;
  };

  const applySideTwist = (cube, face, dir) => {
    let newCube = JSON.parse(JSON.stringify(cube));
    
    if (face < 2) {
      newCube[face] = rotateFace(newCube[face], dir === 0);
    } else if (face === 2) {
      newCube = applyVerticalTwist(newCube, 2, dir);
    } else if (face === 3) {
      newCube = applyVerticalTwist(newCube, 0, dir);
    } else if (face === 4) {
      newCube = applyHorizontalTwist(newCube, 0, dir);
    } else if (face === 5) {
      newCube = applyHorizontalTwist(newCube, 2, dir);
    }
    
    return newCube;
  };

  const rotateFace = (face, clockwise) => {
    const n = 3;
    const newFace = Array(n).fill(null).map(() => Array(n).fill(''));
    
    for (let i = 0; i < n; i++) {
      for (let j = 0; j < n; j++) {
        if (clockwise) {
          newFace[j][n-1-i] = face[i][j];
        } else {
          newFace[n-1-j][i] = face[i][j];
        }
      }
    }
    
    return newFace;
  };

  const checkSolved = (cube) => {
    return cube.every(face => {
      const firstColor = face[0][0];
      return face.every(row => row.every(cell => cell === firstColor));
    });
  };

  const solveCube = () => {
    if (!cubeState || !isShuffled) return;
    
    setIsSolving(true);
    
    setTimeout(() => {
      const reversedMoves = [...moveHistory].reverse().map(move => {
        if (move.includes('CW')) {
          return move.replace('CW', 'CCW');
        } else {
          return move.replace('CCW', 'CW');
        }
      });
      
      setSolution(reversedMoves.join(' → '));
      
      let newCube = JSON.parse(JSON.stringify(cubeState));
      reversedMoves.forEach(move => {
        const parts = move.split(',');
        const direction = parts[1].includes('CW') ? 1 : 0;
        
        if (move.includes('Horizontal')) {
          const row = parseInt(move.match(/row (\d)/)[1]);
          newCube = applyHorizontalTwist(newCube, row, direction);
        } else if (move.includes('Vertical')) {
          const col = parseInt(move.match(/col (\d)/)[1]);
          newCube = applyVerticalTwist(newCube, col, direction);
        } else if (move.includes('Face')) {
          const faceName = move.match(/Face (\w+)/)[1];
          const faceIdx = Object.keys(FACE_COLORS).find(
            key => FACE_COLORS[key].name === faceName
          );
          newCube = applySideTwist(newCube, parseInt(faceIdx), direction);
        }
      });
      
      setCubeState(newCube);
      setIsSolving(false);
      setIsShuffled(false);
    }, 500);
  };

  const reset = () => {
    initializeCube();
    setIsShuffled(false);
    setSolution('');
    setMoveHistory([]);
  };

  const exportCode = () => {
    const code = `// Refactored Rubik's Cube Solver
// Algorithm: ${algorithmType}
// State: ${cubeState ? 'Initialized' : 'Not initialized'}
// Moves: ${moveHistory.length}

class CubeSolver {
  constructor(dimension = 3) {
    this.size = dimension;
    this.initializeState();
  }
  
  initializeState() {
    this.faces = Array(6).fill(null).map((_, idx) =>
      Array(this.size).fill(null).map(() =>
        Array(this.size).fill(this.getColorCode(idx))
      )
    );
  }
  
  getColorCode(faceIndex) {
    const colors = ['R', 'O', 'Y', 'G', 'B', 'W'];
    return colors[faceIndex];
  }
  
  performMove(moveType, index, clockwise) {
    switch(moveType) {
      case 'horizontal': return this.rotateHorizontal(index, clockwise);
      case 'vertical': return this.rotateVertical(index, clockwise);
      case 'face': return this.rotateFace(index, clockwise);
    }
  }
  
  isSolved() {
    return this.faces.every(face => 
      face.flat().every(cell => cell === face[0][0])
    );
  }
}

// Export configuration
module.exports = { CubeSolver };`;

    const blob = new Blob([code], { type: 'text/javascript' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'rubiks-cube-solver.js';
    a.click();
  };

  const renderFace = (faceIdx) => {
    if (!cubeState) return null;
    
    return (
      <div className="inline-block m-2">
        <div className="text-xs font-semibold mb-1 text-center text-gray-600">
          {FACE_COLORS[faceIdx].name}
        </div>
        <div className="grid grid-cols-3 gap-0.5 border-2 border-gray-800 p-1 bg-gray-800 rounded">
          {cubeState[faceIdx].map((row, i) =>
            row.map((cell, j) => {
              const colorClass = Object.values(FACE_COLORS).find(
                fc => fc.initial === cell
              )?.color || 'bg-gray-300';
              
              return (
                <div
                  key={`${i}-${j}`}
                  className={`w-8 h-8 ${colorClass} border border-gray-700 rounded-sm shadow-inner`}
                />
              );
            })
          )}
        </div>
      </div>
    );
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-purple-50 to-blue-50 p-8">
      <div className="max-w-6xl mx-auto">
        <div className="bg-white rounded-2xl shadow-2xl p-8">
          <div className="flex items-center justify-between mb-8">
            <h1 className="text-4xl font-bold bg-gradient-to-r from-purple-600 to-blue-600 bg-clip-text text-transparent">
              Rubik's Cube Solver
            </h1>
            <div className="flex gap-2">
              <button
                onClick={() => setShowCode(!showCode)}
                className="flex items-center gap-2 px-4 py-2 bg-gray-100 hover:bg-gray-200 rounded-lg transition"
              >
                <Code size={18} />
                <span className="text-sm">Code</span>
              </button>
              <button
                onClick={exportCode}
                className="flex items-center gap-2 px-4 py-2 bg-gray-100 hover:bg-gray-200 rounded-lg transition"
              >
                <Download size={18} />
                <span className="text-sm">Export</span>
              </button>
            </div>
          </div>

          <div className="grid md:grid-cols-2 gap-8">
            <div>
              <div className="bg-gray-50 rounded-xl p-6">
                <h2 className="text-xl font-semibold mb-4 text-gray-700">Cube Visualization</h2>
                <div className="flex flex-col items-center">
                  <div>{renderFace(4)}</div>
                  <div className="flex">
                    {renderFace(3)}
                    {renderFace(0)}
                    {renderFace(2)}
                  </div>
                  <div>{renderFace(5)}</div>
                  <div>{renderFace(1)}</div>
                </div>
                
                {checkSolved(cubeState || []) && !isShuffled && (
                  <div className="mt-4 p-3 bg-green-100 border border-green-300 rounded-lg text-center">
                    <span className="text-green-700 font-semibold">✓ Cube is solved!</span>
                  </div>
                )}
              </div>

              <div className="mt-4 flex gap-3">
                <button
                  onClick={shuffleCube}
                  disabled={isSolving}
                  className="flex-1 flex items-center justify-center gap-2 px-6 py-3 bg-gradient-to-r from-purple-500 to-purple-600 text-white rounded-lg hover:from-purple-600 hover:to-purple-700 disabled:opacity-50 disabled:cursor-not-allowed transition shadow-lg"
                >
                  <Shuffle size={20} />
                  Shuffle
                </button>
                
                <button
                  onClick={solveCube}
                  disabled={!isShuffled || isSolving}
                  className="flex-1 flex items-center justify-center gap-2 px-6 py-3 bg-gradient-to-r from-blue-500 to-blue-600 text-white rounded-lg hover:from-blue-600 hover:to-blue-700 disabled:opacity-50 disabled:cursor-not-allowed transition shadow-lg"
                >
                  {isSolving ? <Zap size={20} className="animate-pulse" /> : <Play size={20} />}
                  {isSolving ? 'Solving...' : 'Solve'}
                </button>
                
                <button
                  onClick={reset}
                  className="px-6 py-3 bg-gray-500 text-white rounded-lg hover:bg-gray-600 transition shadow-lg"
                >
                  <RotateCw size={20} />
                </button>
              </div>
            </div>

            <div className="space-y-6">
              <div className="bg-gray-50 rounded-xl p-6">
                <h2 className="text-xl font-semibold mb-4 text-gray-700">Algorithm Selection</h2>
                <select
                  value={algorithmType}
                  onChange={(e) => setAlgorithmType(e.target.value)}
                  className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:ring-2 focus:ring-purple-500 focus:border-transparent"
                >
                  <option value="sequential">Sequential Search</option>
                  <option value="parallel-openmp">OpenMP Parallel</option>
                  <option value="parallel-mpi">MPI Distributed</option>
                  <option value="hybrid">Hybrid MPI + OpenMP</option>
                </select>
              </div>

              <div className="bg-gray-50 rounded-xl p-6">
                <h2 className="text-xl font-semibold mb-4 text-gray-700">Move History</h2>
                <div className="space-y-2 max-h-48 overflow-y-auto">
                  {moveHistory.length > 0 ? (
                    moveHistory.map((move, idx) => (
                      <div key={idx} className="flex items-center gap-2 p-2 bg-white rounded border border-gray-200">
                        <span className="text-xs font-mono text-gray-500">#{idx + 1}</span>
                        <span className="text-sm text-gray-700">{move}</span>
                      </div>
                    ))
                  ) : (
                    <p className="text-gray-400 text-sm italic">No moves yet. Shuffle the cube to start.</p>
                  )}
                </div>
              </div>

              {solution && (
                <div className="bg-gradient-to-r from-green-50 to-emerald-50 rounded-xl p-6 border-2 border-green-200">
                  <h2 className="text-xl font-semibold mb-3 text-green-800">Solution Path</h2>
                  <div className="text-sm text-gray-700 bg-white p-4 rounded-lg font-mono overflow-x-auto">
                    {solution}
                  </div>
                </div>
              )}

              {showCode && (
                <div className="bg-gray-900 rounded-xl p-6 text-white">
                  <h2 className="text-xl font-semibold mb-3">Code Preview</h2>
                  <pre className="text-xs overflow-x-auto">
                    <code>{`// Solver Implementation
const solver = {
  algorithm: '${algorithmType}',
  maxDepth: 13,
  moves: ${moveHistory.length}
};`}</code>
                  </pre>
                </div>
              )}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default App;