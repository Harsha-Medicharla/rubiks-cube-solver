#pragma once
#include "solver.hpp"
#include "rubiks_cube.hpp"
#include <omp.h>

class OpenMPSolver : public Solver {
public:
    OpenMPSolver(int numThreads = 4);
    std::vector<std::string> solve(RubiksCube& cube, int maxDepth = 20) override;
    std::string getName() const override { return "OpenMP"; }
    
    int getNumThreads() const { return numThreads_; }
    
private:
    int numThreads_;
    std::vector<std::string> solution_;
    int maxDepth_;
    bool solutionFound_;
    
    bool searchParallel(RubiksCube& cube, const std::string& firstMove, int depth);
    bool search(RubiksCube& cube, int depth, const std::string& lastMove, 
                std::vector<std::string>& path);
    bool isRedundantMove(const std::string& lastMove, const std::string& nextMove) const;
};
