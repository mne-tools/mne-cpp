# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_viz
  LANGUAGES CXX
)

set(SOURCES
  brain/brainframecomposer.cpp
  brain/brainsurface.cpp
  brain/dataloader.cpp
  brain/dipoleobject.cpp
  brain/electrodeobject.cpp
  brain/geometryinfo.cpp
  brain/interpolation.cpp
  brain/meshfactory.cpp
  brain/networkobject.cpp
  brain/polylineobject.cpp
  brain/realtime/periodictask.cpp
  brain/realtime/rtsensordataworker.cpp
  brain/realtime/sensorstreamsession.cpp
  brain/realtime/rtsensorstream.cpp
  brain/realtime/rtsourcestream.cpp
  brain/realtime/rtsensorinterpolationmatworker.cpp
  brain/realtime/rtsourcedataworker.cpp
  brain/realtime/rtsourceinterpolationmatworker.cpp
  brain/realtime/stcloadingworker.cpp
  brain/sensorfieldmapper.cpp
  brain/scenestate.cpp
  brain/sourceestimatesession.cpp
  brain/sourceestimateoverlay.cpp
  brain/viewstate.cpp
  figures/average_layout.cpp
  figures/average_tile.cpp
  figures/bar_chart.cpp
  figures/butterfly_plot.cpp
  figures/canvas.cpp
  figures/colormap.cpp
  figures/filter_response_plot.cpp
  figures/image_plot.cpp
  figures/line_chart.cpp
  figures/raster_renderer.cpp
  figures/sensor_layout.cpp
  figures/spline_plot.cpp
  figures/svg_renderer.cpp
  figures/text_metrics.cpp
  figures/ticks.cpp
  figures/vector_plot.cpp
  forms/settings_form.cpp
  panels/compensator_panel.cpp
  panels/panel.cpp
  panels/projectors_panel.cpp
  gpu/brain_frame.cpp
  gpu/detail/software_shaders.cpp
  gpu/gpu_frame.cpp
  gpu/software_renderer.cpp
  gpu/trace_frame.cpp
  host/settings_store.cpp
  host/surface.cpp
  host/trace_view_client.cpp
  models/bids_tree.cpp
  models/channel_table.cpp
  models/evoked_conditions.cpp
  models/spectrum_table.cpp
  views/brain_view_interaction.cpp
  views/brain_view_overlay.cpp
  views/camera.cpp
  views/pane_layout.cpp
  views/ray_picking.cpp
  views/sweep_buffer.cpp
  views/trace_buffer.cpp
  views/trace_geometry.cpp
  views/trace_markers.cpp
  views/trace_view.cpp
  views/view_animator.cpp
)

set(HEADERS
  brain/brainframecomposer.h
  brain/brainsurface.h
  brain/dataloader.h
  brain/dipoleobject.h
  brain/electrodeobject.h
  brain/geometryinfo.h
  brain/instancing.h
  brain/interpolation.h
  brain/meshfactory.h
  brain/networkobject.h
  brain/pickresult.h
  brain/polylineobject.h
  brain/realtime/periodictask.h
  brain/realtime/rtsensordataworker.h
  brain/realtime/rtsensorinterpolationmatworker.h
  brain/realtime/rtsensorstream.h
  brain/realtime/rtsourcedataworker.h
  brain/realtime/rtsourceinterpolationmatworker.h
  brain/realtime/rtsourcestream.h
  brain/realtime/sensorstreamsession.h
  brain/realtime/stcloadingworker.h
  brain/rendertypes.h
  brain/sensorfieldmapper.h
  brain/scenestate.h
  brain/sourceestimateoverlay.h
  brain/sourceestimatesession.h
  brain/surfacekeys.h
  brain/viewstate.h
  figures/average_layout.h
  figures/average_tile.h
  figures/bar_chart.h
  figures/butterfly_plot.h
  figures/canvas.h
  figures/colormap.h
  figures/display_list.h
  figures/filter_response_plot.h
  figures/geometry.h
  figures/image_plot.h
  figures/line_chart.h
  figures/raster_renderer.h
  figures/sensor_layout.h
  figures/spline_plot.h
  figures/style.h
  figures/svg_renderer.h
  figures/text_metrics.h
  figures/ticks.h
  figures/vector_plot.h
  forms/settings_form.h
  panels/compensator_panel.h
  panels/panel.h
  panels/projectors_panel.h
  gpu/brain_frame.h
  gpu/gpu_frame.h
  gpu/software_renderer.h
  gpu/trace_frame.h
  host/dispatcher.h
  host/settings_store.h
  host/surface.h
  host/trace_view_client.h
  models/bids_tree.h
  models/channel_table.h
  models/evoked_conditions.h
  models/spectrum_table.h
  views/brain_view_interaction.h
  views/brain_view_overlay.h
  views/camera.h
  views/input_event.h
  views/pane_layout.h
  views/ray_picking.h
  views/sweep_buffer.h
  views/trace_buffer.h
  views/trace_geometry.h
  views/trace_markers.h
  views/trace_source.h
  views/trace_view.h
  views/view_animator.h
  viz.h
  viz_global.h
)

set(PRIVATE_HEADERS
  figures/detail/text_layout.h
  gpu/detail/software_shaders.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${PRIVATE_HEADERS})
include(${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/MneWarnings.cmake)
mne_warn_on_narrowing(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

# Eigen carries the vectors, matrices and quaternions of the 3-D camera and picking; the view models read
# the measurement info of mne_fiff, and the brain view's content reads FreeSurfer surfaces and BEMs
target_link_libraries(${PROJECT_NAME} PUBLIC
  mne_base
  mne_math
  mne_fiff
  mne_fs
  mne_mne
  mne_fwd
  mne_inv
  mne_connectivity
  mne_ieeg
  Eigen3::Eigen)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_VIZ_LIBRARY)

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
        DESTINATION include/mne/viz/${_dir}
        COMPONENT sdk
    )
endforeach()
