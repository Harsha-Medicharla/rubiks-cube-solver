#include "hybrid_solver.hpp"
#include <iostream>
#include <limits>
#include <iomanip>

bool HybridSolver::initialized_ = false;

void HybridSolver::Initialize(int* argc, char*** argv) {
    int flag = 0;
    MPI_Initialized(&flag);
    if (!flag) {
        int provided = 0;
        MPI_Init_thread(argc, argv, MPI_THREAD_FUNNELED, &provided);
        // std::cout << "[DEBUG] Hybrid MPI initialized (MPI_Init_thread) (thread support: " 
        //           << provided << ")" << std::endl;
    } else {
        int provided = 0;
        MPI_Query_thread(&provided);
        // std::cout << "[DEBUG] Hybrid MPI detected already initialized (thread support: " 
        //           << provided << ")" << std::endl;
    }
    initialized_ = true;
}

void HybridSolver::Finalize() {
    if (initialized_) {
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        // std::cout << "[DEBUG] Hybrid MPI finalizing for rank " << rank << std::endl;
        MPI_Finalize();
        initialized_ = false;
    }
}

HybridSolver::HybridSolver(int numThreads) 
    : numThreads_(numThreads), solutionFound_(false) {
    // Fix: verify MPI is initialized via MPI_Initialized instead of relying solely on static flag.
    int flag = 0;
    MPI_Initialized(&flag);
    if (!flag) {
        throw std::runtime_error("MPI not initialized");
    }
    // ensure our static flag reflects the runtime state
    initialized_ = true;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Comm_size(MPI_COMM_WORLD, &size_);
    omp_set_num_threads(numThreads_);
    // std::cout << "[DEBUG] HybridSolver constructed - Rank: " << rank_ << "/" << size_ 
    //           << ", Threads: " << numThreads_ << std::endl;
}

HybridSolver::~HybridSolver() {
    // std::cout << "[DEBUG] HybridSolver destructor - Rank: " << rank_ << std::endl;
}

int HybridSolver::heuristic(const RubiksCube& cube) const {
    return cube.getManhattanDistance();
}

std::vector<std::string> HybridSolver::solve(RubiksCube& cube, int maxDepth) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // std::cout << "[DEBUG] Rank " << rank_ << ": solve() called, maxDepth=" << maxDepth << std::endl;
    
    if (cube.isSolved()) {
        // std::cout << "[DEBUG] Rank " << rank_ << ": Cube already solved" << std::endl;
        return {};
    }
    
    solution_.clear();
    maxDepth_ = maxDepth;
    nodesExplored_ = 0;
    solutionFound_ = false;
    
    if (rank_ == 0) {
        std::cout << "\n=== Hybrid (MPI+OpenMP) IDA* Search ===" << std::endl;
        std::cout << "Processes: " << size_ << ", Threads/Process: " << numThreads_ << std::endl;
        std::cout << "Total workers: " << (size_ * numThreads_) << std::endl;
        std::cout << "Max depth: " << maxDepth << std::endl;
    }
    
    const double TIME_LIMIT = 120.0;
    int threshold = heuristic(cube);
    bool found = false;
    
    // std::cout << "[DEBUG] Rank " << rank_ << ": Initial threshold=" << threshold << std::endl;
    
    int iteration = 0;
    while (!found && threshold <= maxDepth && !solutionFound_) {
        iteration++;
        
        if (rank_ == 0) {
            std::cout << "\n[Iteration " << iteration << "] Threshold " << threshold << "..." << std::endl;
        }
        
        // std::cout << "[DEBUG] Rank " << rank_ << ": Starting iteration " << iteration 
        //           << " with threshold " << threshold << std::endl;
        
        auto moves = RubiksCube::getBasicMoves();
        // std::cout << "[DEBUG] Rank " << rank_ << ": Total moves: " << moves.size() << std::endl;
        
        std::vector<std::string> localSolution;
        int localMin = std::numeric_limits<int>::max();
        
        // Calculate which moves this rank will handle
        int myMoveCount = 0;
        for (size_t i = rank_; i < moves.size(); i += size_) {
            myMoveCount++;
        }
        // std::cout << "[DEBUG] Rank " << rank_ << ": Will explore " << myMoveCount 
        //           << " moves with " << numThreads_ << " threads" << std::endl;
        
        // OpenMP parallel loop over moves assigned to this MPI rank
        #pragma omp parallel for schedule(dynamic)
        for (size_t i = rank_; i < moves.size(); i += size_) {
            if (solutionFound_) continue;
            
            int tid = omp_get_thread_num();
            
            #pragma omp critical
            {
                // std::cout << "[DEBUG] Rank " << rank_ << ", Thread " << tid 
                //           << ": Exploring move " << moves[i] << " (index " << i << ")" << std::endl;
            }
            
            RubiksCube localCube = cube;
            localCube.applyMove(moves[i]);
            
            std::vector<std::string> localPath = {moves[i]};
            int temp = idaSearchHybrid(localCube, 1, threshold, moves[i], localPath, TIME_LIMIT, startTime);
            
            #pragma omp critical
            {
                // std::cout << "[DEBUG] Rank " << rank_ << ", Thread " << tid 
                //           << ": Move " << moves[i] << " returned " 
                //           << (temp == -1 ? "SOLUTION" : std::to_string(temp)) << std::endl;
            }
            
            if (temp == -1) {
                #pragma omp critical
                {
                    if (!solutionFound_) {
                        // std::cout << "[DEBUG] Rank " << rank_ << ", Thread " << tid 
                        //           << ": *** SOLUTION FOUND *** Path length: " 
                        //           << localPath.size() << std::endl;
                        localSolution = localPath;
                        localMin = -1;
                        solutionFound_ = true;
                    }
                }
            } else if (temp < localMin) {
                #pragma omp critical
                {
                    if (temp < localMin) {
                        localMin = temp;
                        // std::cout << "[DEBUG] Rank " << rank_ << ", Thread " << tid 
                        //           << ": Updated localMin to " << localMin << std::endl;
                    }
                }
            }
        }
        
        // std::cout << "[DEBUG] Rank " << rank_ << ": OpenMP parallel section complete, localMin=" 
        //           << localMin << std::endl;
        
        // MPI synchronization
        // std::cout << "[DEBUG] Rank " << rank_ << ": Calling MPI_Allreduce..." << std::endl;
        
        int globalMin;
        MPI_Allreduce(&localMin, &globalMin, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        
        // std::cout << "[DEBUG] Rank " << rank_ << ": MPI_Allreduce complete, globalMin=" 
        //           << globalMin << std::endl;
        
        if (globalMin == -1) {
            found = true;
            // std::cout << "[DEBUG] Rank " << rank_ << ": Global solution found! Broadcasting..." << std::endl;
            
            int hasGlobalBest = (localMin == -1) ? 1 : 0;
            int rankWithBest = -1;
            if (hasGlobalBest) {
                rankWithBest = rank_;
                // std::cout << "[DEBUG] Rank " << rank_ << ": I have the best solution!" << std::endl;
            }
            
            // std::cout << "[DEBUG] Rank " << rank_ << ": Finding rank with best solution..." << std::endl;
            MPI_Allreduce(MPI_IN_PLACE, &rankWithBest, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
            
            // std::cout << "[DEBUG] Rank " << rank_ << ": Best solution is on rank " 
            //           << rankWithBest << std::endl;
            
            if (rank_ == rankWithBest) {
                solution_ = localSolution;
                // std::cout << "[DEBUG] Rank " << rank_ << ": Preparing to broadcast solution of length " 
                //           << solution_.size() << std::endl;
            }
            
            int solutionSize = solution_.size();
            // std::cout << "[DEBUG] Rank " << rank_ << ": Broadcasting solution size..." << std::endl;
            MPI_Bcast(&solutionSize, 1, MPI_INT, rankWithBest, MPI_COMM_WORLD);
            // std::cout << "[DEBUG] Rank " << rank_ << ": Solution size = " << solutionSize << std::endl;
            
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
            // break;
        }
        
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
            std::cout << "  Time: " << solveTime_ << "s" << std::endl;
            std::cout << "  Processes: " << size_ << std::endl;
            std::cout << "  Threads/Process: " << numThreads_ << std::endl;
            std::cout << "  Total workers: " << (size_ * numThreads_) << std::endl;
        } else {
            std::cout << "✗ No solution found" << std::endl;
            std::cout << "  Time: " << solveTime_ << "s" << std::endl;
        }
    }
    
    return solution_;
}

int HybridSolver::idaSearchHybrid(RubiksCube& cube, int g, int threshold,
                                 const std::string& lastMove, std::vector<std::string>& path,
                                 double timeLimit, std::chrono::high_resolution_clock::time_point startTime) {
    #pragma omp atomic
    nodesExplored_++;
    
    // Debug every 100000 nodes (thread-safe)
    int currentNodes = nodesExplored_;
    if (currentNodes % 100000 == 0) {
        int tid = omp_get_thread_num();
        #pragma omp critical
        {
            // std::cout << "[DEBUG] Rank " << rank_ << ", Thread " << tid 
            //           << ": Explored " << currentNodes << " nodes at depth " << g << std::endl;
        }
    }
    
    if (solutionFound_) return std::numeric_limits<int>::max();
    
    int h = heuristic(cube);
    int f = g + h;
    
    if (f > threshold) return f;
    
    if (cube.isSolved()) {
        int tid = omp_get_thread_num();
        #pragma omp critical
        {
            // std::cout << "[DEBUG] Rank " << rank_ << ", Thread " << tid 
            //           << ": *** SOLVED at depth " << g << " (path length: " 
            //           << path.size() << ") ***" << std::endl;
        }
        return -1;
    }
    
    int min = std::numeric_limits<int>::max();
    auto moves = RubiksCube::getBasicMoves();
    
    for (const auto& move : moves) {
        if (solutionFound_) return std::numeric_limits<int>::max();
        if (isRedundantMove(lastMove, move)) continue;
        
        cube.applyMove(move);
        path.push_back(move);
        
        int temp = idaSearchHybrid(cube, g + 1, threshold, move, path, timeLimit, startTime);
        
        if (temp == -1) return -1;
        if (temp < min) min = temp;
        
        path.pop_back();
        cube.applyMove(cube.getInverseMove(move));
    }
    
    return min;
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