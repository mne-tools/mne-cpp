# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.19)
project(test_mne_testing_helper LANGUAGES NONE)

mne_add_test(${PROJECT_NAME}
    LABELS unit
    TIMEOUT 600
    COMMAND "${CMAKE_COMMAND}"
            "-DMNE_TESTING_MODULE=${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake/MneTesting.cmake"
            "-DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/fixtures"
            "-DGENERATOR=${CMAKE_GENERATOR}"
            "-DGENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
            "-DGENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}"
            "-DCXX_COMPILER=${CMAKE_CXX_COMPILER}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/run_self_test.cmake"
)
