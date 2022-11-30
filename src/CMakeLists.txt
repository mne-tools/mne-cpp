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


option(NO_EXAMPLES "Skip building examples" OFF)
option(NO_APPLICATIONS "Skip building applications" OFF)
option(NO_TESTS "Skip building tests" OFF)

option(LIB_ONLY "Build only the libraries. Equivalent to NO_EXAMPLES NO_APPLICATIONS and NO_TESTS" OFF)

##==============================================================================
## Add subdirecotries

add_subdirectory(libraries)
add_subdirectory(external)

if(NOT NO_APPLICATIONS)
    add_subdirectory(applications)
endif()

if(NOT NO_EXAMPLES)
    add_subdirectory(examples)
endif()

if(NOT NO_TESTS)
    add_subdirectory(testframes)
endif()
