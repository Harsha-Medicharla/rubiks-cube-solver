// Updated include/http_server.hpp
#pragma once
#include "rubiks_cube.hpp"
#include "solver.hpp"
#include <string>
#include <memory>
#include <map>

class HTTPServer {
public:
    HTTPServer(int port = 8080);
    ~HTTPServer();
    
    void start();
    void stop();
    
    // Solver selection
    void setSolver(const std::string& solverType);
    std::string getCurrentSolver() const;
    std::vector<std::string> getAvailableSolvers() const;
    
private:
    int port_;
    int serverSocket_;
    bool running_;
    std::unique_ptr<Solver> solver_;
    std::string currentSolverType_;
    RubiksCube currentCube_;
    
    // Request handlers
    std::string handleRequest(const std::string& request);
    std::string handleGET(const std::string& path);
    std::string handlePOST(const std::string& path, const std::string& body);
    std::string handleOPTIONS();
    
    // API endpoints
    std::string getStatus();
    std::string getCubeState();
    std::string resetCube();
    std::string scrambleCube(const std::string& body);
    std::string applyMove(const std::string& body);
    std::string solveCube(const std::string& body);
    std::string setCubeState(const std::string& body);
    std::string selectSolver(const std::string& body);
    std::string listSolvers();
    
    // HTTP helpers
    std::string createResponse(int status, const std::string& body, 
                               const std::string& contentType = "application/json");
    std::map<std::string, std::string> parseJSON(const std::string& json);
    std::string extractJSONValue(const std::string& json, const std::string& key);
    
    // Solver factory
    std::unique_ptr<Solver> createSolver(const std::string& type);
};
