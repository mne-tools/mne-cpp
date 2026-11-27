# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_viz_gpu_frame LANGUAGES CXX)

# Qt-free: the GPU frame description and its software backend are tested without Qt or a GPU
add_executable(${PROJECT_NAME} test_viz_gpu_frame.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
  mne_viz
  mne_base
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  WIN32_EXECUTABLE FALSE
  MACOSX_BUNDLE FALSE
)

enable_testing()
mne_add_test(${PROJECT_NAME}
    LABELS unit
    TIMEOUT 60)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
