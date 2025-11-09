#include "hybrid_solver.hpp"
#include <iostream>
#include <chrono>

bool HybridSolver::initialized_ = false;

void HybridSolver::Initialize(int* argc, char*** argv) {
    if (!initialized_) {
        int provided;
        MPI_Init_thread(argc, argv, MPI_THREAD_FUNNELED, &provided);
        initialized_ = true;
    }
}

void HybridSolver::Finalize() {
    if (initialized_) {
        MPI_Finalize();
        initialized_ = false;
    }
}

HybridSolver::HybridSolver(int numThreads) 
    : numThreads_(numThreads), solutionFound_(false) {
    if (!initialized_) {
        throw std::runtime_error("MPI not initialized. Call HybridSolver::Initialize() first.");
    }
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &size_);
    
    omp_set_num_threads(numThreads_);
}

HybridSolver::~HybridSolver() {
    // Don't finalize here
}

std::vector<std::string> HybridSolver::solve(RubiksCube& cube, int maxDepth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (cube.isSolved()) {
        return {};
    }
    
    solution_.clear();
    maxDepth_ = maxDepth;
    nodesExplored_ = 0;
    solutionFound_ = false;
    
    if (rank_ == 0) {
        std::cout << "Starting Hybrid (MPI+OpenMP) search (processes: " << size_ 
                  << ", threads/process: " << numThreads_
                  << ", max depth: " << maxDepth << ")..." << std::endl;
    }
    
    auto moves = RubiksCube::getBasicMoves();
    std::vector<std::string> localSolution;
    int bestLength = maxDepth + 1;
    
    // MPI level: distribute moves among processes
    // OpenMP level: parallelize within each process
    for (size_t i = rank_; i < moves.size(); i += size_) {
        if (solutionFound_) break;
        
        RubiksCube localCube = cube;
        localCube.applyMove(moves[i]);
        
        std::vector<std::string> localPath = {moves[i]};
        
        if (search(localCube, 1, moves[i], localPath)) {
            if (localPath.size() < (size_t)bestLength) {
                localSolution = localPath;
                bestLength = localPath.size();
                solutionFound_ = true;
            }
        }
    }
    
    // Gather results
    int localLength = localSolution.empty() ? maxDepth + 1 : localSolution.size();
    int globalBestLength;
    
    MPI_Allreduce(&localLength, &globalBestLength, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    
    int hasGlobalBest = (localLength == globalBestLength) ? 1 : 0;
    int rankWithBest = -1;
    
    if (hasGlobalBest) {
        rankWithBest = rank_;
    }
    
    MPI_Allreduce(MPI_IN_PLACE, &rankWithBest, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    
    // Broadcast solution
    if (globalBestLength <= maxDepth) {
        if (rank_ == rankWithBest) {
            solution_ = localSolution;
        }
        
        int solutionSize = solution_.size();
        MPI_Bcast(&solutionSize, 1, MPI_INT, rankWithBest, MPI_COMM_WORLD);
        
        if (rank_ != rankWithBest) {
            solution_.resize(solutionSize);
        }
        
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
                      << ", Processes: " << size_
                      << ", Threads/Process: " << numThreads_ << std::endl;
        } else {
            std::cout << "No solution found within depth " << maxDepth << std::endl;
        }
    }
    
    return solution_;
}

bool HybridSolver::search(RubiksCube& cube, int depth, const std::string& lastMove,
                          std::vector<std::string>& path) {
    #pragma omp atomic
    nodesExplored_++;
    
    if (solutionFound_) return false;
    
    if (cube.isSolved()) {
        solutionFound_ = true;
        return true;
    }
    
    if (depth >= maxDepth_) {
        return false;
    }
    
    auto moves = RubiksCube::getBasicMoves();
    
    // Use OpenMP for deeper levels
    if (depth == 2) {
        bool found = false;
        #pragma omp parallel for schedule(dynamic)
        for (size_t i = 0; i < moves.size(); ++i) {
            if (solutionFound_ || found) continue;
            
            if (isRedundantMove(lastMove, moves[i])) {
                continue;
            }
            
            RubiksCube localCube = cube;
            localCube.applyMove(moves[i]);
            
            std::vector<std::string> localPath = path;
            localPath.push_back(moves[i]);
            
            if (search(localCube, depth + 1, moves[i], localPath)) {
                #pragma omp critical
                {
                    if (!found) {
                        found = true;
                    }
                }
            }
        }
        return found;
    } else {
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
    }
    
    return false;
}

bool HybridSolver::isRedundantMove(const std::string& lastMove, const std::string& nextMove) const {
    if (lastMove.empty()) return false;
    
    char lastFace = lastMove[0];
    char nextFace = nextMove[0];
    
    if (lastFace == nextFace) return true;
    
    if ((lastFace == 'U' && nextFace == 'D') || (lastFace == 'D' && nextFace == 'U')) return true;
    if ((lastFace == 'L' && nextFace == 'R') || (lastFace == 'R' && nextFace == 'L')) return true;
    if ((lastFace == 'F' && nextFace == 'B') || (lastFace == 'B' && nextFace == 'F')) return true;
    
    return false;
}