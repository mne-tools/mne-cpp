# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_polhemus_coregistration LANGUAGES CXX)

# Code-coverage instrumentation for MNE Align's digitizer, compiled into this test
if(WITH_CODE_COV)
  add_compile_options("--coverage" "-O1" "-g")
  add_link_options("--coverage")
endif()

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

# MNE Align's digitizer adapter feeds the coregistration from polhemus://
set(SOURCES
    test_polhemus_coregistration.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/mne_align/digitizer.cpp
    ${CMAKE_SOURCE_DIR}/src/apps/mne_align/digitizer.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
  mne_acq_polhemus
  mne_acq_devices
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${QT_REQUIRED_COMPONENT_LIBS}
  ${MNE_LIBS_REQUIRED}
  Eigen3::Eigen
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src/apps/mne_align)

set_target_properties(${PROJECT_NAME} PROPERTIES
  WIN32_EXECUTABLE FALSE
  MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# Register with CTest
enable_testing()
mne_add_test(${PROJECT_NAME}
    LABELS unit
    TIMEOUT 60)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
