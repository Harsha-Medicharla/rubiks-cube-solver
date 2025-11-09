#pragma once
#include "solver.hpp"
#include "rubiks_cube.hpp"
#include <vector>
#include <string>
#include <limits>

// IDA* (Iterative Deepening A*) solver
// Efficient for finding optimal or near-optimal solutions
class IDAStarSolver : public Solver {
public:
    IDAStarSolver();
    
    std::vector<std::string> solve(RubiksCube& cube, int maxDepth = 20) override;
    std::string getName() const override { return "IDA*"; }
    
private:
    std::vector<std::string> solution_;
    std::vector<std::string> currentPath_;
    int threshold_;
    
    // IDA* search
    int search(RubiksCube& cube, int g, int bound, const std::string& lastMove);
    
    // Heuristic function
    int heuristic(const RubiksCube& cube) const;
    
    // Check if move is redundant
    bool isRedundantMove(const std::string& lastMove, const std::string& nextMove) const;
};