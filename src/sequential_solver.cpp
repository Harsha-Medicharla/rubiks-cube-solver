// src/sequential_solver.cpp - IDA* implementation
#include "sequential_solver.hpp"
#include <iostream>
#include <chrono>
#include <limits>

// Manhattan distance heuristic
int SequentialSolver::heuristic(const RubiksCube& cube) const {
    return cube.getManhattanDistance();
}

std::vector<std::string> SequentialSolver::solve(RubiksCube& cube, int maxDepth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (cube.isSolved()) {
        solveTime_ = 0.0;
        std::cout << "Cube already solved!" << std::endl;
        return {};
    }
    
    solution_.clear();
    currentPath_.clear();
    nodesExplored_ = 0;
    
    std::cout << "=== Sequential IDA* Search ===" << std::endl;
    std::cout << "Max depth: " << maxDepth << std::endl;
    
    const double TIME_LIMIT = 120.0; // 2 minutes
    
    // IDA*: iteratively increase threshold
    int threshold = heuristic(cube);
    bool found = false;
    
    while (!found && threshold <= maxDepth) {
        std::cout << "Searching with threshold " << threshold << "..." << std::endl;
        
        currentPath_.clear();
        int temp = idaSearch(cube, 0, threshold, "", TIME_LIMIT, startTime);
        
        if (temp == -1) {
            found = true;
            break;
        }
        
        if (temp == std::numeric_limits<int>::max()) {
            std::cout << "No solution exists within depth limit" << std::endl;
            break;
        }
        
        // Check time limit
        auto currentTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(currentTime - startTime).count();
        if (elapsed > TIME_LIMIT) {
            std::cout << "Time limit reached" << std::endl;
            break;
        }
        
        threshold = temp;
        std::cout << "  New threshold: " << threshold << ", Nodes: " << nodesExplored_ << std::endl;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    solveTime_ = std::chrono::duration<double>(endTime - startTime).count();
    
    std::cout << "\n=== Search Complete ===" << std::endl;
    if (found) {
        std::cout << "✓ Solution found!" << std::endl;
        std::cout << "  Moves: " << solution_.size() << std::endl;
        std::cout << "  Nodes: " << nodesExplored_ << std::endl;
        std::cout << "  Time: " << solveTime_ << "s" << std::endl;
        return solution_;
    } else {
        std::cout << "✗ No solution found (timeout or invalid scramble)" << std::endl;
        std::cout << "  Nodes: " << nodesExplored_ << std::endl;
        std::cout << "  Time: " << solveTime_ << "s" << std::endl;
    }
    
    return {};
}

int SequentialSolver::idaSearch(RubiksCube& cube, int g, int threshold, 
                                const std::string& lastMove, double timeLimit,
                                std::chrono::high_resolution_clock::time_point startTime) {
    nodesExplored_++;
    
    // Check time limit
    if (nodesExplored_ % 10000 == 0) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(currentTime - startTime).count();
        if (elapsed > timeLimit) {
            return std::numeric_limits<int>::max();
        }
    }
    
    int h = heuristic(cube);
    int f = g + h;
    
    if (f > threshold) {
        return f;
    }
    
    if (cube.isSolved()) {
        solution_ = currentPath_;
        return -1;
    }
    
    int min = std::numeric_limits<int>::max();
    auto moves = RubiksCube::getBasicMoves();
    
    for (const auto& move : moves) {
        if (isRedundantMove(lastMove, move)) {
            continue;
        }
        
        cube.applyMove(move);
        currentPath_.push_back(move);
        
        int temp = idaSearch(cube, g + 1, threshold, move, timeLimit, startTime);
        
        if (temp == -1) {
            return -1;
        }
        
        if (temp < min) {
            min = temp;
        }
        
        currentPath_.pop_back();
        cube.applyMove(cube.getInverseMove(move));
    }
    
    return min;
}

bool SequentialSolver::isRedundantMove(const std::string& lastMove, const std::string& nextMove) const {
    if (lastMove.empty()) return false;
    
    char lastFace = lastMove[0];
    char nextFace = nextMove[0];
    
    if (lastFace == nextFace) return true;
    
    if ((lastFace == 'U' && nextFace == 'D') || (lastFace == 'D' && nextFace == 'U')) return true;
    if ((lastFace == 'L' && nextFace == 'R') || (lastFace == 'R' && nextFace == 'L')) return true;
    if ((lastFace == 'F' && nextFace == 'B') || (lastFace == 'B' && nextFace == 'F')) return true;
    
    return false;
}