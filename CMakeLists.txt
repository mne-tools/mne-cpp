cmake_minimum_required(VERSION 3.14)
project(mne_cpp LANGUAGES CXX)

##==============================================================================
## Save git hash for project

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

##==============================================================================
## Set global build options

set(CMAKE_CXX_STANDARD 14)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_MAC_APP_BUNDLE "Build app bundle on macos" OFF)

##==============================================================================
## Add subdirecotries

add_subdirectory(src)
