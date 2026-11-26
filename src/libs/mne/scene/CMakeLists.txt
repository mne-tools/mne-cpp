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
  brain_frame.cpp
  brain_view_interaction.cpp
  brain_view_overlay.cpp
  butterfly_plot.cpp
  camera.cpp
  canvas.cpp
  colormap.cpp
  evoked_conditions.cpp
  filter_response_plot.cpp
  gpu_frame.cpp
  image_plot.cpp
  line_chart.cpp
  pane_layout.cpp
  raster_renderer.cpp
  ray_picking.cpp
  sensor_layout.cpp
  software_renderer.cpp
  spline_plot.cpp
  svg_renderer.cpp
  sweep_buffer.cpp
  text_metrics.cpp
  ticks.cpp
  trace_buffer.cpp
  trace_frame.cpp
  trace_geometry.cpp
  trace_markers.cpp
  trace_view.cpp
  vector_plot.cpp
  view_animator.cpp
  detail/software_shaders.cpp
)

set(HEADERS
  scene_global.h
  scene.h
  average_layout.h
  average_tile.h
  bar_chart.h
  brain_frame.h
  brain_view_interaction.h
  brain_view_overlay.h
  butterfly_plot.h
  camera.h
  canvas.h
  colormap.h
  display_list.h
  evoked_conditions.h
  filter_response_plot.h
  gpu_frame.h
  image_plot.h
  input_event.h
  line_chart.h
  geometry.h
  pane_layout.h
  raster_renderer.h
  ray_picking.h
  sensor_layout.h
  software_renderer.h
  spline_plot.h
  style.h
  svg_renderer.h
  sweep_buffer.h
  text_metrics.h
  ticks.h
  trace_buffer.h
  trace_frame.h
  trace_geometry.h
  trace_markers.h
  trace_source.h
  trace_view.h
  vector_plot.h
  view_animator.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} detail/software_shaders.h detail/text_layout.h)
include(${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/MneWarnings.cmake)
mne_warn_on_narrowing(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

# Eigen carries the vectors, matrices and quaternions of the 3-D camera and picking
target_link_libraries(${PROJECT_NAME} PUBLIC
  mne_base
  Eigen3::Eigen)

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
