# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(ex_inverse_cmne LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

# qt_add_executable links the Emscripten runtime (embind) that Qt's WebAssembly build needs
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} main.cpp)
else()
    add_executable(${PROJECT_NAME} main.cpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
  mne_disp
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Widgets
  mne_inv
  mne_mne
  mne_fs
  mne_fiff
  Eigen3::Eigen
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

if(BUILD_TESTS)
    mne_add_test(${PROJECT_NAME} LABELS example integration TIMEOUT 300 ENVIRONMENT QT_QPA_PLATFORM=offscreen
        REQUIRES_DATA mne-cpp-test-data/Result/ref-sample_audvis-meg-eeg-oct-6-fwd.fif)
endif()
