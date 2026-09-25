# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_tool_preprocessing LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(SOURCES test_tool_preprocessing.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Test)

mne_add_test(${PROJECT_NAME}
    LABELS integration
    TIMEOUT 60
    REQUIRES_DATA
        mne-cpp-test-data/MEG/sample
        mne-cpp-test-data/subjects/sample)
