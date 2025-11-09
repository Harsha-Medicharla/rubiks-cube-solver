#include "http_server.hpp"
#include "ida_star_solver.hpp"
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

HTTPServer::HTTPServer(int port) 
    : port_(port), serverSocket_(-1), running_(false) {
    solver_ = std::make_unique<IDAStarSolver>();
    currentCube_.reset();
}

HTTPServer::~HTTPServer() {
    stop();
}

void HTTPServer::start() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    int opt = 1;
    setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);
    
    if (bind(serverSocket_, (sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Failed to bind socket" << std::endl;
        return;
    }
    
    if (listen(serverSocket_, 10) < 0) {
        std::cerr << "Failed to listen on socket" << std::endl;
        return;
    }
    
    running_ = true;
    std::cout << "Server started on port " << port_ << std::endl;
    std::cout << "API Endpoints:" << std::endl;
    std::cout << "  GET  /status         - Server status" << std::endl;
    std::cout << "  GET  /cube           - Current cube state" << std::endl;
    std::cout << "  POST /cube/reset     - Reset to solved state" << std::endl;
    std::cout << "  POST /cube/scramble  - Scramble the cube" << std::endl;
    std::cout << "  POST /cube/move      - Apply a move" << std::endl;
    std::cout << "  POST /cube/solve     - Solve the cube" << std::endl;
    std::cout << "  POST /cube/state     - Set cube state" << std::endl;
    
    while (running_) {
        sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket_, (sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket < 0) {
            continue;
        }
        
        char buffer[4096] = {0};
        read(clientSocket, buffer, sizeof(buffer) - 1);
        
        std::string request(buffer);
        std::string response = handleRequest(request);
        
        send(clientSocket, response.c_str(), response.length(), 0);
        close(clientSocket);
    }
}

void HTTPServer::stop() {
    running_ = false;
    if (serverSocket_ >= 0) {
        close(serverSocket_);
        serverSocket_ = -1;
    }
}

std::string HTTPServer::handleRequest(const std::string& request) {
    std::istringstream stream(request);
    std::string method, path, version;
    stream >> method >> path >> version;
    
    // Handle CORS preflight
    if (method == "OPTIONS") {
        return handleOPTIONS();
    }
    
    // Extract body for POST requests
    std::string body;
    size_t bodyPos = request.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        body = request.substr(bodyPos + 4);
    }
    
    if (method == "GET") {
        return handleGET(path);
    } else if (method == "POST") {
        return handlePOST(path, body);
    }
    
    return createResponse(405, "{\"error\":\"Method not allowed\"}");
}

std::string HTTPServer::handleGET(const std::string& path) {
    if (path == "/status") {
        return getStatus();
    } else if (path == "/cube") {
        return getCubeState();
    }
    
    return createResponse(404, "{\"error\":\"Not found\"}");
}

std::string HTTPServer::handlePOST(const std::string& path, const std::string& body) {
    if (path == "/cube/reset") {
        return resetCube();
    } else if (path == "/cube/scramble") {
        return scrambleCube(body);
    } else if (path == "/cube/move") {
        return applyMove(body);
    } else if (path == "/cube/solve") {
        return solveCube(body);
    } else if (path == "/cube/state") {
        return setCubeState(body);
    }
    
    return createResponse(404, "{\"error\":\"Not found\"}");
}

std::string HTTPServer::handleOPTIONS() {
    return createResponse(200, "");
}

std::string HTTPServer::getStatus() {
    std::stringstream ss;
    ss << "{\"status\":\"running\",\"solver\":\"" << solver_->getName() << "\"}";
    return createResponse(200, ss.str());
}

std::string HTTPServer::getCubeState() {
    return createResponse(200, currentCube_.toJSON());
}

std::string HTTPServer::resetCube() {
    currentCube_.reset();
    return createResponse(200, currentCube_.toJSON());
}

std::string HTTPServer::scrambleCube(const std::string& body) {
    int moves = 20;
    
    // Parse moves count from body if provided
    std::string movesStr = extractJSONValue(body, "moves");
    if (!movesStr.empty()) {
        try {
            moves = std::stoi(movesStr);
        } catch (...) {}
    }
    
    currentCube_.scramble(moves);
    return createResponse(200, currentCube_.toJSON());
}

std::string HTTPServer::applyMove(const std::string& body) {
    std::string move = extractJSONValue(body, "move");
    
    if (move.empty()) {
        return createResponse(400, "{\"error\":\"Move not specified\"}");
    }
    
    try {
        currentCube_.applyMove(move);
        return createResponse(200, currentCube_.toJSON());
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "{\"error\":\"" << e.what() << "\"}";
        return createResponse(400, ss.str());
    }
}

std::string HTTPServer::solveCube(const std::string& body) {
    int maxDepth = 20;
    
    std::string depthStr = extractJSONValue(body, "maxDepth");
    if (!depthStr.empty()) {
        try {
            maxDepth = std::stoi(depthStr);
        } catch (...) {}
    }
    
    auto solution = solver_->solve(currentCube_, maxDepth);
    
    std::stringstream ss;
    ss << "{\"solution\":[";
    for (size_t i = 0; i < solution.size(); ++i) {
        ss << "\"" << solution[i] << "\"";
        if (i < solution.size() - 1) ss << ",";
    }
    ss << "],\"moves\":" << solution.size();
    ss << ",\"nodes\":" << solver_->getNodesExplored();
    ss << ",\"time\":" << solver_->getSolveTime();
    ss << ",\"cube\":" << currentCube_.toJSON() << "}";
    
    return createResponse(200, ss.str());
}

std::string HTTPServer::setCubeState(const std::string& body) {
    std::string state = extractJSONValue(body, "state");
    
    if (state.empty() || state.length() != 54) {
        return createResponse(400, "{\"error\":\"Invalid state\"}");
    }
    
    try {
        currentCube_.fromString(state);
        return createResponse(200, currentCube_.toJSON());
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "{\"error\":\"" << e.what() << "\"}";
        return createResponse(400, ss.str());
    }
}

std::string HTTPServer::createResponse(int status, const std::string& body, const std::string& contentType) {
    std::stringstream ss;
    ss << "HTTP/1.1 " << status << " OK\r\n";
    ss << "Content-Type: " << contentType << "\r\n";
    ss << "Content-Length: " << body.length() << "\r\n";
    ss << "Access-Control-Allow-Origin: *\r\n";
    ss << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
    ss << "Access-Control-Allow-Headers: Content-Type\r\n";
    ss << "\r\n";
    ss << body;
    return ss.str();
}

std::string HTTPServer::extractJSONValue(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos = json.find(':', pos);
    if (pos == std::string::npos) return "";
    
    pos++;
    while (pos < json.length() && std::isspace(json[pos])) pos++;
    
    if (json[pos] == '"') {
        pos++;
        size_t end = json.find('"', pos);
        if (end != std::string::npos) {
            return json.substr(pos, end - pos);
        }
    } else {
        size_t end = pos;
        while (end < json.length() && (std::isdigit(json[end]) || json[end] == '.' || json[end] == '-')) {
            end++;
        }
        return json.substr(pos, end - pos);
    }
    
    return "";
}

std::map<std::string, std::string> HTTPServer::parseJSON(const std::string& json) {
    std::map<std::string, std::string> result;
    // Simple JSON parser (implement as needed)
    return result;
}