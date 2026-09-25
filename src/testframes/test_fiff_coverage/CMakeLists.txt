# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.21)
project(test_fiff_coverage LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(SOURCES test_fiff_coverage.cpp)

qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Test
    mne_fiff
    mne_mne
    mne_utils
    mne_math
    Eigen3::Eigen
)

if(WITH_CODE_COV)
    target_compile_options(${PROJECT_NAME} PRIVATE --coverage -O1 -g)
    target_link_options(${PROJECT_NAME} PRIVATE --coverage)
endif()

qt_finalize_target(${PROJECT_NAME})

# Register with CTest
enable_testing()
mne_add_test(${PROJECT_NAME}
    LABELS integration
    TIMEOUT 60
    REQUIRES_DATA
        mne-cpp-test-data/MEG/sample
        mne-cpp-test-data/Result)
