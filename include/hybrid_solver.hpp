#pragma once
#include "solver.hpp"
#include "rubiks_cube.hpp"
#include <mpi.h>
#include <omp.h>

class HybridSolver : public Solver {
public:
    HybridSolver(int numThreads = 2);
    ~HybridSolver();
    
    std::vector<std::string> solve(RubiksCube& cube, int maxDepth = 20) override;
    std::string getName() const override { return "Hybrid (MPI+OpenMP)"; }
    
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
    
    bool search(RubiksCube& cube, int depth, const std::string& lastMove,
                std::vector<std::string>& path);
    bool isRedundantMove(const std::string& lastMove, const std::string& nextMove) const;
};
