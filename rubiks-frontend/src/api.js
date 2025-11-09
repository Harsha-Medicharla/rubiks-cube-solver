const API_URL = 'http://localhost:8080';

export const cubeAPI = {
  getCube: async () => {
    const res = await fetch(`${API_URL}/cube`);
    return res.json();
  },
  
  scramble: async (moves = 20) => {
    const res = await fetch(`${API_URL}/cube/scramble`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ moves })
    });
    return res.json();
  },
  
  solve: async () => {
    const res = await fetch(`${API_URL}/cube/solve`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ maxDepth: 20 })
    });
    return res.json();
  },
  
  applyMove: async (move) => {
    const res = await fetch(`${API_URL}/cube/move`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ move })
    });
    return res.json();
  }
};