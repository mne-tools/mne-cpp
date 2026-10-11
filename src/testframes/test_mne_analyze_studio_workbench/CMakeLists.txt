# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_mne_analyze_studio_workbench LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(WITH_CODE_COV)
    add_compile_options("--coverage" "-O1" "-g")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Test)

set(STUDIO_DIR ../../applications/mne_analyze_studio)
set(SOURCES
    test_mne_analyze_studio_workbench.cpp
    ${STUDIO_DIR}/neuro_kernel/neurokernelservice.cpp
    ${STUDIO_DIR}/neuro_kernel/neurokernelservice.h
    ${STUDIO_DIR}/skill_host/skillhostservice.cpp
    ${STUDIO_DIR}/skill_host/skillhostservice.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${STUDIO_DIR}/neuro_kernel
    ${STUDIO_DIR}/skill_host
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    MNE_STUDIO_EXTENSIONS_DIR="${CMAKE_SOURCE_DIR}/src/applications/mne_analyze_studio/extensions"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    mne_analyze_studio_app_core
    mne_analyze_studio_temporal_filter_skill
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Test
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
        mne-cpp-test-data/MEG/sample
    ENVIRONMENT QT_QPA_PLATFORM=offscreen)
