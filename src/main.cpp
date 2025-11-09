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
#include <unistd.h>  // for sleep()

std::unique_ptr<HTTPServer> server;

void signalHandler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down server..." << std::endl;
        if (server) {
            server->stop();
        }
#ifdef HAVE_MPI
        if (MPISolver::IsInitialized()) {
            MPISolver::Finalize();
        }
#endif
        exit(0);
    }
}

int main(int argc, char* argv[]) {
    int rank = 0;
    
    // Initialize MPI once for all processes
#ifdef HAVE_MPI
    MPISolver::Initialize(&argc, &argv);
    // Note: HybridSolver uses the same MPI initialization
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
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
#ifdef HAVE_MPI
            if (MPISolver::IsInitialized()) {
                MPISolver::Finalize();
            }
#endif
            return 1;
        }
    }

    // Only rank 0 runs HTTP server
    if (rank == 0) {
        std::cout << "==================================" << std::endl;
        std::cout << "Rubik's Cube Solver Backend" << std::endl;
#ifdef HAVE_MPI
        int size;
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        std::cout << "(MPI mode: " << size << " processes)" << std::endl;
#endif
        std::cout << "==================================\n" << std::endl;

        std::cout << "Testing cube implementation..." << std::endl;
        RubiksCube testCube;
        std::cout << "Created solved cube: " << (testCube.isSolved() ? "✓" : "✗") << std::endl;
        testCube.scramble(5);
        std::cout << "Scrambled cube: " << (!testCube.isSolved() ? "✓" : "✗") << std::endl;
        std::cout << "\nStarting HTTP server...\n" << std::endl;

        server = std::make_unique<HTTPServer>(port);
        server->start(); // Blocking
    }
#ifdef HAVE_MPI
    else {
        // Worker ranks: wait for work
        std::cout << "Worker rank " << rank << " ready and waiting..." << std::endl;
        // In a real implementation, workers would listen for MPI messages
        // For now, just keep them alive
        while (true) {
            sleep(1);
        }
    }
#endif

#ifdef HAVE_MPI
    if (MPISolver::IsInitialized()) {
        MPISolver::Finalize();
    }
#endif
    return 0;
}