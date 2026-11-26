# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_scene_trace_view LANGUAGES CXX)

# Qt-free: the trace view of mne_viz is tested without Qt, as it is built
add_executable(${PROJECT_NAME} test_scene_trace_view.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
  mne_viz
  mne_base
)

# Golden SVG files; MNE_UPDATE_GOLDEN=1 rewrites them from the current output
target_compile_definitions(${PROJECT_NAME} PRIVATE GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

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
