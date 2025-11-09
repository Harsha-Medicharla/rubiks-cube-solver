#include "mpi_solver.hpp"
#include <iostream>
#include <chrono>
#include <algorithm>

bool MPISolver::initialized_ = false;

void MPISolver::Initialize(int* argc, char*** argv) {
    if (!initialized_) {
        MPI_Init(argc, argv);
        initialized_ = true;
    }
}

void MPISolver::Finalize() {
    if (initialized_) {
        MPI_Finalize();
        initialized_ = false;
    }
}

MPISolver::MPISolver() {
    if (!initialized_) {
        throw std::runtime_error("MPI not initialized. Call MPISolver::Initialize() first.");
    }
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &size_);
}

MPISolver::~MPISolver() {
    // Don't finalize here - let user control it
}

std::vector<std::string> MPISolver::solve(RubiksCube& cube, int maxDepth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (cube.isSolved()) {
        return {};
    }
    
    solution_.clear();
    maxDepth_ = maxDepth;
    nodesExplored_ = 0;
    
    if (rank_ == 0) {
        std::cout << "Starting MPI DFS search (processes: " << size_ 
                  << ", max depth: " << maxDepth << ")..." << std::endl;
    }
    
    // Get all possible first moves
    auto moves = RubiksCube::getBasicMoves();
    
    // Distribute moves among processes
    std::vector<std::string> localSolution;
    int bestLength = maxDepth + 1;
    
    for (size_t i = rank_; i < moves.size(); i += size_) {
        RubiksCube localCube = cube;
        localCube.applyMove(moves[i]);
        
        std::vector<std::string> localPath = {moves[i]};
        
        if (search(localCube, 1, moves[i], localPath)) {
            if (localPath.size() < (size_t)bestLength) {
                localSolution = localPath;
                bestLength = localPath.size();
            }
        }
    }
    
    // Gather results from all processes
    int localLength = localSolution.empty() ? maxDepth + 1 : localSolution.size();
    int globalBestLength;
    
    MPI_Allreduce(&localLength, &globalBestLength, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    
    // Determine which process has the best solution
    int hasGlobalBest = (localLength == globalBestLength) ? 1 : 0;
    int rankWithBest = -1;
    
    if (hasGlobalBest) {
        rankWithBest = rank_;
    }
    
    MPI_Allreduce(MPI_IN_PLACE, &rankWithBest, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    
    // Broadcast the best solution
    if (globalBestLength <= maxDepth) {
        if (rank_ == rankWithBest) {
            solution_ = localSolution;
        }
        
        // Broadcast solution size
        int solutionSize = solution_.size();
        MPI_Bcast(&solutionSize, 1, MPI_INT, rankWithBest, MPI_COMM_WORLD);
        
        if (rank_ != rankWithBest) {
            solution_.resize(solutionSize);
        }
        
        // Broadcast each move
        for (auto& move : solution_) {
            char moveStr[10] = {0};
            if (rank_ == rankWithBest) {
                strncpy(moveStr, move.c_str(), sizeof(moveStr) - 1);
            }
            MPI_Bcast(moveStr, sizeof(moveStr), MPI_CHAR, rankWithBest, MPI_COMM_WORLD);
            if (rank_ != rankWithBest) {
                move = std::string(moveStr);
            }
        }
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    solveTime_ = std::chrono::duration<double>(endTime - startTime).count();
    
    if (rank_ == 0) {
        if (!solution_.empty()) {
            std::cout << "Solution found! Moves: " << solution_.size() 
                      << ", Nodes: " << nodesExplored_ 
                      << ", Time: " << solveTime_ << "s"
                      << ", Processes: " << size_ << std::endl;
        } else {
            std::cout << "No solution found within depth " << maxDepth << std::endl;
        }
    }
    
    return solution_;
}

bool MPISolver::search(RubiksCube& cube, int depth, const std::string& lastMove,
                       std::vector<std::string>& path) {
    nodesExplored_++;
    
    if (cube.isSolved()) {
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
        path.push_back(move);
        
        if (search(cube, depth + 1, move, path)) {
            return true;
        }
        
        path.pop_back();
        cube.applyMove(cube.getInverseMove(move));
    }
    
    return false;
}

bool MPISolver::isRedundantMove(const std::string& lastMove, const std::string& nextMove) const {
    if (lastMove.empty()) return false;
    
    char lastFace = lastMove[0];
    char nextFace = nextMove[0];
    
    if (lastFace == nextFace) return true;
    
    if ((lastFace == 'U' && nextFace == 'D') || (lastFace == 'D' && nextFace == 'U')) return true;
    if ((lastFace == 'L' && nextFace == 'R') || (lastFace == 'R' && nextFace == 'L')) return true;
    if ((lastFace == 'F' && nextFace == 'B') || (lastFace == 'B' && nextFace == 'F')) return true;
    
    return false;
}
