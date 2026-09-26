# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Juan GPC <jgarciaprieto@mgh.harvard.edu>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_mne_forward_solution LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets Concurrent Network Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    test_mne_forward_solution.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

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
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${QT_REQUIRED_COMPONENT_LIBS}
  ${MNE_LIBS_REQUIRED}
  Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  WIN32_EXECUTABLE FALSE
  MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()


# Register with CTest
enable_testing()
# See test_fwd_bem_field_computation: this one takes over five minutes in a
# coverage build. Note the ctest timeout is not the only limit, QTest aborts a
# single test function after five minutes unless QTEST_FUNCTION_TIMEOUT is raised
# (the CI test workflow does).
mne_add_test(${PROJECT_NAME}
    LABELS integration slow
    TIMEOUT 1800
    REQUIRES_DATA
        mne-cpp-test-data/MEG/sample
        mne-cpp-test-data/Result
        mne-cpp-test-data/subjects/sample)
set_tests_properties(${PROJECT_NAME} PROPERTIES RUN_SERIAL TRUE)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
