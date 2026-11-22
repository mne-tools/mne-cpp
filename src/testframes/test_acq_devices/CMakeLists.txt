# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_acq_devices LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION test_acq_devices.cpp)
else()
    add_executable(${PROJECT_NAME} test_acq_devices.cpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test mne_acq_devices mne_com mne_base)

# A stand-in for the eego SDK library, loaded by the eego:// driver under test
add_library(eego_sdk_stub SHARED eego_sdk_stub.cpp)
set_target_properties(eego_sdk_stub PROPERTIES PREFIX "" OUTPUT_NAME "eego-SDK-stub")
add_dependencies(${PROJECT_NAME} eego_sdk_stub)
target_compile_definitions(${PROJECT_NAME} PRIVATE EEGO_SDK_STUB="$<TARGET_FILE:eego_sdk_stub>")

# Stand-ins for BrainFlow's BoardController library, a supported and an unsupported version
add_library(brainflow_stub SHARED brainflow_stub.cpp)
add_library(brainflow_stub_old SHARED brainflow_stub.cpp)
target_compile_definitions(brainflow_stub_old PRIVATE STUB_VERSION="4.9.2")
add_dependencies(${PROJECT_NAME} brainflow_stub brainflow_stub_old)
target_compile_definitions(${PROJECT_NAME} PRIVATE BRAINFLOW_STUB="$<TARGET_FILE:brainflow_stub>"
                                                   BRAINFLOW_STUB_OLD="$<TARGET_FILE:brainflow_stub_old>"
                                                   BRAINFLOW_STREAM_SCRIPT="${CMAKE_CURRENT_SOURCE_DIR}/brainflow_stream.py")

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE MACOSX_BUNDLE FALSE)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

mne_add_test(${PROJECT_NAME}
    LABELS unit
    TIMEOUT 120)
