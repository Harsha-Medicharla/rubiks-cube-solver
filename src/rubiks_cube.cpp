#include "rubiks_cube.hpp"
#include <algorithm>
#include <stdexcept>

RubiksCube::RubiksCube() {
    reset();
}

RubiksCube::RubiksCube(const std::string& state) {
    fromString(state);
}

void RubiksCube::reset() {
    // Initialize solved state: W=White, Y=Yellow, G=Green, B=Blue, R=Red, O=Orange
    faces_[UP].fill('W');
    faces_[DOWN].fill('Y');
    faces_[FRONT].fill('G');
    faces_[BACK].fill('B');
    faces_[LEFT].fill('O');
    faces_[RIGHT].fill('R');
}

bool RubiksCube::isSolved() const {
    for (int f = 0; f < 6; ++f) {
        char center = faces_[f][4];
        for (int i = 0; i < 9; ++i) {
            if (faces_[f][i] != center) return false;
        }
    }
    return true;
}

void RubiksCube::scramble(int moves) {
    std::random_device rd;
    std::mt19937 gen(rd());
    auto allMoves = getAllMoves();
    std::uniform_int_distribution<> dis(0, allMoves.size() - 1);
    
    for (int i = 0; i < moves; ++i) {
        applyMove(allMoves[dis(gen)]);
    }
}

void RubiksCube::rotateFaceClockwise(Face face) {
    auto& f = faces_[face];
    std::array<char, 9> temp = f;
    f[0] = temp[6]; f[1] = temp[3]; f[2] = temp[0];
    f[3] = temp[7]; f[4] = temp[4]; f[5] = temp[1];
    f[6] = temp[8]; f[7] = temp[5]; f[8] = temp[2];
}

void RubiksCube::rotateFaceCounterClockwise(Face face) {
    auto& f = faces_[face];
    std::array<char, 9> temp = f;
    f[0] = temp[2]; f[1] = temp[5]; f[2] = temp[8];
    f[3] = temp[1]; f[4] = temp[4]; f[5] = temp[7];
    f[6] = temp[0]; f[7] = temp[3]; f[8] = temp[6];
}

void RubiksCube::rotateFace180(Face face) {
    rotateFaceClockwise(face);
    rotateFaceClockwise(face);
}

void RubiksCube::rotateEdgesClockwise(
    Face f1, int e1a, int e1b, int e1c,
    Face f2, int e2a, int e2b, int e2c,
    Face f3, int e3a, int e3b, int e3c,
    Face f4, int e4a, int e4b, int e4c) {
    
    char temp1 = faces_[f1][e1a];
    char temp2 = faces_[f1][e1b];
    char temp3 = faces_[f1][e1c];
    
    faces_[f1][e1a] = faces_[f4][e4a];
    faces_[f1][e1b] = faces_[f4][e4b];
    faces_[f1][e1c] = faces_[f4][e4c];
    
    faces_[f4][e4a] = faces_[f3][e3a];
    faces_[f4][e4b] = faces_[f3][e3b];
    faces_[f4][e4c] = faces_[f3][e3c];
    
    faces_[f3][e3a] = faces_[f2][e2a];
    faces_[f3][e3b] = faces_[f2][e2b];
    faces_[f3][e3c] = faces_[f2][e2c];
    
    faces_[f2][e2a] = temp1;
    faces_[f2][e2b] = temp2;
    faces_[f2][e2c] = temp3;
}

void RubiksCube::moveU() {
    rotateFaceClockwise(UP);
    rotateEdgesClockwise(FRONT, 0, 1, 2, LEFT, 0, 1, 2, BACK, 0, 1, 2, RIGHT, 0, 1, 2);
}

void RubiksCube::moveUPrime() {
    rotateFaceCounterClockwise(UP);
    rotateEdgesClockwise(FRONT, 0, 1, 2, RIGHT, 0, 1, 2, BACK, 0, 1, 2, LEFT, 0, 1, 2);
}

void RubiksCube::moveU2() {
    moveU();
    moveU();
}

void RubiksCube::moveD() {
    rotateFaceClockwise(DOWN);
    rotateEdgesClockwise(FRONT, 6, 7, 8, RIGHT, 6, 7, 8, BACK, 6, 7, 8, LEFT, 6, 7, 8);
}

void RubiksCube::moveDPrime() {
    rotateFaceCounterClockwise(DOWN);
    rotateEdgesClockwise(FRONT, 6, 7, 8, LEFT, 6, 7, 8, BACK, 6, 7, 8, RIGHT, 6, 7, 8);
}

void RubiksCube::moveD2() {
    moveD();
    moveD();
}

void RubiksCube::moveF() {
    rotateFaceClockwise(FRONT);
    rotateEdgesClockwise(UP, 6, 7, 8, RIGHT, 0, 3, 6, DOWN, 2, 1, 0, LEFT, 8, 5, 2);
}

void RubiksCube::moveFPrime() {
    rotateFaceCounterClockwise(FRONT);
    rotateEdgesClockwise(UP, 6, 7, 8, LEFT, 8, 5, 2, DOWN, 2, 1, 0, RIGHT, 0, 3, 6);
}

void RubiksCube::moveF2() {
    moveF();
    moveF();
}

void RubiksCube::moveB() {
    rotateFaceClockwise(BACK);
    rotateEdgesClockwise(UP, 2, 1, 0, LEFT, 0, 3, 6, DOWN, 6, 7, 8, RIGHT, 8, 5, 2);
}

void RubiksCube::moveBPrime() {
    rotateFaceCounterClockwise(BACK);
    rotateEdgesClockwise(UP, 2, 1, 0, RIGHT, 8, 5, 2, DOWN, 6, 7, 8, LEFT, 0, 3, 6);
}

void RubiksCube::moveB2() {
    moveB();
    moveB();
}

void RubiksCube::moveL() {
    rotateFaceClockwise(LEFT);
    rotateEdgesClockwise(UP, 0, 3, 6, FRONT, 0, 3, 6, DOWN, 0, 3, 6, BACK, 8, 5, 2);
}

void RubiksCube::moveLPrime() {
    rotateFaceCounterClockwise(LEFT);
    rotateEdgesClockwise(UP, 0, 3, 6, BACK, 8, 5, 2, DOWN, 0, 3, 6, FRONT, 0, 3, 6);
}

void RubiksCube::moveL2() {
    moveL();
    moveL();
}

void RubiksCube::moveR() {
    rotateFaceClockwise(RIGHT);
    rotateEdgesClockwise(UP, 8, 5, 2, BACK, 0, 3, 6, DOWN, 8, 5, 2, FRONT, 8, 5, 2);
}

void RubiksCube::moveRPrime() {
    rotateFaceCounterClockwise(RIGHT);
    rotateEdgesClockwise(UP, 8, 5, 2, FRONT, 8, 5, 2, DOWN, 8, 5, 2, BACK, 0, 3, 6);
}

void RubiksCube::moveR2() {
    moveR();
    moveR();
}

void RubiksCube::applyMove(const std::string& move) {
    if (move == "U") moveU();
    else if (move == "U'") moveUPrime();
    else if (move == "U2") moveU2();
    else if (move == "D") moveD();
    else if (move == "D'") moveDPrime();
    else if (move == "D2") moveD2();
    else if (move == "F") moveF();
    else if (move == "F'") moveFPrime();
    else if (move == "F2") moveF2();
    else if (move == "B") moveB();
    else if (move == "B'") moveBPrime();
    else if (move == "B2") moveB2();
    else if (move == "L") moveL();
    else if (move == "L'") moveLPrime();
    else if (move == "L2") moveL2();
    else if (move == "R") moveR();
    else if (move == "R'") moveRPrime();
    else if (move == "R2") moveR2();
    else throw std::invalid_argument("Invalid move: " + move);
}

void RubiksCube::applyMoves(const std::vector<std::string>& moves) {
    for (const auto& move : moves) {
        applyMove(move);
    }
}

std::string RubiksCube::getInverseMove(const std::string& move) const {
    if (move.size() == 2 && move[1] == '\'') return std::string(1, move[0]);
    if (move.size() == 2 && move[1] == '2') return move;
    return move + "'";
}

std::vector<std::string> RubiksCube::getAllMoves() {
    return {"U", "U'", "U2", "D", "D'", "D2", "F", "F'", "F2", 
            "B", "B'", "B2", "L", "L'", "L2", "R", "R'", "R2"};
}

std::vector<std::string> RubiksCube::getBasicMoves() {
    return {"U", "U'", "D", "D'", "F", "F'", "B", "B'", "L", "L'", "R", "R'"};
}

std::string RubiksCube::toString() const {
    std::stringstream ss;
    for (int f = 0; f < 6; ++f) {
        for (int i = 0; i < 9; ++i) {
            ss << faces_[f][i];
        }
    }
    return ss.str();
}

std::string RubiksCube::toJSON() const {
    std::stringstream ss;
    ss << "{\"faces\":{";
    ss << "\"U\":[";
    for (int i = 0; i < 9; ++i) {
        ss << "\"" << faces_[UP][i] << "\"";
        if (i < 8) ss << ",";
    }
    ss << "],\"D\":[";
    for (int i = 0; i < 9; ++i) {
        ss << "\"" << faces_[DOWN][i] << "\"";
        if (i < 8) ss << ",";
    }
    ss << "],\"F\":[";
    for (int i = 0; i < 9; ++i) {
        ss << "\"" << faces_[FRONT][i] << "\"";
        if (i < 8) ss << ",";
    }
    ss << "],\"B\":[";
    for (int i = 0; i < 9; ++i) {
        ss << "\"" << faces_[BACK][i] << "\"";
        if (i < 8) ss << ",";
    }
    ss << "],\"L\":[";
    for (int i = 0; i < 9; ++i) {
        ss << "\"" << faces_[LEFT][i] << "\"";
        if (i < 8) ss << ",";
    }
    ss << "],\"R\":[";
    for (int i = 0; i < 9; ++i) {
        ss << "\"" << faces_[RIGHT][i] << "\"";
        if (i < 8) ss << ",";
    }
    ss << "]},\"isSolved\":" << (isSolved() ? "true" : "false") << "}";
    return ss.str();
}

void RubiksCube::fromString(const std::string& state) {
    if (state.length() != 54) {
        throw std::invalid_argument("State must be 54 characters");
    }
    int idx = 0;
    for (int f = 0; f < 6; ++f) {
        for (int i = 0; i < 9; ++i) {
            faces_[f][i] = state[idx++];
        }
    }
}

bool RubiksCube::operator==(const RubiksCube& other) const {
    return faces_ == other.faces_;
}

bool RubiksCube::operator!=(const RubiksCube& other) const {
    return !(*this == other);
}

size_t RubiksCube::hash() const {
    size_t h = 0;
    for (const auto& face : faces_) {
        for (char c : face) {
            h = h * 31 + c;
        }
    }
    return h;
}

int RubiksCube::getManhattanDistance() const {
    int distance = 0;
    
    // Simple heuristic: count misplaced stickers
    for (int f = 0; f < 6; ++f) {
        char center = faces_[f][4];
        for (int i = 0; i < 9; ++i) {
            if (i != 4 && faces_[f][i] != center) {
                distance++;
            }
        }
    }
    
    return distance / 8; // Divide by 8 as each move affects ~8 stickers
}