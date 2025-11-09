#pragma once
#include "solver.hpp"
#include "rubiks_cube.hpp"
#include <mpi.h>
#include <string>

class MPISolver : public Solver {
public:
    MPISolver();
    ~MPISolver();
    
    std::vector<std::string> solve(RubiksCube& cube, int maxDepth = 20) override;
    std::string getName() const override { return "MPI"; }
    
    // Must be called before using solver
    static void Initialize(int* argc, char*** argv);
    static void Finalize();
    static bool IsInitialized() { return initialized_; }
    
    int getRank() const { return rank_; }
    int getSize() const { return size_; }
    
private:
    int rank_;
    int size_;
    static bool initialized_;
    
    std::vector<std::string> solution_;
    int maxDepth_;
    
    bool search(RubiksCube& cube, int depth, const std::string& lastMove,
                std::vector<std::string>& path);
    bool isRedundantMove(const std::string& lastMove, const std::string& nextMove) const;
};
