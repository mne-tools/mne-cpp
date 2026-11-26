# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_fwd_library LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)
qt_standard_project_setup()

qt_add_executable(test_fwd_library test_fwd_library.cpp)


# Register with CTest
enable_testing()
mne_add_test(${PROJECT_NAME}
    LABELS integration
    TIMEOUT 60
    REQUIRES_DATA
        mne-cpp-test-data/MEG/sample
        mne-cpp-test-data/subjects/sample)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(test_fwd_library PRIVATE STATICBUILD)
endif()

target_link_libraries(test_fwd_library PRIVATE
    mne_viz_qt
    mne_dsp
    mne_connectivity
    mne_inv
    mne_fwd
    mne_mne
    mne_fiff
    mne_fs
    mne_base
    mne_math
    Eigen3::Eigen
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
)
