#include "sequential_solver.hpp"
#include <iostream>
#include <chrono>

std::vector<std::string> SequentialSolver::solve(RubiksCube& cube, int maxDepth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (cube.isSolved()) {
        solveTime_ = 0.0;
        std::cout << "Cube already solved!" << std::endl;
        return {};
    }
    
    solution_.clear();
    currentPath_.clear();
    maxDepth_ = maxDepth;
    nodesExplored_ = 0;
    
    std::cout << "=== Sequential DFS Search ===" << std::endl;
    std::cout << "Max depth: " << maxDepth << std::endl;
    std::cout << "Starting iterative deepening search..." << std::endl;
    
    // Time limit: 60 seconds
    const double TIME_LIMIT = 60.0;
    
    bool found = false;
    // Iterative deepening: try depths 1, 2, 3... up to maxDepth
    for (int depth = 1; depth <= maxDepth && !found; depth++) {
        std::cout << "\nSearching depth " << depth << "..." << std::endl;
        maxDepth_ = depth;
        int depthNodes = 0;
        currentPath_.clear();
        
        auto depthStartTime = std::chrono::high_resolution_clock::now();
        
        found = search(cube, 0, "");
        
        auto depthEndTime = std::chrono::high_resolution_clock::now();
        double depthTime = std::chrono::duration<double>(depthEndTime - depthStartTime).count();
        depthNodes = nodesExplored_;
        
        std::cout << "  Depth " << depth << ": " 
                  << depthNodes << " nodes, " 
                  << depthTime << "s"
                  << (found ? " - SOLUTION FOUND!" : "") << std::endl;
        
        // Check time limit
        auto currentTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(currentTime - startTime).count();
        if (elapsed > TIME_LIMIT && !found) {
            std::cout << "\n⏱️  Time limit reached (" << TIME_LIMIT << "s)" << std::endl;
            std::cout << "Consider scrambling with fewer moves (3-7 recommended)" << std::endl;
            break;
        }
        
        // Warn if next depth will likely be too slow
        if (depth >= 7 && !found) {
            std::cout << "  ⚠️  Warning: Deeper searches may take very long..." << std::endl;
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    solveTime_ = std::chrono::duration<double>(endTime - startTime).count();
    
    std::cout << "\n=== Search Complete ===" << std::endl;
    if (found) {
        std::cout << "✓ Solution found!" << std::endl;
        std::cout << "  Moves: " << solution_.size() << std::endl;
        std::cout << "  Total nodes explored: " << nodesExplored_ << std::endl;
        std::cout << "  Time: " << solveTime_ << "s" << std::endl;
        std::cout << "  Solution: ";
        for (const auto& move : solution_) {
            std::cout << move << " ";
        }
        std::cout << std::endl;
        return solution_;
    } else {
        std::cout << "✗ No solution found within depth " << maxDepth << std::endl;
        std::cout << "  Total nodes explored: " << nodesExplored_ << std::endl;
        std::cout << "  Time: " << solveTime_ << "s" << std::endl;
        std::cout << "\n💡 Tips:" << std::endl;
        std::cout << "  - Use scramble depth 3-7 for faster solving" << std::endl;
        std::cout << "  - Try OpenMP solver for better performance" << std::endl;
    }
    
    return {};
}

bool SequentialSolver::search(RubiksCube& cube, int depth, const std::string& lastMove) {
    nodesExplored_++;
    
    // Progress reporting - much less frequent for depth > 8
    int progressInterval = (depth >= 8) ? 1000000 : 100000;
    if (nodesExplored_ % progressInterval == 0) {
        std::cout << "    Progress: " << (nodesExplored_ / 1000000.0) << "M nodes" << std::endl;
    }
    
    if (cube.isSolved()) {
        solution_ = currentPath_;
        return true;
    }
    
    if (depth >= maxDepth_) {
        return false;
    }
    
    // Only explore basic moves (no 2-moves) for efficiency
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
    
    // Don't repeat same face
    if (lastFace == nextFace) return true;
    
    // Don't do opposite faces in sequence (they commute)
    if ((lastFace == 'U' && nextFace == 'D') || (lastFace == 'D' && nextFace == 'U')) return true;
    if ((lastFace == 'L' && nextFace == 'R') || (lastFace == 'R' && nextFace == 'L')) return true;
    if ((lastFace == 'F' && nextFace == 'B') || (lastFace == 'B' && nextFace == 'F')) return true;
    
    return false;
}