cmake_minimum_required(VERSION 3.16)
project(rubiks_cube_solver VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Build unit tests" ON)

# Include directories
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR ${PROJECT_ROOT}/src)
set(INC_DIR ${PROJECT_ROOT}/include)

# Source files for the core library
set(CORE_SOURCES
    ${SRC_DIR}/rubiks_cube.cpp
    ${SRC_DIR}/ida_star_solver.cpp
    ${SRC_DIR}/http_server.cpp
)

# Create core library
add_library(rubiks_core STATIC ${CORE_SOURCES})
target_include_directories(rubiks_core PUBLIC ${INC_DIR})
target_compile_features(rubiks_core PUBLIC cxx_std_17)

# Compiler warnings
target_compile_options(rubiks_core PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
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

# Print configuration
message(STATUS "===================================")
message(STATUS "Rubik's Cube Solver Configuration")
message(STATUS "===================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "===================================")