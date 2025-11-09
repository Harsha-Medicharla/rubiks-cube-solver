cmake_minimum_required(VERSION 3.16)
project(rubiks_cube_solver VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_WITH_OPENMP "Build with OpenMP support" ON)
option(BUILD_WITH_MPI "Build with MPI support" ON)

# Include directories
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR ${PROJECT_ROOT}/src)
set(INC_DIR ${PROJECT_ROOT}/include)

# macOS OpenMP setup
if(APPLE AND BUILD_WITH_OPENMP)
    # Try to find Homebrew's libomp
    execute_process(
        COMMAND brew --prefix libomp
        OUTPUT_VARIABLE LIBOMP_PREFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    if(LIBOMP_PREFIX)
        message(STATUS "Found Homebrew libomp at: ${LIBOMP_PREFIX}")
        set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include")
        set(OpenMP_C_LIB_NAMES "omp")
        set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp -I${LIBOMP_PREFIX}/include")
        set(OpenMP_CXX_LIB_NAMES "omp")
        set(OpenMP_omp_LIBRARY "${LIBOMP_PREFIX}/lib/libomp.dylib")
        
        # Set CMake variables for FindOpenMP
        set(OpenMP_C_INCLUDE_DIR "${LIBOMP_PREFIX}/include")
        set(OpenMP_CXX_INCLUDE_DIR "${LIBOMP_PREFIX}/include")
    endif()
endif()

# Find OpenMP
if(BUILD_WITH_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        message(STATUS "OpenMP found: ${OpenMP_CXX_VERSION}")
        message(STATUS "OpenMP flags: ${OpenMP_CXX_FLAGS}")
    else()
        message(WARNING "OpenMP not found. OpenMP solver will not be available.")
        set(BUILD_WITH_OPENMP OFF)
    endif()
endif()

# Find MPI
if(BUILD_WITH_MPI)
    if(APPLE)
        set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/open-mpi" ${CMAKE_PREFIX_PATH})
        set(MPI_HOME "/opt/homebrew/opt/open-mpi")
        set(MPI_CXX_COMPILER "/opt/homebrew/opt/open-mpi/bin/mpicxx")
        set(MPI_C_COMPILER "/opt/homebrew/opt/open-mpi/bin/mpicc")
    endif()

    find_package(MPI REQUIRED)
    
    if(MPI_CXX_FOUND)
        message(STATUS "MPI found!")
        message(STATUS "MPI include dir: ${MPI_CXX_INCLUDE_PATH}")
        message(STATUS "MPI libs: ${MPI_CXX_LIBRARIES}")
    else()
        message(WARNING "MPI not found. MPI solvers will be disabled.")
        set(BUILD_WITH_MPI OFF)
    endif()
endif()


# Source files for the core library
set(CORE_SOURCES
    ${SRC_DIR}/rubiks_cube.cpp
    ${SRC_DIR}/sequential_solver.cpp
    ${SRC_DIR}/http_server.cpp
)

# Add IDA* solver if it exists
if(EXISTS ${SRC_DIR}/ida_star_solver.cpp)
    list(APPEND CORE_SOURCES ${SRC_DIR}/ida_star_solver.cpp)
    message(STATUS "IDA* solver: INCLUDED")
else()
    message(STATUS "IDA* solver: NOT FOUND (skipping)")
endif()

# Add OpenMP solver if available
if(BUILD_WITH_OPENMP AND OpenMP_CXX_FOUND)
    list(APPEND CORE_SOURCES ${SRC_DIR}/openmp_solver.cpp)
endif()

# Add MPI solvers if available
if(BUILD_WITH_MPI AND MPI_CXX_FOUND)
    list(APPEND CORE_SOURCES 
        ${SRC_DIR}/mpi_solver.cpp
        ${SRC_DIR}/hybrid_solver.cpp
    )
endif()

# Create core library
add_library(rubiks_core STATIC ${CORE_SOURCES})
target_include_directories(rubiks_core PUBLIC ${INC_DIR})
target_compile_features(rubiks_core PUBLIC cxx_std_17)

# Link OpenMP if available
if(BUILD_WITH_OPENMP AND OpenMP_CXX_FOUND)
    target_link_libraries(rubiks_core PUBLIC OpenMP::OpenMP_CXX)
    target_compile_definitions(rubiks_core PUBLIC HAVE_OPENMP)
endif()

# Link MPI if available
if(BUILD_WITH_MPI AND MPI_CXX_FOUND)
    target_link_libraries(rubiks_core PUBLIC MPI::MPI_CXX)
    target_compile_definitions(rubiks_core PUBLIC HAVE_MPI)
endif()

# Compiler warnings
target_compile_options(rubiks_core PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# Platform-specific libraries
if(UNIX AND NOT APPLE)
    target_link_libraries(rubiks_core PUBLIC pthread)
endif()

# Main executable
add_executable(rubiks_solver ${SRC_DIR}/main.cpp)
target_link_libraries(rubiks_solver PRIVATE rubiks_core)
target_include_directories(rubiks_solver PRIVATE ${INC_DIR})

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(${PROJECT_ROOT}/tests)
endif()

# Install targets
install(TARGETS rubiks_core rubiks_solver
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Print configuration summary
message(STATUS "===================================")
message(STATUS "Rubik's Cube Solver Configuration")
message(STATUS "===================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "OpenMP support: ${BUILD_WITH_OPENMP}")
message(STATUS "MPI support: ${BUILD_WITH_MPI}")
message(STATUS "===================================")
message(STATUS "Available Solvers:")
message(STATUS "  - Sequential (Brute-Force)")
if(EXISTS ${SRC_DIR}/ida_star_solver.cpp)
    message(STATUS "  - IDA* (Original)")
endif()
if(BUILD_WITH_OPENMP AND OpenMP_CXX_FOUND)
    message(STATUS "  - OpenMP")
endif()
if(BUILD_WITH_MPI AND MPI_CXX_FOUND)
    message(STATUS "  - MPI")
    if(BUILD_WITH_OPENMP AND OpenMP_CXX_FOUND)
        message(STATUS "  - Hybrid (MPI+OpenMP)")
    endif()
endif()
message(STATUS "===================================")