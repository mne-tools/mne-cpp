cmake_minimum_required(VERSION 3.15)
project(mne_cpp LANGUAGES CXX)

##==============================================================================
## Global build options

set(CMAKE_CXX_STANDARD 14)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_MAC_APP_BUNDLE "Build app bundle on macos" OFF)

option(BUILD_EXAMPLES "Build examples" OFF)
option(BUILD_APPLICATIONS "Build applications" ON)
option(BUILD_TESTS "Build tests" OFF)

option(BUILD_ALL "Build entire project" OFF)

if(BUILD_ALL)
  set(BUILD_EXAMPLES ON)
  set(BUILD_TESTS ON)
  set(BUILD_APPLICATIONS ON)
endif()

option(WITH_CODE_COV "Build with code coverage enabled" OFF)
if(WITH_CODE_COV)
  add_compile_options("--coverage")
  add_link_options("--coverage")
endif()

option(NO_IPC "Build project with no interprocess communication features (shared memory)" OFF)

if(NO_IPC)
    add_compile_definitions(NO_IPC)
endif()


##==============================================================================
## FFTW Options

option(USE_FFTW "Use fftw backend for eigen" OFF)

set(FFTW_DIR_LIBS "${PROJECT_SOURCE_DIR}/external/fftw")
set(FFTW_DIR_INCLUDE "${PROJECT_SOURCE_DIR}/external/fftw")
if(USE_FFTW)
  set(EIGEN_FFTW_DEFAULT)
endif()

##==============================================================================
## WASM

option(WASM "Setup build for wasm" OFF)

if(WASM)
  add_compile_definitions(WASMBUILD)
  # set(CMAKE_PREFIX_PATH ${WASM_QT_PATH})
  # message("CPP - ${CMAKE_PREFIX_PATH}")
  set(BUILD_SHARED_LIBS OFF)
  set(NO_TESTS ON)
  set(NO_EXAMPLES ON)
endif()

##==============================================================================
## Misc. settings

# Generate json file detailing project compilation. (Typically for LSPs)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# If no build type is specified, default to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
  message("-- [MNE-CPP] Build type not defined."
    "Using default value:"
    "${CMAKE_BUILD_TYPE}"
  )
endif()

if(NOT DEFINED CMAKE_BINARY_DIR)
  set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/../build/${CMAKE_BUILD_TYPE})
endif()

if(NOT DEFINED BINARY_OUTPUT_DIRECTORY)
  set(BINARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../out/${CMAKE_BUILD_TYPE})
  message(
    "-- [MNE-CPP] Build name not defined."
    " Using default value: ${BINARY_OUTPUT_DIRECTORY}"
  )
endif()

##==============================================================================
## Dissalow in-soure builds by default

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  if(NOT DEFINED FORCE_IN_SOURCE_BUILD)
    message(FATAL_ERROR
      "We dissalow in-sourec builds by default because they are typically"
      " a bad idea."
      "\n Remove \"${CMAKE_SOURCE_DIR}/CMakeCache.txt\" and "
      "\"${CMAKE_SOURCE_DIR}/CMakeFiles\" and try again from another folder:"
      "\n "
      "\n mkdir build"
      "\n cmake -B build -S src"
      "\n "
      "\n If you are absolutley sure of what you are doing, you can pass in "
      "FORCE_IN_SOURCE_BUILD to CMake to force in-soure building "
      "(not recommended!)"
    )
  endif()
endif()

##==============================================================================
find_package(QT NAMES Qt6 Qt5)
message("QT_DIR - ${QT_DIR}")

##==============================================================================
## Save git hash for project

set(MNE_GIT_HASH_SHORT "No git hash")
set(MNE_GIT_HASH_LONG "No git hash")

execute_process(
  COMMAND git log -1 --format=%h
  OUTPUT_VARIABLE MNE_GIT_HASH_SHORT
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND git log -1 --format=%H
  OUTPUT_VARIABLE MNE_GIT_HASH_LONG
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE)

##==============================================================================
## Add subdirectories

add_subdirectory(libraries)
add_subdirectory(external)

if(BUILD_APPLICATIONS)
  add_subdirectory(applications)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(BUILD_TESTS)
  add_subdirectory(testframes)
endif()
  
##==============================================================================
## Add symbolic links to project resources folder

set(PROJECT_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../resources)
set(OUTPUT_RESOURCES_DIR ${BINARY_OUTPUT_DIRECTORY}/resources)

cmake_path(
  NORMAL_PATH     PROJECT_RESOURCES_DIR
  OUTPUT_VARIABLE PROJECT_RESOURCES_DIR
)

cmake_path(
  NORMAL_PATH     OUTPUT_RESOURCES_DIR
  OUTPUT_VARIABLE OUTPUT_RESOURCES_DIR
)

message(STATUS "PROJECT_RESOURCES_DIR=${PROJECT_RESOURCES_DIR}")
message(STATUS "OUTPUT_RESOURCES_DIR=${OUTPUT_RESOURCES_DIR}")

execute_process(
  COMMAND ${CMAKE_COMMAND} -E make_directory ${BINARY_OUTPUT_DIRECTORY}
)
if(WIN32)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_RESOURCES_DIR} ${OUTPUT_RESOURCES_DIR}
  )
else()
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${PROJECT_RESOURCES_DIR} ${OUTPUT_RESOURCES_DIR}
  )
endif()

