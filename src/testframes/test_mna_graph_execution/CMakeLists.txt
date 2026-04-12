cmake_minimum_required(VERSION 3.14)
project(test_mna_graph_execution LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Test Core)

enable_testing()

set(SOURCES
    test_mna_graph_execution.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Test
    Qt6::Core
    mne_mna
    mne_utils
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
