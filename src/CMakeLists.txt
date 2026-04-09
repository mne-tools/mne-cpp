
cmake_minimum_required(VERSION 3.15)
project(mne_cpp LANGUAGES CXX)

set(MNE_CPP_VERSION_MAJOR 2)
set(MNE_CPP_VERSION_MINOR 1)
set(MNE_CPP_VERSION_PATCH 0)
set(MNE_CPP_VERSION "${MNE_CPP_VERSION_MAJOR}.${MNE_CPP_VERSION_MINOR}.${MNE_CPP_VERSION_PATCH}")

# Make the project version available as a compile definition for CLI tools
add_compile_definitions(MNE_CPP_VERSION="${MNE_CPP_VERSION}")


##==============================================================================
## Global build options

set(CMAKE_CXX_STANDARD 20)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)
if(APPLE)
    option(BUILD_MAC_APP_BUNDLE "Build app bundle on macos" ON)
else()
    option(BUILD_MAC_APP_BUNDLE "Build app bundle on macos" OFF)
endif()

option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_APPLICATIONS "Build applications" ON)
option(BUILD_TOOLS "Build CLI tools (when BUILD_APPLICATIONS is ON)" ON)
option(BUILD_TESTS "Build tests" OFF)

option(BUILD_ALL "Build entire project" OFF)

option(WITH_CODE_COV "Build with code coverage enabled" OFF)

option(NO_IPC "Build project with no interprocess communication features (shared memory)" OFF)

option(WASM "Setup build for wasm" OFF)

option(USE_FFTW "Use fftw backend for eigen" OFF)

set(MNE_LOG_LEVEL "INFO" CACHE STRING "Minimum log level: DEBUG, INFO, WARN, CRIT, FATAL")
set_property(CACHE MNE_LOG_LEVEL PROPERTY STRINGS DEBUG INFO WARN CRIT FATAL)

set(MNE_LOG_MODE "PLAIN" CACHE STRING "Log output mode: PLAIN (raw text), FORMATTED (colored level prefixes)")
set_property(CACHE MNE_LOG_MODE PROPERTY STRINGS PLAIN FORMATTED)

##==============================================================================
## Set up compilation based on options

if(BUILD_ALL)
  set(BUILD_EXAMPLES ON)
  set(BUILD_TESTS ON)
  set(BUILD_APPLICATIONS ON)
  set(BUILD_TOOLS ON)
endif()

if(WASM)
  add_compile_definitions(WASMBUILD)
  set(BUILD_SHARED_LIBS OFF)
  set(NO_TESTS ON)
  set(NO_EXAMPLES ON)
  set(NO_IPC ON)
endif()

if(NO_IPC)
    add_compile_definitions(NO_IPC)
endif()

# Map MNE_LOG_LEVEL string to numeric compile definition
# 0=DEBUG, 1=INFO, 2=WARN, 3=CRIT, 4=FATAL
if(MNE_LOG_LEVEL STREQUAL "DEBUG")
    add_compile_definitions(MNE_LOG_LEVEL=0)
elseif(MNE_LOG_LEVEL STREQUAL "INFO")
    add_compile_definitions(MNE_LOG_LEVEL=1)
elseif(MNE_LOG_LEVEL STREQUAL "WARN")
    add_compile_definitions(MNE_LOG_LEVEL=2)
elseif(MNE_LOG_LEVEL STREQUAL "CRIT")
    add_compile_definitions(MNE_LOG_LEVEL=3)
elseif(MNE_LOG_LEVEL STREQUAL "FATAL")
    add_compile_definitions(MNE_LOG_LEVEL=4)
else()
    message(WARNING "Unknown MNE_LOG_LEVEL '${MNE_LOG_LEVEL}', defaulting to INFO")
    add_compile_definitions(MNE_LOG_LEVEL=1)
endif()
message(STATUS "[MNE-CPP] Log level: ${MNE_LOG_LEVEL}")

# Map MNE_LOG_MODE to compile definition
if(MNE_LOG_MODE STREQUAL "FORMATTED")
    add_compile_definitions(MNE_LOG_MODE_FORMATTED)
endif()
message(STATUS "[MNE-CPP] Log mode: ${MNE_LOG_MODE}")

set(FFTW_DIR_LIBS "${PROJECT_SOURCE_DIR}/external/fftw")
set(FFTW_DIR_INCLUDE "${PROJECT_SOURCE_DIR}/external/fftw")
if(USE_FFTW)
  set(EIGEN_FFTW_DEFAULT)
endif()

# NOTE: WITH_CODE_COV coverage flags are applied in src/libraries/,
# src/applications/, and src/tools/ CMakeLists.txt so that gcov data
# is generated for libraries, applications, and CLI tools.

if(MSVC)
  add_compile_options(/EHsc /MP)
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
  set(CMAKE_BINARY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../build/${CMAKE_BUILD_TYPE})
endif()

if(NOT DEFINED BINARY_OUTPUT_DIRECTORY)
  set(BINARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../out/${CMAKE_BUILD_TYPE})
  message(
    "-- [MNE-CPP] Build name not defined."
    " Using default value: ${BINARY_OUTPUT_DIRECTORY}"
  )
endif()

##==============================================================================
## Disallow in-source builds by default

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
  if(NOT DEFINED FORCE_IN_SOURCE_BUILD)
    message(FATAL_ERROR
      "We disallow in-source builds by default because they are typically"
      " a bad idea."
      "\n Remove \"${CMAKE_SOURCE_DIR}/CMakeCache.txt\" and "
      "\"${CMAKE_SOURCE_DIR}/CMakeFiles\" and try again from another folder:"
      "\n "
      "\n mkdir build"
      "\n cmake -S . -B build"
      "\n "
      "\n If you are absolutely sure of what you are doing, you can pass in "
      "FORCE_IN_SOURCE_BUILD to CMake to force in-source building "
      "(not recommended!)"
    )
  endif()
endif()

##==============================================================================
find_package(QT NAMES Qt6 Qt5)
message("QT_DIR - ${QT_DIR}")

# ── Pre-find all Qt components once ──────────────────────────────────────────
# Qt6::Gui (pulled in transitively by Widgets) checks for WrapVulkanHeaders
# every time find_package(Qt6 …) is called.  Since 40+ subdirectory
# CMakeLists.txt each call find_package, the warning
#   "Could NOT find WrapVulkanHeaders (missing: Vulkan_INCLUDE_DIR)"
# would be printed dozens of times.  Finding all required components here
# makes the subdirectory calls no-ops and prints the warning at most once.

# Define required Qt components for MNE-CPP (Qt6 only, no OpenGL, require GuiPrivate for QRhi)
set(_MNE_QT_COMPONENTS Core Concurrent Gui Network Widgets Svg Xml)
if(NOT WASM)
  list(APPEND _MNE_QT_COMPONENTS PrintSupport)
endif()
if(QT_VERSION_MAJOR EQUAL 6)
  list(APPEND _MNE_QT_COMPONENTS ShaderTools GuiPrivate)
endif()
if(BUILD_TESTS)
  list(APPEND _MNE_QT_COMPONENTS Test)
endif()
list(REMOVE_DUPLICATES _MNE_QT_COMPONENTS)
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${_MNE_QT_COMPONENTS})
unset(_MNE_QT_COMPONENTS)

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
## Define resource directory (before subdirectories so they can use it)

set(PROJECT_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../resources)

##==============================================================================
## SDK export set — disabled for WASM (Qt resource targets cannot be exported).
if(NOT WASM)
    set(MNE_CPP_EXPORT_SET EXPORT MNECPPTargets)
endif()

##==============================================================================
## Eigen dependency

set(MNE_CPP_EIGEN_VERSION "5.0.1" CACHE STRING "Preferred Eigen package version for MNE-CPP.")
set(MNE_CPP_PREBUILT_EIGEN_DIR "${PROJECT_SOURCE_DIR}/external/eigen"
    CACHE PATH "Location of a prebuilt Eigen package prepared by the init scripts or CI helpers.")

find_package(Eigen3 ${MNE_CPP_EIGEN_VERSION} QUIET NO_MODULE
    HINTS
        "${MNE_CPP_PREBUILT_EIGEN_DIR}"
        "${PROJECT_SOURCE_DIR}/external/eigen-${MNE_CPP_EIGEN_VERSION}-install")

if(TARGET Eigen3::Eigen)
    if(NOT TARGET eigen)
        add_library(eigen INTERFACE)
        target_link_libraries(eigen INTERFACE Eigen3::Eigen)
    endif()

    if(DEFINED Eigen3_DIR AND NOT Eigen3_DIR STREQUAL "")
        get_filename_component(MNE_CPP_EIGEN_PACKAGE_ROOT "${Eigen3_DIR}/../../.." ABSOLUTE)
        message(STATUS "[MNE-CPP] Using Eigen package from ${MNE_CPP_EIGEN_PACKAGE_ROOT}")
    else()
        message(STATUS "[MNE-CPP] Using Eigen package found via CMake package discovery.")
    endif()
else()
    message(FATAL_ERROR
        "Could not find Eigen3 ${MNE_CPP_EIGEN_VERSION}.\n"
        "Run ./init.sh (or .\\init.bat on Windows) to fetch the MNE-CPP-maintained dependency artifacts,\n"
        "or install Eigen ${MNE_CPP_EIGEN_VERSION} on your system and set CMAKE_PREFIX_PATH accordingly."
    )
endif()

##==============================================================================
## Add subdirectories

add_subdirectory(libraries)

if(BUILD_APPLICATIONS)
  add_subdirectory(applications)
endif()

if(BUILD_APPLICATIONS AND BUILD_TOOLS)
  add_subdirectory(tools)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()


if(BUILD_TESTS)
  add_subdirectory(testframes)
endif()

option(MNE_ENABLE_INSTALLER "Enable building the installer" OFF)
if(MNE_ENABLE_INSTALLER)
    include(${CMAKE_CURRENT_SOURCE_DIR}/../scripts/packaging/Installer.cmake)
endif()

##==============================================================================
## CMake package config for find_package(MNE-CPP) — SDK discovery
## (Not applicable for WASM builds which use a bundled Eigen target and
##  Qt-generated resource targets that cannot be exported.)

if(NOT WASM)

include(CMakePackageConfigHelpers)

if(MNE_CPP_EIGEN_MODE STREQUAL "prebuilt" AND DEFINED MNE_CPP_EIGEN_PACKAGE_ROOT)
    if(EXISTS "${MNE_CPP_EIGEN_PACKAGE_ROOT}/include/eigen3")
        install(DIRECTORY "${MNE_CPP_EIGEN_PACKAGE_ROOT}/include/eigen3"
            DESTINATION include
            COMPONENT sdk
        )
    endif()

    if(EXISTS "${MNE_CPP_EIGEN_PACKAGE_ROOT}/share/eigen3")
        install(DIRECTORY "${MNE_CPP_EIGEN_PACKAGE_ROOT}/share/eigen3"
            DESTINATION share
            COMPONENT sdk
        )
    endif()

    if(EXISTS "${MNE_CPP_EIGEN_PACKAGE_ROOT}/share/pkgconfig/eigen3.pc")
        install(FILES "${MNE_CPP_EIGEN_PACKAGE_ROOT}/share/pkgconfig/eigen3.pc"
            DESTINATION share/pkgconfig
            COMPONENT sdk
        )
    endif()
endif()

# Generate MNE-CPPConfig.cmake from template
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/packaging/MNECPPConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MNE-CPPConfig.cmake"
    INSTALL_DESTINATION lib/cmake/MNE-CPP
)

# Generate version file for find_package(MNE-CPP 2.1.0)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MNE-CPPConfigVersion.cmake"
    VERSION ${MNE_CPP_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the export set (MNECPPTargets.cmake with imported targets)
install(EXPORT MNECPPTargets
    FILE MNECPPTargets.cmake
    NAMESPACE MNE-CPP::
    DESTINATION lib/cmake/MNE-CPP
    COMPONENT sdk
)

# Install the config and version files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MNE-CPPConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MNE-CPPConfigVersion.cmake"
    DESTINATION lib/cmake/MNE-CPP
    COMPONENT sdk
)

endif() # NOT WASM

  
##==============================================================================
## Add symbolic links to project resources folder

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
