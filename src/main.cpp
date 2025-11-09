// src/main.cpp - Correct MPI lifecycle (Initialize once, Finalize once)
#include "http_server.hpp"
#include "rubiks_cube.hpp"
#include "sequential_solver.hpp"
#ifdef HAVE_MPI
#include "mpi_solver.hpp"
#include "hybrid_solver.hpp"
#include <mpi.h>
#endif
#include <iostream>
#include <csignal>
#include <memory>
#include <unistd.h>
#include <iomanip>

std::unique_ptr<HTTPServer> server;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down server..." << std::endl;
        if (server) {
            server->stop();
        }
        // DO NOT finalize MPI here. Let main finalize when exiting.
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    int rank = 0;
    int size = 1;

#ifdef HAVE_MPI
    MPISolver::Initialize(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        std::cout << "MPI initialized successfully" << std::endl;
    }
#endif

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    int port = 8080;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        } catch (...) {
            if (rank == 0) {
                std::cerr << "Invalid port number" << std::endl;
            }
            return 1; // Do NOT finalize here
        }
    }

    // Only rank 0 runs HTTP server
    if (rank == 0) {
        std::cout << "==================================" << std::endl;
        std::cout << "Rubik's Cube Solver Backend" << std::endl;
#ifdef HAVE_MPI
        std::cout << "(MPI mode: " << size << " processes)" << std::endl;
#endif
        std::cout << "==================================\n" << std::endl;

        RubiksCube testCube;
        std::cout << "Created solved cube: " << (testCube.isSolved() ? "✓" : "✗") << std::endl;
        testCube.scramble(5);
        std::cout << "Scrambled cube: " << (!testCube.isSolved() ? "✓" : "✗") << std::endl;

        server = std::make_unique<HTTPServer>(port);
        server->start(); // blocking
    }
#ifdef HAVE_MPI
    else {
        std::cout << "Worker rank " << rank << " waiting for solve commands..." << std::endl;

        while (true) {
            int solveCommand = 0;
            MPI_Bcast(&solveCommand, 1, MPI_INT, 0, MPI_COMM_WORLD);

            if (solveCommand == 0) {
                usleep(100000);
                continue;
            }

            int maxDepth;
            MPI_Bcast(&maxDepth, 1, MPI_INT, 0, MPI_COMM_WORLD);

            int stateLength;
            MPI_Bcast(&stateLength, 1, MPI_INT, 0, MPI_COMM_WORLD);

            char* buffer = new char[stateLength + 1];
            MPI_Bcast(buffer, stateLength + 1, MPI_CHAR, 0, MPI_COMM_WORLD);
            RubiksCube cube{ std::string(buffer) };
            delete[] buffer;

            if (solveCommand == 1) {
                MPISolver solver;
                solver.solve(cube, maxDepth);
            } 
            if (solveCommand == 2) {
                HybridSolver solver(2);
                solver.solve(cube, maxDepth);
                continue;
            }
        }
    }
#endif

#ifdef HAVE_MPI
    MPISolver::Finalize(); 
#endif
    return 0;
}
