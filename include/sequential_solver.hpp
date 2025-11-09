// include/sequential_solver.hpp
#pragma once
#include "solver.hpp"
#include "rubiks_cube.hpp"
#include <chrono>

class SequentialSolver : public Solver {
public:
    SequentialSolver() = default;
    std::vector<std::string> solve(RubiksCube& cube, int maxDepth = 20) override;
    std::string getName() const override { return "Sequential (IDA*)"; }
    
private:
    std::vector<std::string> solution_;
    std::vector<std::string> currentPath_;
    int maxDepth_;
    
    int heuristic(const RubiksCube& cube) const;
    int idaSearch(RubiksCube& cube, int g, int threshold, const std::string& lastMove,
                  double timeLimit, std::chrono::high_resolution_clock::time_point startTime);
    bool isRedundantMove(const std::string& lastMove, const std::string& nextMove) const;
};