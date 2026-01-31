cmake_minimum_required(VERSION 3.10)
project(ex_brain_view)

set(MNE_CPP_EXAMPLE_TARGET ex_brain_view)

# Find Qt
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets ShaderTools)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

message(WARNING "DEBUG: QT_VERSION_MAJOR = ${QT_VERSION_MAJOR}")
if(TARGET Qt6::Gui)
    message(WARNING "DEBUG: Target Qt6::Gui exists")
else()
    message(WARNING "DEBUG: Target Qt6::Gui DOES NOT EXIST")
endif()
if(TARGET Qt::Gui)
    message(WARNING "DEBUG: Target Qt::Gui exists")
endif()

# Define the source files
set(SOURCES
    main.cpp
    brainview.cpp
    brainrenderer.cpp
    brainsurface.cpp
)

# Add the executable unconditionally for debugging
add_executable(${MNE_CPP_EXAMPLE_TARGET} ${SOURCES})
# Qt6 finalization handled manually if needed, but qt_add_executable is preferred.
# Turning MANUAL_FINALIZATION off for add_executable basic case.
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    target_link_libraries(${MNE_CPP_EXAMPLE_TARGET} PRIVATE Qt6::Core)
    qt_finalize_executable(${MNE_CPP_EXAMPLE_TARGET})
endif()

# Manual fix for QRhi private headers in Qt 6.10
if(TARGET Qt6::Gui)
    # Attempt to locate versioned headers relative to Qt6_DIR
    # Qt6_DIR is usually <QT_PREFIX>/lib/cmake/Qt6
    # Headers are usually <QT_PREFIX>/lib/QtGui.framework/Headers/<VER>/QtGui
    
    message(WARNING "DEBUG: Qt6_DIR = ${Qt6_DIR}")
    message(WARNING "DEBUG: Qt6_VERSION = ${Qt6_VERSION}")
    
    find_path(QRHI_PRIVATE_HEADER_DIR "rhi/qrhi.h"
        HINTS
        "${Qt6_DIR}/../../../QtGui.framework/Headers/${Qt6_VERSION}/QtGui"
        "${Qt6_DIR}/../../../QtGui.framework/Versions/A/Headers/${Qt6_VERSION}/QtGui"
        "${Qt6_DIR}/../../../include/QtGui/${Qt6_VERSION}/QtGui"
        "/Users/christoph.dinh/Qt/6.10.2/macos/lib/QtGui.framework/Headers/6.10.2/QtGui"
        NO_DEFAULT_PATH
    )
    
    if(QRHI_PRIVATE_HEADER_DIR)
        message(STATUS "Found QRhi headers: ${QRHI_PRIVATE_HEADER_DIR}")
        target_include_directories(${MNE_CPP_EXAMPLE_TARGET} PRIVATE ${QRHI_PRIVATE_HEADER_DIR})
    else()
        message(WARNING "Could not find rhi/qrhi.h. Please ensure private headers are installed.")
    endif()
endif()

# Add shaders (Interface library trick)
add_library(brain_shaders INTERFACE)
qt_add_shaders(${MNE_CPP_EXAMPLE_TARGET} "brain_shaders"
    PREFIX
        "/"
    BASE
        "shaders"
    FILES
        "shaders/standard.vert"
        "shaders/standard.frag"
        "shaders/holographic.vert"
        "shaders/holographic.frag"
        "shaders/glossy.vert"
        "shaders/glossy.frag"
)

# Link against MNE-CPP libraries and Qt
target_link_libraries(${MNE_CPP_EXAMPLE_TARGET} PRIVATE
    mne_fs
    mne_fiff
    mne_utils
    eigen
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

# Set properties
set_target_properties(${MNE_CPP_EXAMPLE_TARGET} PROPERTIES
    FOLDER "Examples"
)

# Install
if(MNE_CPP_INSTALL_EXAMPLES)
    install(TARGETS ${MNE_CPP_EXAMPLE_TARGET}
        RUNTIME DESTINATION bin
        BUNDLE DESTINATION bin
    )
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${MNE_CPP_EXAMPLE_TARGET})
endif()
