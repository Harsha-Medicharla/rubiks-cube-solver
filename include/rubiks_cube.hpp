#pragma once
#include <array>
#include <string>
#include <vector>
#include <random>
#include <sstream>

// Represents a 3x3x3 Rubik's Cube
class RubiksCube {
public:
    // Face indices
    enum Face { UP = 0, DOWN = 1, FRONT = 2, BACK = 3, LEFT = 4, RIGHT = 5 };
    
    // Each face has 9 stickers (3x3 grid)
    // Sticker indices: 0-8 for each face
    // Layout:
    //   0 1 2
    //   3 4 5
    //   6 7 8
    
    RubiksCube();
    explicit RubiksCube(const std::string& state);
    
    // Core operations
    void reset();
    void scramble(int moves = 20);
    bool isSolved() const;
    
    // Move operations (clockwise 90 degrees)
    void moveU();   // Up face
    void moveD();   // Down face
    void moveL();   // Left face
    void moveR();   // Right face
    void moveF();   // Front face
    void moveB();   // Back face
    
    // Prime moves (counter-clockwise 90 degrees)
    void moveUPrime();
    void moveDPrime();
    void moveLPrime();
    void moveRPrime();
    void moveFPrime();
    void moveBPrime();
    
    // Double moves (180 degrees)
    void moveU2();
    void moveD2();
    void moveL2();
    void moveR2();
    void moveF2();
    void moveB2();
    
    // Apply move from string notation
    void applyMove(const std::string& move);
    void applyMoves(const std::vector<std::string>& moves);
    
    // Get inverse of a move
    std::string getInverseMove(const std::string& move) const;
    
    // Serialization
    std::string toString() const;
    std::string toJSON() const;
    void fromString(const std::string& state);
    
    // Get all possible moves
    static std::vector<std::string> getAllMoves();
    static std::vector<std::string> getBasicMoves(); // Only 90-degree moves
    
    // Heuristic for A* search
    int getManhattanDistance() const;
    
    // Comparison
    bool operator==(const RubiksCube& other) const;
    bool operator!=(const RubiksCube& other) const;
    
    // Hash for unordered containers
    size_t hash() const;
    
    // Get face color
    char getFaceCenter(Face face) const { return faces_[face][4]; }
    char getSticker(Face face, int position) const { return faces_[face][position]; }
    
private:
    // 6 faces, each with 9 stickers
    std::array<std::array<char, 9>, 6> faces_;
    
    // Helper functions
    void rotateFaceClockwise(Face face);
    void rotateFaceCounterClockwise(Face face);
    void rotateFace180(Face face);
    
    // Edge rotations
    void rotateEdgesClockwise(
        Face f1, int e1a, int e1b, int e1c,
        Face f2, int e2a, int e2b, int e2c,
        Face f3, int e3a, int e3b, int e3c,
        Face f4, int e4a, int e4b, int e4c
    );
};

// Hash function for use in unordered containers
namespace std {
    template<>
    struct hash<RubiksCube> {
        size_t operator()(const RubiksCube& cube) const {
            return cube.hash();
        }
    };
}