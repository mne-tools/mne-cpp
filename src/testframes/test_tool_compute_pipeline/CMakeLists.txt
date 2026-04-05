cmake_minimum_required(VERSION 3.14)
project(test_tool_compute_pipeline LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(SOURCES test_tool_compute_pipeline.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Test)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
