cmake_minimum_required(VERSION 3.15)
project(mne_cpp
  LANGUAGES CXX
)

##==============================================================================
## Global build options

set(CMAKE_CXX_STANDARD 14)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
option(BUILD_MAC_APP_BUNDLE "Build app bundle on macos" OFF)

option(NO_EXAMPLES "Skip building examples" OFF)
option(NO_APPLICATIONS "Skip building applications" OFF)
option(NO_TESTS "Skip building tests" OFF)

# Equivalent to NO_EXAMPLES, NO_APPLICATIONS and NO_TESTS
option(LIB_ONLY "Build only the libraries." OFF)

# Equivalent to NO_EXAMPLES and NO_TESTS
option(FOR_RELEASE "Build only libs and apps." OFF)

option(WITH_CODE_COV "Build with code coverage enabled" OFF)
if(WITH_CODE_COV)
  add_compile_options("--coverage")
  add_link_options("--coverage")
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
## Save git hash for project

set(MNE_GIT_HASH_SHORT "No git hash")
set(MNE_GIT_HASH_LONG "No git hash")

execute_process(COMMAND git log -1 --format=%h
  OUTPUT_VARIABLE MNE_GIT_HASH_SHORT
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git log -1 --format=%H
  OUTPUT_VARIABLE MNE_GIT_HASH_LONG
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_STRIP_TRAILING_WHITESPACE)

##==============================================================================
## Add subdirecotries

add_subdirectory(libraries)
add_subdirectory(external)

if(NOT LIB_ONLY)
  if(NOT NO_APPLICATIONS)
    add_subdirectory(applications)
  endif()
  if(NOT FOR_RELEASE)
    if(NOT NO_EXAMPLES)
      add_subdirectory(examples)
    endif()
    if(NOT NO_TESTS)
      add_subdirectory(testframes)
    endif()
  endif()
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


if(WIN32)
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BINARY_OUTPUT_DIRECTORY}
  )
  if(${CMAKE_VERSION} VERSION_LESS "3.26.0")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy_directory
      ${PROJECT_RESOURCES_DIR} ${OUTPUT_RESOURCES_DIR}
    )
  else()
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
      ${PROJECT_RESOURCES_DIR} ${OUTPUT_RESOURCES_DIR}
    )
  endif()
  
else()
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BINARY_OUTPUT_DIRECTORY}
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${PROJECT_RESOURCES_DIR} ${OUTPUT_RESOURCES_DIR}
  )
endif()
