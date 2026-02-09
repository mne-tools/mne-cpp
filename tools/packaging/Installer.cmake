#
#  Installer.cmake
#  mne-cpp
#
#  Created by the MNE-CPP Developers on 2026/02/03.
#

##==============================================================================
## CPack / IFW Installer Configuration
##==============================================================================

set(CPACK_GENERATOR "IFW")
set(CPACK_PACKAGE_NAME "MNE-CPP")
set(CPACK_PACKAGE_VENDOR "MNE-CPP Developers")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "MNE-CPP - Cross-platform tools for MEG/EEG data processing")
set(CPACK_PACKAGE_VERSION_MAJOR "${MNE_CPP_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${MNE_CPP_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${MNE_CPP_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${MNE_CPP_VERSION_MAJOR}.${MNE_CPP_VERSION_MINOR}.${MNE_CPP_VERSION_PATCH}")

set(CPACK_PACKAGE_INSTALL_DIRECTORY "MNE-CPP")
set(CPACK_IFW_PACKAGE_TITLE "MNE-CPP ${CPACK_PACKAGE_VERSION}")
set(CPACK_IFW_PACKAGE_START_MENU_DIRECTORY "MNE-CPP")

# Set the path to the Qt Installer Framework
if(NOT DEFINED CPACK_IFW_ROOT)
    set(CPACK_IFW_ROOT $ENV{QtInstallerFramework_DIR})
endif()

include(CPack)
include(CPackIFW)

##==============================================================================
## Component Definitions
##==============================================================================

# --- 1. Applications (binaries) ---
cpack_add_component(applications
    DISPLAY_NAME "Applications"
    DESCRIPTION "MNE-CPP command-line and GUI applications (mne_scan, mne_analyze, mne_browse, etc.)"
    REQUIRED
)

# --- 2. Runtime Libraries (shared libs, always needed) ---
cpack_add_component(runtime
    DISPLAY_NAME "Runtime Libraries"
    DESCRIPTION "Shared libraries required by MNE-CPP applications"
    HIDDEN
    REQUIRED
)

# --- 3. Development SDK (headers + static archives + CMake config) ---
cpack_add_component(sdk
    DISPLAY_NAME "Development SDK"
    DESCRIPTION "C++ headers, static libraries, and CMake package configuration for building custom applications with MNE-CPP"
    DISABLED
)

# --- 4. MNE Sample Dataset (downloaded post-install) ---
cpack_add_component(sampledata
    DISPLAY_NAME "MNE Sample Dataset"
    DESCRIPTION "Download the MNE sample dataset (~1.5 GB) for testing and tutorials. Sets the MNE_DATASETS_SAMPLE_PATH environment variable."
    DISABLED
)

# --- 5. MNE Python (installed post-install via pip) ---
cpack_add_component(mnepython
    DISPLAY_NAME "MNE Python"
    DESCRIPTION "Install MNE-Python via 'pip install mne'. Requires Python 3 and pip to be available on the system PATH."
    DISABLED
)

# --- 6. Environment / PATH Configuration ---
cpack_add_component(pathconfig
    DISPLAY_NAME "PATH Configuration"
    DESCRIPTION "Add MNE-CPP binary directory to the system PATH and optionally set environment variables for the SDK and sample data."
)

##==============================================================================
## Install post-install / helper scripts
##==============================================================================

set(PACKAGING_SCRIPTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../tools/packaging/scripts")

# Post-install: Download MNE sample dataset
install(
    FILES "${PACKAGING_SCRIPTS_DIR}/download_sample_data.sh"
          "${PACKAGING_SCRIPTS_DIR}/download_sample_data.bat"
    DESTINATION scripts
    COMPONENT sampledata
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# Post-install: Install MNE Python
install(
    FILES "${PACKAGING_SCRIPTS_DIR}/install_mne_python.sh"
          "${PACKAGING_SCRIPTS_DIR}/install_mne_python.bat"
    DESTINATION scripts
    COMPONENT mnepython
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# Post-install: PATH / environment variable configuration
install(
    FILES "${PACKAGING_SCRIPTS_DIR}/configure_environment.sh"
          "${PACKAGING_SCRIPTS_DIR}/configure_environment.bat"
    DESTINATION scripts
    COMPONENT pathconfig
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)

# CMake package configuration for SDK consumers
if(MNE_ENABLE_INSTALLER)
    install(
        FILES "${CMAKE_CURRENT_SOURCE_DIR}/../tools/packaging/MNECPPConfig.cmake"
        DESTINATION lib/cmake/MNE-CPP
        COMPONENT sdk
    )
endif()
