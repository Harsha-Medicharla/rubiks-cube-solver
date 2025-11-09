#include "openmp_solver.hpp"
#include <iostream>
#include <chrono>
#include <limits>

OpenMPSolver::OpenMPSolver(int numThreads) 
    : numThreads_(numThreads), solutionFound_(false) {
    omp_set_num_threads(numThreads_);
}

int OpenMPSolver::heuristic(const RubiksCube& cube) const {
    return cube.getManhattanDistance();
}

std::vector<std::string> OpenMPSolver::solve(RubiksCube& cube, int maxDepth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (cube.isSolved()) {
        solveTime_ = 0.0;
        std::cout << "Cube already solved!" << std::endl;
        return {};
    }
    
    solution_.clear();
    nodesExplored_ = 0;
    solutionFound_ = false;
    
    std::cout << "=== OpenMP IDA* Search ===" << std::endl;
    std::cout << "Threads: " << numThreads_ << std::endl;
    std::cout << "Max depth: " << maxDepth << std::endl;
    
    const double TIME_LIMIT = 120.0;
    
    int threshold = heuristic(cube);
    bool found = false;
    
    while (!found && threshold <= maxDepth && !solutionFound_) {
        std::cout << "Searching with threshold " << threshold << "..." << std::endl;
        
        int minNext = std::numeric_limits<int>::max();
        auto moves = RubiksCube::getBasicMoves();
        
        #pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < moves.size(); ++i) {
            if (solutionFound_) continue;
            
            RubiksCube localCube = cube;
            localCube.applyMove(moves[i]);
            
            std::vector<std::string> localPath = {moves[i]};
            
            int temp = idaSearchParallel(localCube, 1, threshold, moves[i], 
                                        localPath, TIME_LIMIT, startTime);
            
            if (temp == -1) {
                #pragma omp critical
                {
                    if (!solutionFound_) {
                        solution_ = localPath;
                        solutionFound_ = true;
                        found = true;
                    }
                }
            } else if (temp < minNext) {
                #pragma omp critical
                {
                    if (temp < minNext) {
                        minNext = temp;
                    }
                }
            }
        }
        
        if (found) break;
        
        auto currentTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(currentTime - startTime).count();
        if (elapsed > TIME_LIMIT) {
            std::cout << "Time limit reached" << std::endl;
            break;
        }
        
        if (minNext == std::numeric_limits<int>::max()) {
            break;
        }
        
        threshold = minNext;
        std::cout << "  New threshold: " << threshold << ", Nodes: " << nodesExplored_ << std::endl;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    solveTime_ = std::chrono::duration<double>(endTime - startTime).count();
    
    std::cout << "\n=== Search Complete ===" << std::endl;
    if (solutionFound_) {
        std::cout << "✓ Solution found!" << std::endl;
        std::cout << "  Moves: " << solution_.size() << std::endl;
        std::cout << "  Nodes: " << nodesExplored_ << std::endl;
        std::cout << "  Time: " << solveTime_ << "s" << std::endl;
        std::cout << "  Threads: " << numThreads_ << std::endl;
        return solution_;
    }
    
    std::cout << "✗ No solution found" << std::endl;
    std::cout << "  Nodes: " << nodesExplored_ << std::endl;
    std::cout << "  Time: " << solveTime_ << "s" << std::endl;
    return {};
}

int OpenMPSolver::idaSearchParallel(RubiksCube& cube, int g, int threshold,
                                   const std::string& lastMove,
                                   std::vector<std::string>& path,
                                   double timeLimit,
                                   std::chrono::high_resolution_clock::time_point startTime) {
    #pragma omp atomic
    nodesExplored_++;
    
    if (solutionFound_) return std::numeric_limits<int>::max();
    
    int h = heuristic(cube);
    int f = g + h;
    
    if (f > threshold) {
        return f;
    }
    
    if (cube.isSolved()) {
        return -1;
    }
    
    int min = std::numeric_limits<int>::max();
    auto moves = RubiksCube::getBasicMoves();
    
    for (const auto& move : moves) {
        if (solutionFound_) return std::numeric_limits<int>::max();
        
        if (isRedundantMove(lastMove, move)) {
            continue;
        }
        
        cube.applyMove(move);
        path.push_back(move);
        
        int temp = idaSearchParallel(cube, g + 1, threshold, move, path, timeLimit, startTime);
        
        if (temp == -1) {
            return -1;
        }
        
        if (temp < min) {
            min = temp;
        }
        
        path.pop_back();
        cube.applyMove(cube.getInverseMove(move));
    }
    
    return min;
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