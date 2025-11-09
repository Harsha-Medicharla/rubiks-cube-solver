import React, { useState, useEffect } from 'react';
import { Shuffle, Play, RotateCw, BookOpen, TrendingUp, Clock } from 'lucide-react';

const App = () => {
  const [cubeState, setCubeState] = useState(null);
  const [results, setResults] = useState([]);
  const [isSolving, setIsSolving] = useState(false);
  const [isScrambling, setIsScrambling] = useState(false);
  const [serverStatus, setServerStatus] = useState('checking...');
  const [scrambleMoves, setScrambleMoves] = useState(7);
  const [showManual, setShowManual] = useState(false);

  const COLOR_MAP = {
    'W': 'bg-white border-gray-400',
    'Y': 'bg-yellow-400',
    'G': 'bg-green-500',
    'B': 'bg-blue-500',
    'R': 'bg-red-500',
    'O': 'bg-orange-500'
  };

  const FACE_NAMES = {
    'U': 'Up', 'D': 'Down', 'F': 'Front', 'B': 'Back', 'L': 'Left', 'R': 'Right'
  };

  const MOVE_GUIDE = {
    'U': 'Up face clockwise', "U'": 'Up face counter-clockwise',
    'D': 'Down face clockwise', "D'": 'Down face counter-clockwise',
    'F': 'Front face clockwise', "F'": 'Front face counter-clockwise',
    'B': 'Back face clockwise', "B'": 'Back face counter-clockwise',
    'L': 'Left face clockwise', "L'": 'Left face counter-clockwise',
    'R': 'Right face clockwise', "R'": 'Right face counter-clockwise'
  };

  useEffect(() => {
    checkBackend();
    loadCube();
  }, []);

  const checkBackend = async () => {
    try {
      const response = await fetch('http://localhost:8080/status');
      setServerStatus(response.ok ? 'connected' : 'error');
    } catch (err) {
      setServerStatus('disconnected');
    }
  };

  const loadCube = async () => {
    try {
      const response = await fetch('http://localhost:8080/cube');
      const data = await response.json();
      setCubeState(data);
    } catch (err) {
      console.error('Failed to load cube:', err);
    }
  };

  const handleScramble = async () => {
    setIsScrambling(true);
    setResults([]);
    try {
      const response = await fetch('http://localhost:8080/cube/scramble', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ moves: scrambleMoves })
      });
      const data = await response.json();
      setCubeState(data);
    } catch (err) {
      alert('Backend not responding!');
    } finally {
      setIsScrambling(false);
    }
  };

  const handleSolve = async () => {
    setIsSolving(true);
    setResults([]);
    
    try {
      const response = await fetch('http://localhost:8080/cube/solve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ maxDepth: 15 })
      });
      
      const data = await response.json();
      
      if (data.results) {
        setResults(data.results);
        setCubeState(data.cube);
      } else {
        alert('No solution found');
      }
    } catch (err) {
      alert('Solve failed: ' + err.message);
    } finally {
      setIsSolving(false);
    }
  };

  const handleReset = async () => {
    setResults([]);
    try {
      const response = await fetch('http://localhost:8080/cube/reset', { method: 'POST' });
      const data = await response.json();
      setCubeState(data);
    } catch (err) {
      console.error('Reset failed:', err);
    }
  };

  const handleApplySolution = async (moves) => {
    try {
      for (const move of moves) {
        await fetch('http://localhost:8080/cube/move', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ move })
        });
        await new Promise(resolve => setTimeout(resolve, 300));
        await loadCube();
      }
    } catch (err) {
      console.error('Failed to apply moves:', err);
    }
  };

  const renderFace = (faceName, faceData) => {
    if (!faceData) return null;
    return (
      <div className="inline-block m-2">
        <div className="text-xs font-bold mb-1 text-center text-gray-700">
          {FACE_NAMES[faceName]}
        </div>
        <div className="grid grid-cols-3 gap-1 border-4 border-gray-800 p-1 bg-gray-800 rounded-lg">
          {faceData.map((color, idx) => (
            <div
              key={idx}
              className={`w-8 h-8 ${COLOR_MAP[color]} border-2 border-gray-600 rounded flex items-center justify-center text-xs font-bold`}
            >
              {color}
            </div>
          ))}
        </div>
      </div>
    );
  };

  if (!cubeState) {
    return <div className="min-h-screen bg-gradient-to-br from-purple-50 to-blue-50 flex items-center justify-center">
      <div className="text-2xl font-bold">Loading...</div>
    </div>;
  }

  return (
    <div className="min-h-screen bg-gradient-to-br from-purple-50 to-blue-50 p-8">
      <div className="max-w-7xl mx-auto">
        <div className="bg-white rounded-2xl shadow-2xl p-8">
          
          {/* Header */}
          <div className="flex items-center justify-between mb-6">
            <div>
              <h1 className="text-4xl font-bold bg-gradient-to-r from-purple-600 to-blue-600 bg-clip-text text-transparent">
                Rubik's Cube Solver - IDA* Parallel Computing
              </h1>
              <p className="text-sm text-gray-600 mt-2">
                Backend: <span className={`font-bold ${serverStatus === 'connected' ? 'text-green-600' : 'text-red-600'}`}>
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

          <div className="grid lg:grid-cols-2 gap-8">
            
            {/* Left: Cube */}
            <div>
              <div className="bg-gray-50 rounded-xl p-6">
                <div className="flex justify-between items-center mb-4">
                  <h2 className="text-xl font-bold text-gray-700">Cube State</h2>
                  <button
                    onClick={() => setShowManual(!showManual)}
                    className="flex items-center gap-2 px-4 py-2 bg-blue-500 text-white rounded-lg hover:bg-blue-600"
                  >
                    <BookOpen size={20} />
                    Move Guide
                  </button>
                </div>
                
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
                  <span className="font-bold text-gray-700">Scramble Depth:</span>
                  <span className="text-blue-600 font-bold text-lg">{scrambleMoves} moves</span>
                </label>
                <input
                  type="range"
                  min="3"
                  max="12"
                  value={scrambleMoves}
                  onChange={(e) => setScrambleMoves(parseInt(e.target.value))}
                  className="w-full h-2 bg-gray-200 rounded-lg cursor-pointer accent-purple-600"
                  disabled={isSolving || isScrambling}
                />
                <div className="flex justify-between text-xs text-gray-500 mt-1">
                  <span>Easy (3)</span>
                  <span>Medium (7)</span>
                  <span>Hard (12)</span>
                </div>
              </div>

              {/* Control Buttons */}
              <div className="mt-6 flex gap-3">
                <button
                  onClick={handleScramble}
                  disabled={isScrambling || isSolving}
                  className="flex-1 flex items-center justify-center gap-2 px-6 py-4 bg-gradient-to-r from-purple-500 to-purple-600 text-white rounded-xl hover:from-purple-600 hover:to-purple-700 disabled:opacity-50 shadow-lg font-bold"
                >
                  <Shuffle size={24} />
                  {isScrambling ? 'Scrambling...' : 'Scramble'}
                </button>
                
                <button
                  onClick={handleSolve}
                  disabled={cubeState.isSolved || isSolving || isScrambling}
                  className="flex-1 flex items-center justify-center gap-2 px-6 py-4 bg-gradient-to-r from-blue-500 to-blue-600 text-white rounded-xl hover:from-blue-600 hover:to-blue-700 disabled:opacity-50 shadow-lg font-bold"
                >
                  <Play size={24} />
                  {isSolving ? 'Solving All...' : 'Solve with All 4'}
                </button>
                
                <button
                  onClick={handleReset}
                  disabled={isSolving || isScrambling}
                  className="px-6 py-4 bg-gray-500 text-white rounded-xl hover:bg-gray-600 shadow-lg disabled:opacity-50"
                >
                  <RotateCw size={24} />
                </button>
              </div>
            </div>

            {/* Right: Results */}
            <div className="space-y-6">
              
              {/* Move Guide */}
              {showManual && (
                <div className="bg-gradient-to-r from-blue-50 to-indigo-50 rounded-xl p-6 border-2 border-blue-200">
                  <h3 className="font-bold text-blue-900 mb-3 flex items-center gap-2">
                    <BookOpen size={20} />
                    Move Notation Guide
                  </h3>
                  <div className="grid grid-cols-2 gap-2 text-sm">
                    {Object.entries(MOVE_GUIDE).map(([move, desc]) => (
                      <div key={move} className="flex items-center gap-2 bg-white p-2 rounded">
                        <span className="font-mono font-bold text-purple-600 min-w-[30px]">{move}</span>
                        <span className="text-gray-700">{desc}</span>
                      </div>
                    ))}
                  </div>
                  <div className="mt-3 text-xs text-blue-800 bg-blue-100 p-3 rounded">
                    <strong>Key:</strong> Clockwise = 90° CW, Counter-clockwise (') = 90° CCW
                  </div>
                </div>
              )}

              {/* Results Comparison */}
              {results.length > 0 && (
                <div className="bg-gradient-to-r from-green-50 to-emerald-50 rounded-xl p-6 border-2 border-green-300">
                  <h2 className="text-xl font-bold mb-4 text-green-800 flex items-center gap-2">
                    <TrendingUp size={24} />
                    Algorithm Comparison
                  </h2>
                  
                  {/* Comparison Table */}
                  <div className="bg-white rounded-lg overflow-hidden mb-4">
                    <table className="w-full text-sm">
                      <thead className="bg-gray-100">
                        <tr>
                          <th className="text-left p-3 font-bold">Algorithm</th>
                          <th className="text-center p-3 font-bold">Time (s)</th>
                          <th className="text-center p-3 font-bold">Moves</th>
                          <th className="text-center p-3 font-bold">Speedup</th>
                        </tr>
                      </thead>
                      <tbody>
                        {results.map((result, idx) => (
                          <tr key={idx} className={idx % 2 === 0 ? 'bg-gray-50' : 'bg-white'}>
                            <td className="p-3 font-bold text-purple-600">{result.name}</td>
                            <td className="text-center p-3">
                              {result.success ? (
                                <span className="flex items-center justify-center gap-1">
                                  <Clock size={16} />
                                  {result.time.toFixed(4)}
                                </span>
                              ) : 'TIMEOUT'}
                            </td>
                            <td className="text-center p-3 font-bold text-green-600">
                              {result.success ? result.moves : '-'}
                            </td>
                            <td className="text-center p-3 font-bold text-blue-600">
                              {result.success ? `${result.speedup.toFixed(2)}x` : '-'}
                            </td>
                          </tr>
                        ))}
                      </tbody>
                    </table>
                  </div>

                  {/* Solution Moves by Algorithm */}
                  {results.filter(r => r.success).map((result, idx) => (
                    <div key={idx} className="mb-4 bg-white rounded-lg p-4 border border-gray-200">
                      <div className="flex justify-between items-center mb-2">
                        <h3 className="font-bold text-purple-700">{result.name}</h3>
                        <button
                          onClick={() => handleApplySolution(result.solution)}
                          className="px-3 py-1 bg-green-500 text-white text-xs rounded hover:bg-green-600"
                        >
                          Apply Solution
                        </button>
                      </div>
                      <div className="flex flex-wrap gap-1">
                        {result.solution.map((move, i) => (
                          <span key={i} className="px-2 py-1 bg-purple-100 border border-purple-300 rounded font-mono text-xs">
                            {move}
                          </span>
                        ))}
                      </div>
                    </div>
                  ))}
                </div>
              )}

              {/* Instructions */}
              <div className="bg-blue-50 rounded-xl p-6 border border-blue-200">
                <h3 className="font-bold text-blue-900 mb-2">How to Use:</h3>
                <ol className="text-sm text-blue-800 space-y-1 list-decimal list-inside">
                  <li>Set scramble depth (3-12 moves)</li>
                  <li>Click <strong>Scramble</strong></li>
                  <li>Click <strong>Solve with All 4</strong> to run all algorithms</li>
                  <li>Compare execution times and speedups</li>
                  <li>Click <strong>Apply Solution</strong> to execute moves</li>
                </ol>
                <div className="mt-3 p-3 bg-blue-100 rounded text-xs">
                  <strong>💡 Tip:</strong> IDA* is much faster than DFS! Try scramble depth 7-10 for best comparison.
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};

export default App;