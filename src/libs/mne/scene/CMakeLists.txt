# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_scene
  LANGUAGES CXX
)

set(SOURCES
  average_layout.cpp
  average_tile.cpp
  bar_chart.cpp
  butterfly_plot.cpp
  canvas.cpp
  colormap.cpp
  filter_response_plot.cpp
  image_plot.cpp
  line_chart.cpp
  raster_renderer.cpp
  sensor_layout.cpp
  spline_plot.cpp
  svg_renderer.cpp
  text_metrics.cpp
  ticks.cpp
  trace_view.cpp
  vector_plot.cpp
)

set(HEADERS
  scene_global.h
  scene.h
  average_layout.h
  average_tile.h
  bar_chart.h
  butterfly_plot.h
  canvas.h
  colormap.h
  display_list.h
  filter_response_plot.h
  image_plot.h
  input_event.h
  line_chart.h
  geometry.h
  raster_renderer.h
  sensor_layout.h
  spline_plot.h
  style.h
  svg_renderer.h
  text_metrics.h
  ticks.h
  trace_view.h
  vector_plot.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} detail/text_layout.h)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/MneWarnings.cmake)
mne_warn_on_narrowing(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

target_link_libraries(${PROJECT_NAME} PUBLIC
  mne_base)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_SCENE_LIBRARY)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

# Install public headers for SDK
foreach(_header ${HEADERS})
    get_filename_component(_dir "${_header}" DIRECTORY)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/mne/scene/${_dir}
        COMPONENT sdk
    )
endforeach()
