# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_mne_inspect_app LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

enable_testing()
mne_add_test(${PROJECT_NAME}
    LABELS integration gui
    TIMEOUT 120
    REQUIRES_DATA
        mne-cpp-test-data/MEG/sample
        mne-cpp-test-data/subjects/sample
    REQUIRES_TARGETS mne_disp3D mne_inspect_app_core
    ENVIRONMENT QT_QPA_PLATFORM=offscreen)
if(MNE_TEST_SKIPPED)
    return()
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION test_mne_inspect_app.cpp)
else()
    add_executable(${PROJECT_NAME} test_mne_inspect_app.cpp)
endif()

list(TRANSFORM QT_REQUIRED_COMPONENTS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/applications/mne_inspect/app
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENTS}
    mne_inspect_app_core
    inspect_electrodes
    inspect_mri_slices
    mne_disp3D
    mne_mna
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
