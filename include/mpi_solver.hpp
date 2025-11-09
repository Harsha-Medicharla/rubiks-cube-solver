#pragma once
#include "solver.hpp"
#include "rubiks_cube.hpp"
#include <mpi.h>
#include <chrono>

class MPISolver : public Solver {
public:
    MPISolver();
    ~MPISolver();
    
    std::vector<std::string> solve(RubiksCube& cube, int maxDepth = 20) override;
    std::string getName() const override { return "MPI (IDA*)"; }
    
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
    
    int heuristic(const RubiksCube& cube) const;
    int idaSearch(RubiksCube& cube, int g, int threshold, const std::string& lastMove,
                  std::vector<std::string>& path, double timeLimit,
                  std::chrono::high_resolution_clock::time_point startTime);
    bool isRedundantMove(const std::string& lastMove, const std::string& nextMove) const;
};
