#pragma once
#include "solver.hpp"
#include "rubiks_cube.hpp"
#include <omp.h>
#include <chrono>

class OpenMPSolver : public Solver {
public:
    OpenMPSolver(int numThreads = 4);
    std::vector<std::string> solve(RubiksCube& cube, int maxDepth = 20) override;
    std::string getName() const override { return "OpenMP (IDA*)"; }
    
    int getNumThreads() const { return numThreads_; }
    
private:
    int numThreads_;
    std::vector<std::string> solution_;
    int maxDepth_;
    bool solutionFound_;
    
    int heuristic(const RubiksCube& cube) const;
    int idaSearchParallel(RubiksCube& cube, int g, int threshold,
                         const std::string& lastMove, std::vector<std::string>& path,
                         double timeLimit, std::chrono::high_resolution_clock::time_point startTime);
    bool isRedundantMove(const std::string& lastMove, const std::string& nextMove) const;
};
