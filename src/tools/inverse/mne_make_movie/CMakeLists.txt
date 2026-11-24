# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_make_movie LANGUAGES CXX)

# Standard C++ only: frames are rasterised and written by mne::RasterImage
add_executable(${PROJECT_NAME} main.cpp)

set(MNE_LIBS_REQUIRED
    mne_fiff
    mne_mne
    mne_fs
    mne_inv
    mne_base
    mne_math
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)
