# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(scan_tmsi LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    tmsi.cpp 
    tmsi_global.cpp 
    FormFiles/tmsisetupwidget.cpp 
    FormFiles/tmsimanualannotationwidget.cpp 
    FormFiles/tmsiimpedancewidget.cpp 
    tmsielectrodeitem.cpp 
    tmsiimpedanceview.cpp 
    tmsiimpedancescene.cpp 
    FormFiles/tmsisetupprojectwidget.cpp
)

set(HEADERS
    tmsi.h
    tmsi_global.h 
    FormFiles/tmsisetupwidget.h 
    FormFiles/tmsimanualannotationwidget.h 
    FormFiles/tmsiimpedancewidget.h 
    tmsielectrodeitem.h 
    tmsiimpedanceview.h 
    tmsiimpedancescene.h 
    FormFiles/tmsisetupprojectwidget.h
)

set(UI
    FormFiles/tmsisetup.ui
    FormFiles/tmsimanualannotation.ui
    FormFiles/tmsiimpedancewidget.ui
    FormFiles/tmsisetupprojectwidget.ui
)

set(RESOURCES
    tmsi.qrc    
)

set(FILE_TO_UPDATE tmsi_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES} ${UI})

target_include_directories(${PROJECT_NAME} PUBLIC ../)
target_include_directories(${PROJECT_NAME} PRIVATE .)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED 
    mne_acq_devices
    mne_acq_fiff
    mne_disp
    mne_channels
    mne_base
    mne_math
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
    mne_inv
    mne_dsp
    mne_connectivity
    mne_events
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
    scDisp
    scShared
    scMeas)

target_compile_definitions(${PROJECT_NAME} PRIVATE SCAN_TMSI_PLUGIN)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD QT_STATICPLUGIN)
endif()
