#pragma once
#include "rubiks_cube.hpp"
#include "solver.hpp"
#include <string>
#include <memory>
#include <map>

// Simple HTTP server for REST API
class HTTPServer {
public:
    HTTPServer(int port = 8080);
    ~HTTPServer();
    
    // Start server (blocking)
    void start();
    
    // Stop server
    void stop();
    
private:
    int port_;
    int serverSocket_;
    bool running_;
    std::unique_ptr<Solver> solver_;
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
    
    // HTTP helpers
    std::string createResponse(int status, const std::string& body, const std::string& contentType = "application/json");
    std::map<std::string, std::string> parseJSON(const std::string& json);
    std::string extractJSONValue(const std::string& json, const std::string& key);
};