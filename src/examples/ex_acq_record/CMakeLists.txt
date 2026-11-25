# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(ex_acq_record LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} main.cpp)
else()
    add_executable(${PROJECT_NAME} main.cpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
  mne_acq
  mne_acq_fiff
  mne_fiff
  mne_disp
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Widgets
  Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE FALSE WIN32_EXECUTABLE FALSE)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

if(BUILD_TESTS)
    mne_add_test(${PROJECT_NAME} LABELS example integration TIMEOUT 120 ENVIRONMENT QT_QPA_PLATFORM=offscreen)
endif()
