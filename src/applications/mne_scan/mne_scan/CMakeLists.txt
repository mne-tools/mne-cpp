# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Juan GPC <jgarciaprieto@mgh.harvard.edu>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_scan LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets Concurrent Network)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(MNE_SCAN_CORE_SOURCES
    mainsplashscreencloser.cpp
    startupwidget.cpp
    mainsplashscreen.cpp
    pluginscene.cpp
    pluginitem.cpp
    plugingui.cpp
    arrow.cpp
    mainwindow.cpp
    mne_scan.qrc
)

set(MNE_SCAN_CORE_HEADERS
    info.h
    mainsplashscreencloser.h
    startupwidget.h
    mainsplashscreen.h
    pluginscene.h
    pluginitem.h
    plugingui.h
    arrow.h
    mainwindow.h
)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_dsp
    mne_connectivity
    mne_inv
    mne_fwd
    mne_mne
    mne_fiff
    mne_fs
    mne_utils
    mne_math
    mne_events
    mne_disp
    mne_disp3D
    mne_mna
)

# The main window and plugin GUI as a static library, so mne_doc_shots can build the live widget tree
# without a second copy of the sources. The executable only owns main.cpp.
add_library(mne_scan_app_core STATIC ${MNE_SCAN_CORE_SOURCES} ${MNE_SCAN_CORE_HEADERS})
target_include_directories(mne_scan_app_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(mne_scan_app_core PUBLIC
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
    scDisp
    scShared
    scMeas
)
set_target_properties(mne_scan_app_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(RESOURCES
    images/appIcons/mne_scan.icns
)

set_source_files_properties(images/appIcons/mne_scan.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION main.cpp ${RESOURCES})
else()
    add_executable(${PROJECT_NAME} main.cpp ${RESOURCES})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE mne_scan_app_core)

if(NOT BUILD_SHARED_LIBS)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        scan_averaging
        scan_babymeg
        scan_covariance
        scan_fiffsimulator
        scan_ftbuffer
        scan_hpi
        scan_natus
        scan_neuronalconnectivity
        scan_noisereduction
        scan_rtclustmne
        scan_rtfwd
        scan_writetofile)

    target_compile_definitions(mne_scan_app_core PUBLIC STATICBUILD)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER mne-cpp.org
    MACOSX_BUNDLE ${BUILD_MAC_APP_BUNDLE}
    MACOSX_BUNDLE_ICON_FILE mne_scan.icns
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
