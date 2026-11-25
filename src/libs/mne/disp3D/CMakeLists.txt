# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   johaenns <j.vorw01@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_disp3D LANGUAGES CXX)

# Handle Qt uic, moc, rcc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets ShaderTools)

# GuiPrivate is needed for QRhi headers — check availability (may be absent in some WASM builds)
find_package(Qt6 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 QUIET COMPONENTS GuiPrivate)
if(NOT Qt6GuiPrivate_FOUND)
  message(WARNING "Qt6::GuiPrivate not found — disp3D will not be built (required for QRhi)")
  return()
endif()
list(APPEND QT_REQUIRED_COMPONENTS GuiPrivate)

set(SOURCES
    core/dataloader.cpp
    core/viewstate.cpp
    geometry/meshfactory.cpp
    helpers/geometryinfo.cpp
    helpers/interpolation.cpp
    input/cameracontroller.cpp
    input/raypicker.cpp
    model/braintreemodel.cpp
    model/items/abstracttreeitem.cpp
    model/items/surfacetreeitem.cpp
    model/items/bemtreeitem.cpp
    model/items/sensortreeitem.cpp
    model/items/dipoletreeitem.cpp
    model/items/sourcespacetreeitem.cpp
    model/items/digitizertreeitem.cpp
    model/items/digitizersettreeitem.cpp
    model/items/networktreeitem.cpp
    renderable/brainsurface.cpp
    renderable/sourceestimateoverlay.cpp
    renderable/dipoleobject.cpp
    renderable/electrodeobject.cpp
    renderable/sliceobject.cpp
    renderable/networkobject.cpp
    renderable/polylineobject.cpp
    scene/multimodalscene.cpp
    scene/sensorfieldmapper.cpp
    scene/rtsensorstreammanager.cpp
    scene/sourceestimatemanager.cpp
    view/brainview.cpp
    view/brainviewcore.cpp
    view/brainwindow.cpp
    view/brainrenderer.cpp
    view/multiviewlayout.cpp
    workers/periodictask.cpp
    workers/stcloadingworker.cpp
    workers/rtsourcedataworker.cpp
    workers/rtsourcedatacontroller.cpp
    workers/rtsensordataworker.cpp
    workers/rtsensordatacontroller.cpp
    workers/rtsensorinterpolationmatworker.cpp
    workers/rtsourceinterpolationmatworker.cpp
)

set(HEADERS
    disp3D_global.h
    core/surfacekeys.h
    core/dataloader.h
    core/viewstate.h
    core/rendertypes.h
    geometry/meshfactory.h
    helpers/geometryinfo.h
    helpers/interpolation.h
    input/cameracontroller.h
    input/raypicker.h
    model/braintreemodel.h
    model/items/abstracttreeitem.h
    model/items/surfacetreeitem.h
    model/items/bemtreeitem.h
    model/items/sensortreeitem.h
    model/items/dipoletreeitem.h
    model/items/sourcespacetreeitem.h
    model/items/digitizertreeitem.h
    model/items/digitizersettreeitem.h
    model/items/networktreeitem.h
    renderable/brainsurface.h
    renderable/sourceestimateoverlay.h
    renderable/dipoleobject.h
    renderable/electrodeobject.h
    renderable/sliceobject.h
    renderable/networkobject.h
    renderable/polylineobject.h
    renderable/videooverlay.h
    scene/multimodalscene.h
    scene/pickresult.h
    scene/sensorfieldmapper.h
    scene/rtsensorstreammanager.h
    scene/sourceestimatemanager.h
    view/brainview.h
    view/brainviewcore.h
    view/brainwindow.h
    view/brainrenderer.h
    view/multiviewlayout.h
    workers/periodictask.h
    workers/stcloadingworker.h
    workers/rtsourcedataworker.h
    workers/rtsourcedatacontroller.h
    workers/rtsensordataworker.h
    workers/rtsensordatacontroller.h
    workers/rtsensorinterpolationmatworker.h
    workers/rtsourceinterpolationmatworker.h
)

# Build-info stamp

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Compile shaders into the library
qt_add_shaders(${PROJECT_NAME} "disp3d_shaders"
    PREFIX
        "/"
    BASE
        "shaders"
    FILES
        "shaders/standard.vert"
        "shaders/standard.frag"
        "shaders/holographic.vert"
        "shaders/holographic.frag"
        "shaders/anatomical.vert"
        "shaders/anatomical.frag"
        "shaders/dipole.vert"
        "shaders/dipole.frag"
        "shaders/shownormals.vert"
        "shaders/shownormals.frag"
        "shaders/electrode.vert"
        "shaders/electrode.frag"
        "shaders/slice.vert"
        "shaders/slice.frag"
        "shaders/video_overlay.vert"
        "shaders/video_overlay.frag"
        "shaders/screen_overlay.vert"
        "shaders/screen_overlay.frag"
        "shaders/video_decal.vert"
        "shaders/video_decal.frag"
        "shaders/video_decal_depth.vert"
        "shaders/video_decal_depth.frag"
)

target_include_directories(${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..>
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    PUBLIC $<INSTALL_INTERFACE:include>
    PUBLIC $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt6::")

set(MNE_LIBS_REQUIRED
    mne_ieeg
    mne_base
    mne_math
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
    mne_inv
    mne_mri
    mne_disp
    mne_connectivity
)

# The public headers include Qt
target_link_libraries(${PROJECT_NAME} PUBLIC ${QT_REQUIRED_COMPONENT_LIBS})
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen)

# Qt6::GuiPrivate is PRIVATE — QRhi details are fully encapsulated in .cpp files.
# Public headers use only forward declarations and lightweight rendertypes.h enums.
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::GuiPrivate)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC MNE_DISP3D
    PRIVATE
        MNE_DISP3D_LIBRARY)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)

    # Qt resource targets (e.g. from qt_add_shaders) must be exported for static builds
    foreach(_idx RANGE 1 10)
        if(TARGET ${PROJECT_NAME}_resources_${_idx})
            install(TARGETS ${PROJECT_NAME}_resources_${_idx} ${MNE_CPP_EXPORT_SET}
                ARCHIVE DESTINATION lib COMPONENT sdk)
        endif()
    endforeach()
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

# Install public headers preserving subdirectory structure
foreach(_header ${HEADERS})
    get_filename_component(_dir "${_header}" DIRECTORY)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/mne/disp3D/${_dir}
        COMPONENT sdk
    )
endforeach()
