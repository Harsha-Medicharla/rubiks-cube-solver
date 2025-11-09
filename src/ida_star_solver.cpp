#include "ida_star_solver.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>

IDAStarSolver::IDAStarSolver() : threshold_(0) {}

std::vector<std::string> IDAStarSolver::solve(RubiksCube& cube, int maxDepth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (cube.isSolved()) {
        return {};
    }
    
    solution_.clear();
    currentPath_.clear();
    nodesExplored_ = 0;
    
    // Initial threshold is the heuristic value
    threshold_ = heuristic(cube);
    
    std::cout << "Starting IDA* search with initial threshold: " << threshold_ << std::endl;
    
    // Iterative deepening
    while (threshold_ <= maxDepth) {
        int t = search(cube, 0, threshold_, "");
        
        if (t == -1) {
            // Solution found
            auto endTime = std::chrono::high_resolution_clock::now();
            solveTime_ = std::chrono::duration<double>(endTime - startTime).count();
            
            std::cout << "Solution found! Moves: " << solution_.size() 
                      << ", Nodes: " << nodesExplored_ 
                      << ", Time: " << solveTime_ << "s" << std::endl;
            
            return solution_;
        }
        
        if (t == std::numeric_limits<int>::max()) {
            // No solution exists
            break;
        }
        
        threshold_ = t;
        std::cout << "Increasing threshold to: " << threshold_ << std::endl;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    solveTime_ = std::chrono::duration<double>(endTime - startTime).count();
    
    std::cout << "No solution found within depth " << maxDepth << std::endl;
    return {};
}

int IDAStarSolver::search(RubiksCube& cube, int g, int bound, const std::string& lastMove) {
    nodesExplored_++;
    
    int f = g + heuristic(cube);
    
    if (f > bound) {
        return f;
    }
    
    if (cube.isSolved()) {
        solution_ = currentPath_;
        return -1;
    }
    
    int min = std::numeric_limits<int>::max();
    
    // Try all possible moves
    auto moves = RubiksCube::getBasicMoves();
    
    for (const auto& move : moves) {
        // Skip redundant moves
        if (isRedundantMove(lastMove, move)) {
            continue;
        }
        
        // Apply move
        cube.applyMove(move);
        currentPath_.push_back(move);
        
        int t = search(cube, g + 1, bound, move);
        
        if (t == -1) {
            return -1; // Solution found
        }
        
        if (t < min) {
            min = t;
        }
        
        // Revert move
        currentPath_.pop_back();
        cube.applyMove(cube.getInverseMove(move));
    }
    
    return min;
}

int IDAStarSolver::heuristic(const RubiksCube& cube) const {
    // Use Manhattan distance as heuristic
    return cube.getManhattanDistance();
}

bool IDAStarSolver::isRedundantMove(const std::string& lastMove, const std::string& nextMove) const {
    if (lastMove.empty()) {
        return false;
    }
    
    // Don't do opposite moves in sequence (e.g., U followed by U')
    char lastFace = lastMove[0];
    char nextFace = nextMove[0];
    
    if (lastFace == nextFace) {
        return true; // Same face - redundant
    }
    
    // Don't alternate between opposite faces
    if ((lastFace == 'U' && nextFace == 'D') || (lastFace == 'D' && nextFace == 'U')) {
        return true;
    }
    if ((lastFace == 'L' && nextFace == 'R') || (lastFace == 'R' && nextFace == 'L')) {
        return true;
    }
    if ((lastFace == 'F' && nextFace == 'B') || (lastFace == 'B' && nextFace == 'F')) {
        return true;
    }
    
    return false;
}