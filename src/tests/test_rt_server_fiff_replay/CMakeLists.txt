# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_rt_server_fiff_replay LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION test_rt_server_fiff_replay.cpp)
else()
    add_executable(${PROJECT_NAME} test_rt_server_fiff_replay.cpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    mne_acq
    mne_acq_devices
    mne_acq_fiff
    mne_com
    mne_fiff
    Eigen3::Eigen)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    MNE_CTF_COMP_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../test_mne_ctf_comp_python/data")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE MACOSX_BUNDLE FALSE)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

add_dependencies(${PROJECT_NAME} mne_rt_server)

# The server listens on the fixed ports 4217 and 4218.
mne_add_test(${PROJECT_NAME}
    LABELS integration
    TIMEOUT 120)
set_tests_properties(${PROJECT_NAME} PROPERTIES RUN_SERIAL TRUE)
