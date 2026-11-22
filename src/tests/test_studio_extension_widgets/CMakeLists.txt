# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_studio_extension_widgets LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Test Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

mne_add_test(${PROJECT_NAME}
    LABELS unit gui
    TIMEOUT 60
    REQUIRES_TARGETS mne_analyze_studio_analysis_results_extension_widgets mne_analyze_studio_core mne_analyze_studio_psd_analysis_extension_widgets)
if(MNE_TEST_SKIPPED)
    return()
endif()

set(SOURCES
    test_studio_extension_widgets.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
  add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(MNE_LIBS_REQUIRED
    mne_analyze_studio_analysis_results_extension_widgets
    mne_analyze_studio_psd_analysis_extension_widgets
    mne_analyze_studio_core
    mne_fiff
    mne_base
    mne_math
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_finalize_executable(${PROJECT_NAME})
endif()
