add_executable(test_solver
    ${CMAKE_CURRENT_SOURCE_DIR}/test_solver.cpp
)

target_include_directories(test_solver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(test_solver PRIVATE rubiks_core)

add_test(NAME solver_tests COMMAND test_solver)

message(STATUS "Test executable configured: test_solver")