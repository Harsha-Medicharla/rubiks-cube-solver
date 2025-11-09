#include "mpi_solver.hpp"
#include <iostream>
#include <algorithm>
#include <limits>
#include <iomanip>

bool MPISolver::initialized_ = false;

void MPISolver::Initialize(int* argc, char*** argv) {
    int flag;
    MPI_Initialized(&flag);
    if (!flag) {
        MPI_Init(argc, argv);
        initialized_ = true;
    }
}


void MPISolver::Finalize() {
    int flag;
    MPI_Finalized(&flag);
    if (!flag) {
        MPI_Finalize();
    }
    initialized_ = false;
}

MPISolver::MPISolver() {
    if (!initialized_) throw std::runtime_error("MPI not initialized");
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &size_);
}

MPISolver::~MPISolver() {}

int MPISolver::heuristic(const RubiksCube& cube) const {
    return cube.getManhattanDistance();
}

std::vector<std::string> MPISolver::solve(RubiksCube& cube, int maxDepth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
   // std::cout << "[DEBUG] Rank " << rank_ << ": solve() called, maxDepth=" << maxDepth << std::endl;
    
    if (cube.isSolved()) {
       // std::cout << "[DEBUG] Rank " << rank_ << ": Cube already solved" << std::endl;
        return {};
    }
    
    solution_.clear();
    maxDepth_ = maxDepth;
    nodesExplored_ = 0;
    
    if (rank_ == 0) {
        std::cout << "\n=== MPI IDA* Search ===" << std::endl;
        std::cout << "Processes: " << size_ << std::endl;
        std::cout << "Max depth: " << maxDepth << std::endl;
    }
    
    const double TIME_LIMIT = 120.0;
    int threshold = heuristic(cube);
    bool found = false;
    
   // std::cout << "[DEBUG] Rank " << rank_ << ": Initial threshold=" << threshold << std::endl;
    
    int iteration = 0;
    while (!found && threshold <= maxDepth) {
        iteration++;
        
        if (rank_ == 0) {
            std::cout << "\n[Iteration " << iteration << "] Threshold " << threshold << "..." << std::endl;
        }
        
        // std::cout << "[DEBUG] Rank " << rank_ << ": Starting iteration " << iteration 
        //           << " with threshold " << threshold << std::endl;
        
        auto moves = RubiksCube::getBasicMoves();
        // std::cout << "[DEBUG] Rank " << rank_ << ": Total moves to explore: " << moves.size() << std::endl;
        
        std::vector<std::string> localSolution;
        int localMin = std::numeric_limits<int>::max();
        int movesExplored = 0;
        
        // Distribute moves across processes
        for (size_t i = rank_; i < moves.size(); i += size_) {
            movesExplored++;
            // std::cout << "[DEBUG] Rank " << rank_ << ": Exploring move " << (i+1) 
            //           << "/" << moves.size() << " (" << moves[i] << ")" << std::endl;
            
            RubiksCube localCube = cube;
            localCube.applyMove(moves[i]);
            
            std::vector<std::string> localPath = {moves[i]};
            
            // std::cout << "[DEBUG] Rank " << rank_ << ": Calling idaSearch for move " 
            //           << moves[i] << std::endl;
            
            int temp = idaSearch(localCube, 1, threshold, moves[i], localPath, TIME_LIMIT, startTime);
            
            // std::cout << "[DEBUG] Rank " << rank_ << ": idaSearch returned " 
            //           << (temp == -1 ? "SOLUTION" : std::to_string(temp)) 
            //           << " for move " << moves[i] << std::endl;
            
            if (temp == -1) {
                // std::cout << "[DEBUG] Rank " << rank_ << ": *** SOLUTION FOUND *** Path length: " 
                //           << localPath.size() << std::endl;
                localSolution = localPath;
                localMin = -1;
                break;
            }
            
            if (temp < localMin) {
                localMin = temp;
                // std::cout << "[DEBUG] Rank " << rank_ << ": Updated localMin to " << localMin << std::endl;
            }
        }
        
        // std::cout << "[DEBUG] Rank " << rank_ << ": Explored " << movesExplored 
        //           << " moves, localMin=" << localMin << std::endl;
        
        // Synchronize results across all processes
        // std::cout << "[DEBUG] Rank " << rank_ << ": Calling MPI_Allreduce..." << std::endl;
        
        int globalMin;
        MPI_Allreduce(&localMin, &globalMin, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        
        // std::cout << "[DEBUG] Rank " << rank_ << ": MPI_Allreduce complete, globalMin=" 
        //           << globalMin << std::endl;
        
        if (globalMin == -1) {
            found = true;
            //std::cout << "[DEBUG] Rank " << rank_ << ": Global solution found! Broadcasting..." << std::endl;
            
            // Find which rank has the solution
            int hasGlobalBest = (localMin == -1) ? 1 : 0;
            int rankWithBest = -1;
            if (hasGlobalBest) {
                rankWithBest = rank_;
                //std::cout << "[DEBUG] Rank " << rank_ << ": I have the best solution!" << std::endl;
            }
            
           // std::cout << "[DEBUG] Rank " << rank_ << ": Finding rank with best solution..." << std::endl;
            MPI_Allreduce(MPI_IN_PLACE, &rankWithBest, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
            
            // std::cout << "[DEBUG] Rank " << rank_ << ": Best solution is on rank " 
            //           << rankWithBest << std::endl;
            
            // Broadcast solution from the winning rank
            if (rank_ == rankWithBest) {
                solution_ = localSolution;
                // std::cout << "[DEBUG] Rank " << rank_ << ": Preparing to broadcast solution of length " 
                //           << solution_.size() << std::endl;
            }
            
            int solutionSize = solution_.size();
            //std::cout << "[DEBUG] Rank " << rank_ << ": Broadcasting solution size..." << std::endl;
            MPI_Bcast(&solutionSize, 1, MPI_INT, rankWithBest, MPI_COMM_WORLD);
            //std::cout << "[DEBUG] Rank " << rank_ << ": Solution size = " << solutionSize << std::endl;
            
            if (rank_ != rankWithBest) {
                solution_.resize(solutionSize);
            }
            
            // std::cout << "[DEBUG] Rank " << rank_ << ": Broadcasting " << solutionSize 
            //           << " moves..." << std::endl;
            
            for (size_t j = 0; j < solution_.size(); ++j) {
                char moveStr[10] = {0};
                if (rank_ == rankWithBest) {
                    strncpy(moveStr, solution_[j].c_str(), sizeof(moveStr) - 1);
                }
                MPI_Bcast(moveStr, sizeof(moveStr), MPI_CHAR, rankWithBest, MPI_COMM_WORLD);
                if (rank_ != rankWithBest) {
                    solution_[j] = std::string(moveStr);
                }
            }
            
           // std::cout << "[DEBUG] Rank " << rank_ << ": Solution broadcast complete" << std::endl;
            break;
        }
        
        // Check time limit
        auto currentTime = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(currentTime - startTime).count();
        
        // std::cout << "[DEBUG] Rank " << rank_ << ": Elapsed time: " << std::fixed 
        //           << std::setprecision(2) << elapsed << "s / " << TIME_LIMIT << "s" << std::endl;
        
        if (elapsed > TIME_LIMIT) {
            if (rank_ == 0) {
               // std::cout << "[DEBUG] Time limit reached" << std::endl;
            }
            break;
        }
        
        if (globalMin == std::numeric_limits<int>::max()) {
            if (rank_ == 0) {
               // std::cout << "[DEBUG] No more thresholds to explore (globalMin=MAX)" << std::endl;
            }
            break;
        }
        
        threshold = globalMin;
       // std::cout << "[DEBUG] Rank " << rank_ << ": Next threshold will be " << threshold << std::endl;
    }
    
    auto endTime = std::chrono::high_resolution_clock::now();
    solveTime_ = std::chrono::duration<double>(endTime - startTime).count();
    
    // std::cout << "[DEBUG] Rank " << rank_ << ": Search complete in " << solveTime_ << "s" << std::endl;
    
    if (rank_ == 0) {
        std::cout << "\n=== Search Complete ===" << std::endl;
        if (!solution_.empty()) {
            std::cout << "✓ Solution found!" << std::endl;
            std::cout << "  Moves: " << solution_.size() << std::endl;
            std::cout << "  Nodes: " << nodesExplored_ << std::endl;
            std::cout << "  Time: " << solveTime_ << "s" << std::endl;
            std::cout << "  Processes: " << size_ << std::endl;
        } else {
            std::cout << "✗ No solution found" << std::endl;
            std::cout << "  Nodes: " << nodesExplored_ << std::endl;
            std::cout << "  Time: " << solveTime_ << "s" << std::endl;
        }
    }
    
    return solution_;
}

int MPISolver::idaSearch(RubiksCube& cube, int g, int threshold, const std::string& lastMove,
                        std::vector<std::string>& path, double timeLimit,
                        std::chrono::high_resolution_clock::time_point startTime) {
    nodesExplored_++;
    
    // Debug every 100000 nodes
    if (nodesExplored_ % 100000 == 0) {
        // std::cout << "[DEBUG] Rank " << rank_ << ": Explored " << nodesExplored_ 
        //           << " nodes at depth " << g << std::endl;
    }
    
    int h = heuristic(cube);
    int f = g + h;
    
    if (f > threshold) {
        return f;
    }
    
    if (cube.isSolved()) {
        // std::cout << "[DEBUG] Rank " << rank_ << ": *** SOLVED at depth " << g 
        //           << " (path length: " << path.size() << ") ***" << std::endl;
        return -1;
    }
    
    int min = std::numeric_limits<int>::max();
    auto moves = RubiksCube::getBasicMoves();
    
    for (const auto& move : moves) {
        if (isRedundantMove(lastMove, move)) {
            continue;
        }
        
        cube.applyMove(move);
        path.push_back(move);
        
        int temp = idaSearch(cube, g + 1, threshold, move, path, timeLimit, startTime);
        
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