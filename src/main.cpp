#include "http_server.hpp"
#include "rubiks_cube.hpp"
#include "sequential_solver.hpp"
#include <iostream>
#include <csignal>
#include <memory>
#include <mpi.h>

std::unique_ptr<HTTPServer> server;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down server..." << std::endl;
        if (server) {
            server->stop();
        }
        MPI_Finalize(); // Clean MPI
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Only rank 0 should respond to shutdown signals
    if (rank == 0) {
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);
    }

    int port = 8080;
    if (argc > 1 && rank == 0) { // Only rank 0 needs CLI args
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            std::cerr << "Invalid port number" << std::endl;
            MPI_Finalize();
            return 1;
        }
    }

    // Rank 0 starts HTTP server
    if (rank == 0) {
        std::cout << "==================================" << std::endl;
        std::cout << "Rubik's Cube Solver Backend (MPI mode)" << std::endl;
        std::cout << "==================================\n" << std::endl;

        std::cout << "Testing cube implementation..." << std::endl;
        RubiksCube testCube;
        std::cout << "Created solved cube: " << (testCube.isSolved() ? "✓" : "✗") << std::endl;
        testCube.scramble(5);
        std::cout << "Scrambled cube: " << (!testCube.isSolved() ? "✓" : "✗") << std::endl;
        std::cout << "\nStarting HTTP server...\n" << std::endl;

        server = std::make_unique<HTTPServer>(port);
        server->start(); // Blocking loop – Rank 0 stays here
    }
    else {
        // Rank > 0 = Worker ranks (idle until solver uses them)
        // Prevent exit
        while (true) {
            // In future: MPI_Recv task packets here for distributed solving
            MPI_Barrier(MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
