# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(ex_mne_api LANGUAGES CXX)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} main.cpp)
else()
    add_executable(${PROJECT_NAME} main.cpp)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  mne_mne
  mne_fiff
  Eigen3::Eigen
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    MNE_CTF_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../../testframes/test_mne_ctf_comp_python/data/ctf_grade3_raw.fif"
    MNE_SSS_FILE="${CMAKE_CURRENT_SOURCE_DIR}/../../testframes/test_fiff_raw_formats_python/data/raw_sss_raw.fif")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

if(BUILD_TESTS)
    mne_add_test(${PROJECT_NAME} LABELS example integration TIMEOUT 120
        REQUIRES_DATA mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif)
endif()
