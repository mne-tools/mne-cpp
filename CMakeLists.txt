cmake_minimum_required(VERSION 3.14)
project(src LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

# Save git hash for project
set(MNE_GIT_HASH_SHORT "No git hash")
set(MNE_GIT_HASH_LONG "No git hash")

execute_process(COMMAND git log -1 --format=%h
                OUTPUT_VARIABLE MNE_GIT_HASH_SHORT
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git log -1 --format=%H
                OUTPUT_VARIABLE MNE_GIT_HASH_LONG
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_STRIP_TRAILING_WHITESPACE)

add_subdirectory(src)
