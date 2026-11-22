# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(scan_babymeg LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    babymeg.cpp
    babymeg_global.cpp
    FormFiles/babymegsetupwidget.cpp
)

set(HEADERS
    babymeg.h
    babymeg_global.h
    FormFiles/babymegsetupwidget.h
)

set(FILE_TO_UPDATE babymeg_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_acq_devices
    mne_acq_fiff
    mne_base
    mne_fiff
    Eigen3::Eigen
    scShared
    scMeas)

target_compile_definitions(${PROJECT_NAME} PRIVATE SCAN_BABYMEG_PLUGIN)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD QT_STATICPLUGIN)
endif()
