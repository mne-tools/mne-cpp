# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_acq_recorder LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION test_acq_recorder.cpp)
else()
    add_executable(${PROJECT_NAME} test_acq_recorder.cpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test mne_acq_fiff Eigen3::Eigen)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    ACQ_RECORDER_CHECK_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/check_recording.py")
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE MACOSX_BUNDLE FALSE)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

mne_add_test(${PROJECT_NAME}
    LABELS unit
    TIMEOUT 600)
