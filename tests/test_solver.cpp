#include "rubiks_cube.hpp"
#include "sequential_solver.hpp" 
#include <iostream>
#include <cassert>
#include <vector>

void testCubeInitialization() {
    std::cout << "Testing cube initialization..." << std::endl;
    RubiksCube cube;
    assert(cube.isSolved());
    std::cout << "  ✓ Cube initialized as solved" << std::endl;
}

void testCubeMoves() {
    std::cout << "Testing cube moves..." << std::endl;
    RubiksCube cube;
    
    cube.moveU();
    assert(!cube.isSolved());
    cube.moveUPrime();
    assert(cube.isSolved());
    std::cout << "  ✓ U and U' moves work correctly" << std::endl;
    
    std::vector<std::string> moves = {"U", "D", "F", "B", "L", "R"};
    for (const auto& move : moves) {
        cube.applyMove(move);
        assert(!cube.isSolved());
        cube.applyMove(move + "'");
        assert(cube.isSolved());
    }
    std::cout << "  ✓ All basic moves and inverses work" << std::endl;
    
    cube.moveU();
    cube.moveU();
    RubiksCube cube2;
    cube2.moveU2();
    assert(cube == cube2);
    std::cout << "  ✓ Double moves work correctly" << std::endl;
}

void testCubeSerialization() {
    std::cout << "Testing cube serialization..." << std::endl;
    RubiksCube cube1;
    cube1.scramble(10);
    
    std::string state = cube1.toString();
    RubiksCube cube2(state);
    
    assert(cube1 == cube2);
    std::cout << "  ✓ Serialization and deserialization work" << std::endl;
}

void testScramble() {
    std::cout << "Testing scramble..." << std::endl;
    RubiksCube cube;
    cube.scramble(20);
    assert(!cube.isSolved());
    std::cout << "  ✓ Scramble produces non-solved state" << std::endl;
}

void testSolverOnEasyCase() {
    std::cout << "Testing solver on easy case..." << std::endl;
    RubiksCube cube;
    
    cube.moveU();
    cube.moveR();
    
    SequentialSolver solver;
    auto solution = solver.solve(cube, 10);
    
    cube.applyMoves(solution);
    assert(cube.isSolved());
    std::cout << "  ✓ Solver found solution with " << solution.size() << " moves" << std::endl;
}

void testSolverOnSolvedCube() {
    std::cout << "Testing solver on already solved cube..." << std::endl;
    RubiksCube cube;
    
    SequentialSolver solver; 
    auto solution = solver.solve(cube, 10);
    
    assert(solution.empty());
    std::cout << "  ✓ Solver returns empty solution for solved cube" << std::endl;
}

void testMoveSequence() {
    std::cout << "Testing move sequence..." << std::endl;
    RubiksCube cube;
    
    std::vector<std::string> moves = {"R", "U", "R'", "U'"};
    cube.applyMoves(moves);
    
    for (int i = moves.size() - 1; i >= 0; --i) {
        cube.applyMove(cube.getInverseMove(moves[i]));
    }
    
    assert(cube.isSolved());
    std::cout << "  ✓ Move sequence and inverse restore solved state" << std::endl;
}

void testGetAllMoves() {
    std::cout << "Testing getAllMoves..." << std::endl;
    auto moves = RubiksCube::getAllMoves();
    assert(moves.size() == 18);
    std::cout << "  ✓ getAllMoves returns 18 moves" << std::endl;
}

void testJSON() {
    std::cout << "Testing JSON output..." << std::endl;
    RubiksCube cube;
    std::string json = cube.toJSON();
    assert(json.find("\"isSolved\":true") != std::string::npos);
    std::cout << "  ✓ JSON output contains expected fields" << std::endl;
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Running Rubik's Cube Solver Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    try {
        testCubeInitialization();
        testCubeMoves();
        testCubeSerialization();
        testScramble();
        testSolverOnSolvedCube();
        testSolverOnEasyCase();
        testMoveSequence();
        testGetAllMoves();
        testJSON();
        
        std::cout << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "All tests passed! ✓" << std::endl;
        std::cout << "========================================" << std::endl;
        return 0;
    } catch (...) {
        std::cout << "\nTest failed\n";
        return 1;
    }
}
