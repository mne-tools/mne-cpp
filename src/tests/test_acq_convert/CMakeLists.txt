# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_acq_convert LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION test_acq_convert.cpp)
else()
    add_executable(${PROJECT_NAME} test_acq_convert.cpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Test mne_acq)

# The scalar reference must use the same arithmetic as the converters.
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /fp:precise /fp:contract-)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -ffp-contract=off)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE FALSE MACOSX_BUNDLE FALSE)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

mne_add_test(${PROJECT_NAME}
    LABELS unit
    TIMEOUT 120)
