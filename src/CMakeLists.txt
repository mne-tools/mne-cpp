cmake_minimum_required(VERSION 3.14)
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
