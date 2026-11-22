# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_acq_dlpack LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

# The library a NumPy consumer loads with ctypes; the test links it as well.
add_library(acq_dlpack_probe SHARED acq_dlpack_probe.cpp)
target_include_directories(acq_dlpack_probe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/dlpack/include)
target_link_libraries(acq_dlpack_probe PUBLIC mne_acq)
# Next to mne_acq, where the tests' rpath (../lib) finds it; Windows DLLs already land next to the test.
set_target_properties(acq_dlpack_probe PROPERTIES LIBRARY_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:mne_acq>")
# Python loads the probe, so the tests' executable-relative rpath does not apply; mne_acq sits next to it.
if(APPLE)
    set_target_properties(acq_dlpack_probe PROPERTIES INSTALL_RPATH "@loader_path")
elseif(UNIX)
    set_target_properties(acq_dlpack_probe PROPERTIES INSTALL_RPATH "\$ORIGIN")
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION test_acq_dlpack.cpp)
else()
    add_executable(${PROJECT_NAME} test_acq_dlpack.cpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test acq_dlpack_probe)
target_compile_definitions(${PROJECT_NAME} PRIVATE ACQ_DLPACK_PROBE_PATH="$<TARGET_FILE:acq_dlpack_probe>"
                                                   ACQ_LIBRARY_DIR="$<TARGET_FILE_DIR:mne_acq>")

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE MACOSX_BUNDLE FALSE)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

mne_add_test(${PROJECT_NAME}
    LABELS unit
    TIMEOUT 300)
