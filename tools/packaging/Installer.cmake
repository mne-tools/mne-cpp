#
#  Installer.cmake
#  mne-cpp
#
#  Created by the MNE-CPP Developers on 2026/02/03.
#
#

set(CPACK_GENERATOR "IFW")
set(CPACK_PACKAGE_NAME "MNE-CPP")
set(CPACK_PACKAGE_VENDOR "MNE-CPP Developers")
set(CPACK_PACKAGE_VERSION_MAJOR "${MNE_CPP_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${MNE_CPP_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${MNE_CPP_VERSION_PATCH}")
set(CPACK_PACKAGE_VERSION "${MNE_CPP_VERSION_MAJOR}.${MNE_CPP_VERSION_MINOR}.${MNE_CPP_VERSION_PATCH}")

# Set the path to the Qt Installer Framework
if(NOT DEFINED CPACK_IFW_ROOT)
    set(CPACK_IFW_ROOT $ENV{QtInstallerFramework_DIR})
endif()

include(CPack)
include(CPackIFW)

cpack_add_component(applications
    DISPLAY_NAME "Applications"
    DESCRIPTION "MNE-CPP Applications"
    REQUIRED
)

cpack_add_component(runtime
    DISPLAY_NAME "Runtime Libraries"
    HIDDEN
    REQUIRED
)

cpack_add_component(development
    DISPLAY_NAME "Development Files"
    DESCRIPTION "Headers and CMake configuration files"
    DISABLED
)
