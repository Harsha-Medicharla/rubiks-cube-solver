#pragma once
#include "solver.hpp"
#include "rubiks_cube.hpp"

class SequentialSolver : public Solver {
public:
    SequentialSolver() = default;
    std::vector<std::string> solve(RubiksCube& cube, int maxDepth = 20) override;
    std::string getName() const override { return "Sequential (Brute-Force)"; }
    
private:
    std::vector<std::string> solution_;
    std::vector<std::string> currentPath_;
    int maxDepth_;
    
    bool search(RubiksCube& cube, int depth, const std::string& lastMove);
    bool isRedundantMove(const std::string& lastMove, const std::string& nextMove) const;
};
