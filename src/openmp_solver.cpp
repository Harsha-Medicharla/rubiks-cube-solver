#include "openmp_solver.hpp"
#include <iostream>
#include <chrono>

OpenMPSolver::OpenMPSolver(int numThreads) 
    : numThreads_(numThreads), solutionFound_(false) {
    omp_set_num_threads(numThreads_);
}

std::vector<std::string> OpenMPSolver::solve(RubiksCube& cube, int maxDepth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (cube.isSolved()) {
        solveTime_ = 0.0;
        std::cout << "Cube already solved!" << std::endl;
        return {};
    }
    
    solution_.clear();
    maxDepth_ = maxDepth;
    nodesExplored_ = 0;
    solutionFound_ = false;
    
    std::cout << "=== OpenMP DFS Search ===" << std::endl;
    std::cout << "Threads: " << numThreads_ << std::endl;
    std::cout << "Max depth: " << maxDepth << std::endl;
    
    // Iterative deepening for better results
    for (int depth = 1; depth <= maxDepth && !solutionFound_; depth++) {
        std::cout << "\nTrying depth " << depth << "..." << std::endl;
        maxDepth_ = depth;
        nodesExplored_ = 0;
        
        auto moves = RubiksCube::getBasicMoves();
        
        // Parallel search at first level
        #pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < moves.size(); ++i) {
            if (solutionFound_) continue;
            
            RubiksCube localCube = cube;
            localCube.applyMove(moves[i]);
            
            std::vector<std::string> localPath = {moves[i]};
            
            if (search(localCube, 1, moves[i], localPath)) {
                #pragma omp critical
                {
                    if (!solutionFound_) {
                        solution_ = localPath;
                        solutionFound_ = true;
                        std::cout << "  Thread found solution at depth " << depth << "!" << std::endl;
                    }
                }
            }
        }
        
        std::cout << "  Depth " << depth << ": " 
                  << nodesExplored_ << " nodes explored"
                  << (solutionFound_ ? " - SOLUTION FOUND!" : " - no solution") << std::endl;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    solveTime_ = std::chrono::duration<double>(endTime - startTime).count();
    
    std::cout << "\n=== Search Complete ===" << std::endl;
    if (solutionFound_) {
        std::cout << "✓ Solution found!" << std::endl;
        std::cout << "  Moves: " << solution_.size() << std::endl;
        std::cout << "  Total nodes: " << nodesExplored_ << std::endl;
        std::cout << "  Time: " << solveTime_ << "s" << std::endl;
        std::cout << "  Threads: " << numThreads_ << std::endl;
        std::cout << "  Solution: ";
        for (const auto& move : solution_) {
            std::cout << move << " ";
        }
        std::cout << std::endl;
        return solution_;
    }
    
    std::cout << "✗ No solution found within depth " << maxDepth << std::endl;
    std::cout << "  Total nodes: " << nodesExplored_ << std::endl;
    std::cout << "  Time: " << solveTime_ << "s" << std::endl;
    return {};
}

bool OpenMPSolver::searchParallel(RubiksCube& cube, const std::string& firstMove, int depth) {
    std::vector<std::string> path = {firstMove};
    return search(cube, depth, firstMove, path);
}

bool OpenMPSolver::search(RubiksCube& cube, int depth, const std::string& lastMove,
                          std::vector<std::string>& path) {
    #pragma omp atomic
    nodesExplored_++;
    
    // Progress reporting (thread-safe)
    if (nodesExplored_ % 50000 == 0) {
        #pragma omp critical
        {
            std::cout << "    Progress: " << nodesExplored_ << " nodes" << std::endl;
        }
    }
    
    if (solutionFound_) return false;
    
    if (cube.isSolved()) {
        #pragma omp critical
        {
            if (!solutionFound_) {
                solution_ = path;
                solutionFound_ = true;
            }
        }
        return true;
    }
    
    if (depth >= maxDepth_) {
        return false;
    }
    
    auto moves = RubiksCube::getBasicMoves();
    
    for (const auto& move : moves) {
        if (solutionFound_) return false;
        
        if (isRedundantMove(lastMove, move)) {
            continue;
        }
        
        cube.applyMove(move);
        path.push_back(move);
        
        if (search(cube, depth + 1, move, path)) {
            return true;
        }
        
        path.pop_back();
        cube.applyMove(cube.getInverseMove(move));
    }
    
    return false;
}

bool OpenMPSolver::isRedundantMove(const std::string& lastMove, const std::string& nextMove) const {
    if (lastMove.empty()) return false;
    
    char lastFace = lastMove[0];
    char nextFace = nextMove[0];
    
    if (lastFace == nextFace) return true;
    
    if ((lastFace == 'U' && nextFace == 'D') || (lastFace == 'D' && nextFace == 'U')) return true;
    if ((lastFace == 'L' && nextFace == 'R') || (lastFace == 'R' && nextFace == 'L')) return true;
    if ((lastFace == 'F' && nextFace == 'B') || (lastFace == 'B' && nextFace == 'F')) return true;
    
    return false;
}