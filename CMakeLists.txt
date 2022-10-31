cmake_minimum_required(VERSION 3.14)
project(src LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

add_subdirectory(src)
