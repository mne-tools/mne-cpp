# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

# mne_viz_sdl is the SDL3 backend of mne_viz: SDL windows and input, frames drawn by the software renderer.

cmake_minimum_required(VERSION 3.14)
project(mne_viz_sdl LANGUAGES CXX)

find_package(SDL3 CONFIG QUIET)
if(NOT SDL3_FOUND)
  message(WARNING "mne_viz_sdl: SDL3 not found, the SDL backend is skipped (set SDL3_DIR or CMAKE_PREFIX_PATH)")
  return()
endif()

set(SOURCES
  sdl_host.cpp
)

set(HEADERS
  viz_sdl_global.h
  sdl_host.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
include(${CMAKE_CURRENT_LIST_DIR}/../../../../../../cmake/MneWarnings.cmake)
mne_warn_on_narrowing(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../..> $<INSTALL_INTERFACE:include>)

# The public headers include mne_viz; SDL stays private behind forward declarations
target_link_libraries(${PROJECT_NAME} PUBLIC mne_viz PRIVATE SDL3::SDL3)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_VIZ_SDL_LIBRARY)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

# Not part of the exported SDK targets yet: MNE-CPPConfig.cmake does not look for SDL3
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

# Install public headers for SDK
foreach(_header ${HEADERS})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/mne/viz/backends/sdl
        COMPONENT sdk
    )
endforeach()
