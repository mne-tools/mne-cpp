# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_mne_anonymize_gui LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(MNE_ANONYMIZE_GUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../apps/mne_anonymize_gui)
set(MNE_ANONYMIZE_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/preprocessing/mne_anonymize)

set(SOURCES
    test_mne_anonymize_gui.cpp
    ${MNE_ANONYMIZE_GUI_DIR}/mainwindow.cpp
    ${MNE_ANONYMIZE_GUI_DIR}/settingscontrollergui.cpp
    ${MNE_ANONYMIZE_CORE_DIR}/fiffanonymizer.cpp
    ${MNE_ANONYMIZE_CORE_DIR}/settingscontrollercl.cpp
    ${MNE_ANONYMIZE_GUI_DIR}/mainwindow.h
    ${MNE_ANONYMIZE_GUI_DIR}/settingscontrollergui.h
    ${MNE_ANONYMIZE_GUI_DIR}/mainwindow.ui
    ${MNE_ANONYMIZE_CORE_DIR}/fiffanonymizer.h
    ${MNE_ANONYMIZE_CORE_DIR}/settingscontrollercl.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${MNE_ANONYMIZE_GUI_DIR} ${MNE_ANONYMIZE_CORE_DIR})

if(WITH_CODE_COV)
  target_compile_options(${PROJECT_NAME} PRIVATE --coverage -O1 -g)
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${QT_REQUIRED_COMPONENT_LIBS}
  mne_fiff
  mne_base
  Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  WIN32_EXECUTABLE FALSE
  MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

enable_testing()
mne_add_test(${PROJECT_NAME}
    LABELS integration gui
    TIMEOUT 120
    REQUIRES_DATA
        mne-cpp-test-data/MEG/sample)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
