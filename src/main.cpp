#include "http_server.hpp"
#include "rubiks_cube.hpp"
#include "ida_star_solver.hpp"
#include <iostream>
#include <csignal>
#include <memory>

std::unique_ptr<HTTPServer> server;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down server..." << std::endl;
        if (server) {
            server->stop();
        }
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    // Register signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    int port = 8080;
    
    // Parse command line arguments
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port number" << std::endl;
            return 1;
        }
    }
    
    std::cout << "==================================" << std::endl;
    std::cout << "Rubik's Cube Solver Backend" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << std::endl;
    
    // Test the cube implementation
    std::cout << "Testing cube implementation..." << std::endl;
    RubiksCube testCube;
    std::cout << "Created solved cube: " << (testCube.isSolved() ? "✓" : "✗") << std::endl;
    
    testCube.scramble(5);
    std::cout << "Scrambled cube: " << (!testCube.isSolved() ? "✓" : "✗") << std::endl;
    
    IDAStarSolver solver;
    std::cout << "Testing solver on easy scramble..." << std::endl;
    auto solution = solver.solve(testCube, 10);
    
    if (!solution.empty()) {
        std::cout << "Solver test passed! Found solution with " 
                  << solution.size() << " moves" << std::endl;
    } else {
        std::cout << "Solver test: No solution found (expected for deeper scrambles)" << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Starting HTTP server..." << std::endl;
    
    // Start HTTP server
    server = std::make_unique<HTTPServer>(port);
    server->start();
    
    return 0;
}