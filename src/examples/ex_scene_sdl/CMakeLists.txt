# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(ex_scene_sdl LANGUAGES CXX)

# A second host for the trace view: SDL3 instead of Qt
find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
  message(WARNING "ex_scene_sdl: SDL3 not found, the example is skipped (set SDL3_DIR or CMAKE_PREFIX_PATH)")
  return()
endif()

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
  mne_viz
  mne_base
  SDL3::SDL3
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

# Runs without a display on SDL's dummy video driver
if(BUILD_TESTS)
    mne_add_test(${PROJECT_NAME} COMMAND $<TARGET_FILE:${PROJECT_NAME}> --frames 5 ENVIRONMENT SDL_VIDEODRIVER=dummy
                 LABELS example unit TIMEOUT 120)
endif()
