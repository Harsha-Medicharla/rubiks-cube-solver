#pragma once
#include "solver.hpp"
#include "rubiks_cube.hpp"
#include <mpi.h>
#include <omp.h>
#include <chrono>

class HybridSolver : public Solver {
public:
    HybridSolver(int numThreads = 2);
    ~HybridSolver();
    
    std::vector<std::string> solve(RubiksCube& cube, int maxDepth = 20) override;
    std::string getName() const override { return "Hybrid (MPI+OpenMP IDA*)"; }
    
    static void Initialize(int* argc, char*** argv);
    static void Finalize();
    static bool IsInitialized() { return initialized_; }
    
    int getRank() const { return rank_; }
    int getSize() const { return size_; }
    int getNumThreads() const { return numThreads_; }
    
private:
    int rank_;
    int size_;
    int numThreads_;
    static bool initialized_;
    
    std::vector<std::string> solution_;
    int maxDepth_;
    bool solutionFound_;
    
    int heuristic(const RubiksCube& cube) const;
    int idaSearchHybrid(RubiksCube& cube, int g, int threshold,
                       const std::string& lastMove, std::vector<std::string>& path,
                       double timeLimit, std::chrono::high_resolution_clock::time_point startTime);
    bool isRedundantMove(const std::string& lastMove, const std::string& nextMove) const;
};