#pragma once
#include "rubiks_cube.hpp"
#include <vector>
#include <string>

// Abstract solver interface
class Solver {
public:
    virtual ~Solver() = default;
    
    // Solve the cube and return the sequence of moves
    virtual std::vector<std::string> solve(RubiksCube& cube, int maxDepth = 20) = 0;
    
    // Get solver name
    virtual std::string getName() const = 0;
    
    // Get statistics from last solve
    virtual int getNodesExplored() const { return nodesExplored_; }
    virtual double getSolveTime() const { return solveTime_; }
    
protected:
    int nodesExplored_ = 0;
    double solveTime_ = 0.0;
};