#include "http_server.hpp"
#include "sequential_solver.hpp"

#ifdef HAVE_OPENMP
#include "openmp_solver.hpp"
#endif

#ifdef HAVE_MPI
#include "mpi_solver.hpp"
#include "hybrid_solver.hpp"
#endif

#include <iomanip>
#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

HTTPServer::HTTPServer(int port) 
    : port_(port), serverSocket_(-1), running_(false), currentSolverType_("sequential") {
    // Default to sequential solver
    solver_ = std::make_unique<SequentialSolver>();
    currentCube_.reset();
}

HTTPServer::~HTTPServer() {
    stop();
}

void HTTPServer::setSolver(const std::string& solverType) {
    solver_ = createSolver(solverType);
    currentSolverType_ = solverType;
}

std::string HTTPServer::getCurrentSolver() const {
    return currentSolverType_;
}

std::vector<std::string> HTTPServer::getAvailableSolvers() const {
    std::vector<std::string> solvers = {"sequential"};
    
#ifdef HAVE_OPENMP
    solvers.push_back("openmp");
#endif
    
#ifdef HAVE_MPI
    if (MPISolver::IsInitialized()) {
        solvers.push_back("mpi");
        solvers.push_back("hybrid");
    }
#endif
    
    return solvers;
}

std::unique_ptr<Solver> HTTPServer::createSolver(const std::string& type) {
    std::cout << "Creating solver: " << type << std::endl;
    
    if (type == "sequential") {
        return std::make_unique<SequentialSolver>();
    }
#ifdef HAVE_OPENMP
    else if (type == "openmp") {
        return std::make_unique<OpenMPSolver>(4); // Default 4 threads
    }
#endif
#ifdef HAVE_MPI
    else if (type == "mpi") {
        if (!MPISolver::IsInitialized()) {
            std::cerr << "ERROR: MPI not initialized!" << std::endl;
            std::cerr << "Start the server with: mpirun -np 4 ./rubiks_solver" << std::endl;
            throw std::runtime_error("MPI not initialized. Cannot create MPISolver.");
        }
        return std::make_unique<MPISolver>();
    } else if (type == "hybrid") {
        if (!MPISolver::IsInitialized()) {
            std::cerr << "ERROR: MPI not initialized!" << std::endl;
            std::cerr << "Start the server with: mpirun -np 4 ./rubiks_solver" << std::endl;
            throw std::runtime_error("MPI not initialized. Cannot create HybridSolver.");
        }
        return std::make_unique<HybridSolver>(2); // Default 2 threads per process
    }
#endif
    
    // Default fallback
    std::cerr << "Unknown solver type: " << type << ", falling back to sequential" << std::endl;
    return std::make_unique<SequentialSolver>();
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
    std::cout << "\n========================================" << std::endl;
    std::cout << "Server started on port " << port_ << std::endl;
    std::cout << "Current solver: " << solver_->getName() << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nAPI Endpoints:" << std::endl;
    std::cout << "  GET  /status         - Server status" << std::endl;
    std::cout << "  GET  /cube           - Current cube state" << std::endl;
    std::cout << "  GET  /solvers        - List available solvers" << std::endl;
    std::cout << "  POST /solver/select  - Select solver algorithm" << std::endl;
    std::cout << "  POST /cube/reset     - Reset to solved state" << std::endl;
    std::cout << "  POST /cube/scramble  - Scramble the cube" << std::endl;
    std::cout << "  POST /cube/move      - Apply a move" << std::endl;
    std::cout << "  POST /cube/solve     - Solve the cube" << std::endl;
    std::cout << "  POST /cube/state     - Set cube state" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
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
    
    // Log request
    std::cout << method << " " << path << std::endl;
    
    if (method == "OPTIONS") {
        return handleOPTIONS();
    }
    
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
    } else if (path == "/solvers") {
        return listSolvers();
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
    } else if (path == "/solver/select") {
        return selectSolver(body);
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

std::string HTTPServer::listSolvers() {
    auto solvers = getAvailableSolvers();
    std::stringstream ss;
    ss << "{\"solvers\":[";
    for (size_t i = 0; i < solvers.size(); ++i) {
        ss << "\"" << solvers[i] << "\"";
        if (i < solvers.size() - 1) ss << ",";
    }
    ss << "],\"current\":\"" << currentSolverType_ << "\"}";
    return createResponse(200, ss.str());
}

std::string HTTPServer::selectSolver(const std::string& body) {
    std::string solverType = extractJSONValue(body, "solver");
    
    if (solverType.empty()) {
        return createResponse(400, "{\"error\":\"Solver type not specified\"}");
    }
    
    auto available = getAvailableSolvers();
    bool isAvailable = std::find(available.begin(), available.end(), solverType) != available.end();
    
    if (!isAvailable) {
        std::stringstream ss;
        ss << "{\"error\":\"Solver '" << solverType << "' not available or MPI not initialized\"}";
        return createResponse(400, ss.str());
    }
    
    try {
        setSolver(solverType);
        
        std::stringstream ss;
        ss << "{\"success\":true,\"solver\":\"" << solver_->getName() << "\"}";
        return createResponse(200, ss.str());
    } catch (const std::exception& e) {
        std::stringstream ss;
        ss << "{\"error\":\"" << e.what() << "\"}";
        return createResponse(500, ss.str());
    }
}

std::string HTTPServer::getCubeState() {
    return createResponse(200, currentCube_.toJSON());
}

std::string HTTPServer::resetCube() {
    std::cout << "Resetting cube to solved state" << std::endl;
    currentCube_.reset();
    return createResponse(200, currentCube_.toJSON());
}

std::string HTTPServer::scrambleCube(const std::string& body) {
    int moves = 20;
    
    std::string movesStr = extractJSONValue(body, "moves");
    if (!movesStr.empty()) {
        try {
            moves = std::stoi(movesStr);
        } catch (...) {}
    }
    
    std::cout << "Scrambling cube with " << moves << " moves" << std::endl;
    currentCube_.scramble(moves);
    return createResponse(200, currentCube_.toJSON());
}

std::string HTTPServer::applyMove(const std::string& body) {
    std::string move = extractJSONValue(body, "move");
    
    if (move.empty()) {
        return createResponse(400, "{\"error\":\"Move not specified\"}");
    }
    
    try {
        std::cout << "Applying move: " << move << std::endl;
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
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "SOLVING WITH ALL 4 ALGORITHMS" << std::endl;
    std::cout << "========================================\n" << std::endl;
    
    // Store cube state for each algorithm
    std::string cubeState = currentCube_.toString();
    
    struct AlgorithmResult {
        std::string name;
        std::vector<std::string> solution;
        double time;
        int nodes;
        bool success;
    };
    
    std::vector<AlgorithmResult> results;
    
    // 1. Sequential IDA*
    {
        std::cout << "\n[1/4] Running Sequential IDA*..." << std::endl;
        RubiksCube cube(cubeState);
        SequentialSolver solver;
        auto solution = solver.solve(cube, maxDepth);
        
        AlgorithmResult result;
        result.name = solver.getName();
        result.solution = solution;
        result.time = solver.getSolveTime();
        result.nodes = solver.getNodesExplored();
        result.success = !solution.empty();
        results.push_back(result);
    }
    
    // 2. OpenMP IDA*
#ifdef HAVE_OPENMP
    {
        std::cout << "\n[2/4] Running OpenMP IDA*..." << std::endl;
        RubiksCube cube(cubeState);
        OpenMPSolver solver(4);
        auto solution = solver.solve(cube, maxDepth);
        
        AlgorithmResult result;
        result.name = solver.getName();
        result.solution = solution;
        result.time = solver.getSolveTime();
        result.nodes = solver.getNodesExplored();
        result.success = !solution.empty();
        results.push_back(result);
    }
#endif
    
    // 3. MPI IDA* - Special handling to coordinate all ranks
#ifdef HAVE_MPI
    if (MPISolver::IsInitialized()) {
        std::cout << "\n[3/4] Running MPI IDA*..." << std::endl;
        // std::cout << "[DEBUG] Broadcasting cube state and solve command to all MPI ranks..." << std::endl;
        
        // Get my rank
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        
        // Broadcast solve command (1 = solve with MPI, 0 = skip)
        int solveCommand = 1;
        MPI_Bcast(&solveCommand, 1, MPI_INT, 0, MPI_COMM_WORLD);
        // std::cout << "[DEBUG] Rank 0: Broadcasted solve command" << std::endl;
        
        // Broadcast maxDepth
        MPI_Bcast(&maxDepth, 1, MPI_INT, 0, MPI_COMM_WORLD);
        // std::cout << "[DEBUG] Rank 0: Broadcasted maxDepth=" << maxDepth << std::endl;
        
        // Broadcast cube state length
        int stateLength = cubeState.length();
        MPI_Bcast(&stateLength, 1, MPI_INT, 0, MPI_COMM_WORLD);
        // std::cout << "[DEBUG] Rank 0: Broadcasted state length=" << stateLength << std::endl;
        
        // Broadcast cube state
        char* stateBuffer = new char[stateLength + 1];
        strcpy(stateBuffer, cubeState.c_str());
        MPI_Bcast(stateBuffer, stateLength + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
        // std::cout << "[DEBUG] Rank 0: Broadcasted cube state" << std::endl;
        
        // Now all ranks have the cube state, create solver and solve
        RubiksCube cube(cubeState);
        MPISolver solver;
        
       //  std::cout << "[DEBUG] Rank 0: Calling solver.solve()..." << std::endl;
        auto solution = solver.solve(cube, maxDepth);
       //  std::cout << "[DEBUG] Rank 0: solver.solve() returned" << std::endl;
        
        delete[] stateBuffer;
        
        // Only rank 0 records results
        if (rank == 0) {
            AlgorithmResult result;
            result.name = solver.getName();
            result.solution = solution;
            result.time = solver.getSolveTime();
            result.nodes = solver.getNodesExplored();
            result.success = !solution.empty();
            results.push_back(result);
        }
    }
#endif
    
    // 4. Hybrid MPI+OpenMP IDA* - Special handling
#ifdef HAVE_MPI
    if (MPISolver::IsInitialized()) {
        std::cout << "\n[4/4] Running Hybrid IDA*..." << std::endl;
        // std::cout << "[DEBUG] Broadcasting cube state and solve command to all MPI ranks..." << std::endl;
        
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        
        // Broadcast solve command (2 = solve with Hybrid)
        int solveCommand = 2;
        MPI_Bcast(&solveCommand, 1, MPI_INT, 0, MPI_COMM_WORLD);
       //  std::cout << "[DEBUG] Rank 0: Broadcasted solve command (Hybrid)" << std::endl;
        
        // Broadcast maxDepth
        MPI_Bcast(&maxDepth, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        // Broadcast cube state
        int stateLength = cubeState.length();
        MPI_Bcast(&stateLength, 1, MPI_INT, 0, MPI_COMM_WORLD);
        
        char* stateBuffer = new char[stateLength + 1];
        strcpy(stateBuffer, cubeState.c_str());
        MPI_Bcast(stateBuffer, stateLength + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
       //  std::cout << "[DEBUG] Rank 0: Broadcasted cube state (Hybrid)" << std::endl;
        
        RubiksCube cube(cubeState);
        HybridSolver solver(2);
        
      //   std::cout << "[DEBUG] Rank 0: Calling solver.solve() (Hybrid)..." << std::endl;
        auto solution = solver.solve(cube, maxDepth);
      //   std::cout << "[DEBUG] Rank 0: solver.solve() returned (Hybrid)" << std::endl;
        
        delete[] stateBuffer;
        
        if (rank == 0) {
            AlgorithmResult result;
            result.name = solver.getName();
            result.solution = solution;
            result.time = solver.getSolveTime();
            result.nodes = solver.getNodesExplored();
            result.success = !solution.empty();
            results.push_back(result);
        }
    }
#endif
    
    // Print comparison table (only on rank 0)
    std::cout << "\n========================================" << std::endl;
    std::cout << "RESULTS COMPARISON" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::cout << std::left << std::setw(25) << "Algorithm"
              << std::setw(10) << "Time(s)"
              << std::setw(12) << "Moves"
              << std::setw(12) << "Nodes"
              << std::setw(10) << "Speedup" << std::endl;
    std::cout << std::string(70, '-') << std::endl;
    
    double baseTime = results[0].time;
    
    for (const auto& result : results) {
        if (result.success) {
            double speedup = baseTime / result.time;
            std::cout << std::left << std::setw(25) << result.name
                      << std::setw(10) << std::fixed << std::setprecision(4) << result.time
                      << std::setw(12) << result.solution.size()
                      << std::setw(12) << result.nodes
                      << std::setw(10) << std::fixed << std::setprecision(2) << speedup << "x"
                      << std::endl;
        } else {
            std::cout << std::left << std::setw(25) << result.name
                      << "TIMEOUT/FAILED" << std::endl;
        }
    }
    std::cout << "========================================\n" << std::endl;
    
    // Build JSON response
    std::stringstream ss;
    ss << "{\"results\":[";
    
    for (size_t i = 0; i < results.size(); ++i) {
        const auto& result = results[i];
        
        ss << "{\"name\":\"" << result.name << "\"";
        ss << ",\"success\":" << (result.success ? "true" : "false");
        
        if (result.success) {
            ss << ",\"solution\":[";
            for (size_t j = 0; j < result.solution.size(); ++j) {
                ss << "\"" << result.solution[j] << "\"";
                if (j < result.solution.size() - 1) ss << ",";
            }
            ss << "],\"moves\":" << result.solution.size();
            ss << ",\"time\":" << std::fixed << std::setprecision(6) << result.time;
            ss << ",\"nodes\":" << result.nodes;
            ss << ",\"speedup\":" << std::fixed << std::setprecision(2) << (baseTime / result.time);
        } else {
            ss << ",\"solution\":[],\"moves\":0,\"time\":0,\"nodes\":0,\"speedup\":0";
        }
        
        ss << "}";
        if (i < results.size() - 1) ss << ",";
    }
    
    ss << "],\"cube\":" << currentCube_.toJSON() << "}";
    
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

std::string HTTPServer::createResponse(int status, const std::string& body, 
                                       const std::string& contentType) {
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

std::map<std::string, std::string> HTTPServer::parseJSON(const std::string& /* json */) {
    std::map<std::string, std::string> result;
    // Not implemented - using extractJSONValue instead
    return result;
}