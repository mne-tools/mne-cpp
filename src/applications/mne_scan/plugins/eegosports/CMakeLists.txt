# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(scan_eegosports LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets Svg)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    FormFiles/eegosportsimpedancewidget.cpp 
    eegosports.cpp 
    eegosports_global.cpp 
    eegosportselectrodeitem.cpp 
    eegosportsimpedancescene.cpp 
    eegosportsimpedanceview.cpp 
    FormFiles/eegosportssetupwidget.cpp 
    FormFiles/eegosportssetupprojectwidget.cpp 
)

set(HEADERS
    FormFiles/eegosportsimpedancewidget.h 
    eegosports.h
    eegosports_global.h 
    eegosportselectrodeitem.h 
    eegosportsimpedancescene.h 
    eegosportsimpedanceview.h 
    FormFiles/eegosportssetupwidget.h 
    FormFiles/eegosportssetupprojectwidget.h 
)

set(UI
    FormFiles/eegosportsimpedancewidget.ui
    FormFiles/eegosportssetup.ui
    FormFiles/eegosportssetupprojectwidget.ui
)

set(RESOURCES
    eegosports.qrc    
)

set(FILE_TO_UPDATE eegosports_global.cpp)

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
    mne_fiff
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
    scDisp
    scShared
    scMeas)

target_compile_definitions(${PROJECT_NAME} PRIVATE SCAN_EEGOSPORTS_PLUGIN)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD QT_STATICPLUGIN)
endif()
