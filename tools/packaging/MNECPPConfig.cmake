#
#  MNECPPConfig.cmake
#  MNE-CPP CMake Package Configuration
#
#  This file enables downstream projects to use:
#    find_package(MNE-CPP REQUIRED)
#
#  After a successful find_package(), the following variables are set:
#    MNE_CPP_FOUND        - TRUE
#    MNE_CPP_INCLUDE_DIRS - Include directories for MNE-CPP headers
#    MNE_CPP_LIBRARY_DIRS - Library directories
#    MNE_CPP_LIBRARIES    - List of all MNE-CPP libraries to link against
#    MNE_CPP_VERSION      - Version string
#
#  Usage in downstream CMakeLists.txt:
#    find_package(MNE-CPP REQUIRED)
#    target_include_directories(my_app PRIVATE ${MNE_CPP_INCLUDE_DIRS})
#    target_link_libraries(my_app PRIVATE ${MNE_CPP_LIBRARIES})
#

cmake_minimum_required(VERSION 3.14)

# Compute installation prefix relative to this file's location
# This file is installed at: <prefix>/lib/cmake/MNE-CPP/MNECPPConfig.cmake
get_filename_component(MNE_CPP_INSTALL_PREFIX "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(MNE_CPP_FOUND TRUE)
set(MNE_CPP_VERSION "2.0.0")
set(MNE_CPP_INCLUDE_DIRS "${MNE_CPP_INSTALL_PREFIX}/include")
set(MNE_CPP_LIBRARY_DIRS "${MNE_CPP_INSTALL_PREFIX}/lib")

# List of all MNE-CPP libraries (in dependency order)
set(MNE_CPP_LIBRARIES
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
    mne_inverse
    mne_communication
    mne_rtprocessing
    mne_connectivity
    mne_events
    mne_disp
    mne_disp3D_rhi
)

# Add library directory to linker search path
link_directories(${MNE_CPP_LIBRARY_DIRS})

# Verify that the installation looks valid
if(NOT EXISTS "${MNE_CPP_INCLUDE_DIRS}")
    set(MNE_CPP_FOUND FALSE)
    if(MNE-CPP_FIND_REQUIRED)
        message(FATAL_ERROR
            "MNE-CPP include directory not found: ${MNE_CPP_INCLUDE_DIRS}\n"
            "The Development SDK component may not have been installed.\n"
            "Please reinstall MNE-CPP with the 'Development SDK' option enabled."
        )
    endif()
endif()

if(NOT EXISTS "${MNE_CPP_LIBRARY_DIRS}")
    set(MNE_CPP_FOUND FALSE)
    if(MNE-CPP_FIND_REQUIRED)
        message(FATAL_ERROR
            "MNE-CPP library directory not found: ${MNE_CPP_LIBRARY_DIRS}\n"
            "Please check your MNE-CPP installation."
        )
    endif()
endif()

if(MNE_CPP_FOUND)
    message(STATUS "Found MNE-CPP ${MNE_CPP_VERSION} at ${MNE_CPP_INSTALL_PREFIX}")
    message(STATUS "  Include dirs: ${MNE_CPP_INCLUDE_DIRS}")
    message(STATUS "  Library dirs: ${MNE_CPP_LIBRARY_DIRS}")
endif()
