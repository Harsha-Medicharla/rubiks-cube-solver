#include "sequential_solver.hpp"
#include <iostream>
#include <chrono>

std::vector<std::string> SequentialSolver::solve(RubiksCube& cube, int maxDepth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (cube.isSolved()) {
        solveTime_ = 0.0;
        return {};
    }
    
    solution_.clear();
    currentPath_.clear();
    maxDepth_ = maxDepth;
    nodesExplored_ = 0;
    
    std::cout << "Starting Sequential DFS search (max depth: " << maxDepth << ")..." << std::endl;
    
    bool found = search(cube, 0, "");
    
    auto endTime = std::chrono::high_resolution_clock::now();
    solveTime_ = std::chrono::duration<double>(endTime - startTime).count();
    
    if (found) {
        std::cout << "Solution found! Moves: " << solution_.size() 
                  << ", Nodes: " << nodesExplored_ 
                  << ", Time: " << solveTime_ << "s" << std::endl;
        return solution_;
    }
    
    std::cout << "No solution found within depth " << maxDepth << std::endl;
    return {};
}

bool SequentialSolver::search(RubiksCube& cube, int depth, const std::string& lastMove) {
    nodesExplored_++;
    
    if (cube.isSolved()) {
        solution_ = currentPath_;
        return true;
    }
    
    if (depth >= maxDepth_) {
        return false;
    }
    
    auto moves = RubiksCube::getBasicMoves();
    
    for (const auto& move : moves) {
        if (isRedundantMove(lastMove, move)) {
            continue;
        }
        
        cube.applyMove(move);
        currentPath_.push_back(move);
        
        if (search(cube, depth + 1, move)) {
            return true;
        }
        
        currentPath_.pop_back();
        cube.applyMove(cube.getInverseMove(move));
    }
    
    return false;
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

