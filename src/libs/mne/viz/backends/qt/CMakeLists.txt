# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   johaenns <j.vorw01@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

# mne_viz_qt is the Qt backend of mne_viz: QRhi rendering of GPU frames, QWindow hosts, 2-D plots and viewers,
# the 3-D brain view (3d/) and Qt item models adapting the view models of mne_viz.

cmake_minimum_required(VERSION 3.14)
project(mne_viz_qt LANGUAGES CXX)

# Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

if(NOT (Qt6_FOUND OR QT_VERSION_MAJOR EQUAL 6))
    message(FATAL_ERROR "mne_viz_qt requires Qt 6 for QRhi rendering.")
endif()

# QRhi currently relies on Qt private headers here by design; suppress the
# upstream reminder so real project warnings stay visible in configure output.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 REQUIRED COMPONENTS GuiPrivate ShaderTools)

set(SOURCES
    plots/imagesc.cpp
    plots/plot.cpp
    plots/graph.cpp
    plots/tfplot.cpp
    plots/bar.cpp
    plots/spline.cpp
    plots/lineplot.cpp
    plots/filterplot.cpp
    plots/helpers/colormap.cpp
    plots/helpers/figureexporter.cpp
    plots/helpers/svgwriter.cpp
    plots/helpers/sceneinput.cpp
    plots/helpers/scenepainter.cpp
    plots/helpers/rhiframerenderer.cpp
    detail/map_value.h
    detail/layout_reset.h
    detail/scoped_flag.h
    viewers/abstractview.cpp
    viewers/applytoview.cpp
    viewers/coregsettingsview.cpp
    viewers/dipolefitview.cpp
    viewers/filterdesignview.cpp
    viewers/averagelayoutview.cpp
    viewers/fwdsettingsview.cpp
    viewers/helpers/scalecontrol.cpp
    viewers/progressview.cpp
    viewers/spectrumview.cpp
    viewers/modalityselectionview.cpp
    viewers/butterflyview.cpp
    viewers/channelselectionview.cpp
    viewers/spectrumsettingsview.cpp
    viewers/scalingview.cpp
    viewers/projectorsview.cpp
    viewers/compensatorview.cpp
    viewers/filtersettingsview.cpp
    viewers/spharasettingsview.cpp
    viewers/fiffrawviewsettings.cpp
    viewers/averageselectionview.cpp
    viewers/triggerdetectionview.cpp
    viewers/quickcontrolview.cpp
    viewers/connectivitysettingsview.cpp
    viewers/minimumnormsettingsview.cpp
    viewers/averagingsettingsview.cpp
    viewers/projectsettingsview.cpp
    viewers/control3dview.cpp
    viewers/tfsettingsview.cpp
    viewers/artifactsettingsview.cpp
    viewers/channeldataview.cpp
    viewers/rtfiffrawview.cpp
    viewers/multiviewwindow.cpp
    viewers/multiview.cpp
    viewers/hpisettingsview.cpp
    viewers/covariancesettingsview.cpp
    viewers/settingspanel.cpp
    viewers/bidsview.cpp
    viewers/helpers/channeldatamodel.cpp
    viewers/helpers/channelrhiview.cpp
    viewers/helpers/channelrhirenderer.cpp
    viewers/helpers/channelwindow.cpp
    viewers/helpers/channellabelpanel.cpp
    viewers/helpers/overviewbarwidget.cpp
    viewers/helpers/timerulerwidget.cpp
    viewers/helpers/rtfiffrawviewmodel.cpp
    viewers/helpers/rtfiffrawviewdelegate.cpp
    viewers/helpers/evokedsetmodel.cpp
    viewers/helpers/sceneviewport.cpp
    viewers/helpers/averagelayoutwidget.cpp
    viewers/helpers/sensorlayoutwidget.cpp
    viewers/helpers/channelinfomodel.cpp
    viewers/helpers/mneoperator.cpp
    viewers/helpers/draggableframelesswidget.cpp
    viewers/helpers/frequencyspectrumdelegate.cpp
    viewers/helpers/frequencyspectrummodel.cpp
    viewers/helpers/bidsviewmodel.cpp
    3d/core/dataloader.cpp
    3d/core/viewstate.cpp
    3d/geometry/meshfactory.cpp
    3d/helpers/geometryinfo.cpp
    3d/helpers/interpolation.cpp
    3d/input/cameracontroller.cpp
    3d/input/raypicker.cpp
    3d/model/braintreemodel.cpp
    3d/model/items/abstracttreeitem.cpp
    3d/model/items/surfacetreeitem.cpp
    3d/model/items/bemtreeitem.cpp
    3d/model/items/sensortreeitem.cpp
    3d/model/items/dipoletreeitem.cpp
    3d/model/items/sourcespacetreeitem.cpp
    3d/model/items/digitizertreeitem.cpp
    3d/model/items/digitizersettreeitem.cpp
    3d/model/items/networktreeitem.cpp
    3d/renderable/brainsurface.cpp
    3d/renderable/sourceestimateoverlay.cpp
    3d/renderable/dipoleobject.cpp
    3d/renderable/electrodeobject.cpp
    3d/renderable/sliceobject.cpp
    3d/renderable/networkobject.cpp
    3d/renderable/polylineobject.cpp
    3d/scene/multimodalscene.cpp
    3d/scene/sensorfieldmapper.cpp
    3d/scene/rtsensorstreammanager.cpp
    3d/scene/sourceestimatemanager.cpp
    3d/view/brainview.cpp
    3d/view/brainviewcore.cpp
    3d/view/brainwindow.cpp
    3d/view/brainrenderer.cpp
    3d/view/multiviewlayout.cpp
    3d/workers/periodictask.cpp
    3d/workers/stcloadingworker.cpp
    3d/workers/rtsourcedataworker.cpp
    3d/workers/rtsourcedatacontroller.cpp
    3d/workers/rtsensordataworker.cpp
    3d/workers/rtsensordatacontroller.cpp
    3d/workers/rtsensorinterpolationmatworker.cpp
    3d/workers/rtsourceinterpolationmatworker.cpp
)

set(HEADERS
    viz_qt_global.h
    plots/imagesc.h
    plots/plot.h
    plots/graph.h
    plots/tfplot.h
    plots/bar.h
    plots/spline.h
    plots/lineplot.h
    plots/filterplot.h
    plots/helpers/colormap.h
    plots/helpers/figureexporter.h
    plots/helpers/svgwriter.h
    plots/helpers/sceneinput.h
    plots/helpers/scenepainter.h
    plots/helpers/rhiframerenderer.h
    viewers/abstractview.h
    viewers/applytoview.h
    viewers/coregsettingsview.h
    viewers/dipolefitview.h
    viewers/filterdesignview.h
    viewers/averagelayoutview.h
    viewers/fwdsettingsview.h
    viewers/helpers/scalecontrol.h
    viewers/progressview.h
    viewers/spectrumview.h
    viewers/modalityselectionview.h
    viewers/butterflyview.h
    viewers/channelselectionview.h
    viewers/spectrumsettingsview.h
    viewers/scalingview.h
    viewers/projectorsview.h
    viewers/compensatorview.h
    viewers/filtersettingsview.h
    viewers/spharasettingsview.h
    viewers/fiffrawviewsettings.h
    viewers/averageselectionview.h
    viewers/triggerdetectionview.h
    viewers/quickcontrolview.h
    viewers/connectivitysettingsview.h
    viewers/minimumnormsettingsview.h
    viewers/averagingsettingsview.h
    viewers/projectsettingsview.h
    viewers/control3dview.h
    viewers/tfsettingsview.h
    viewers/artifactsettingsview.h
    viewers/channeldataview.h
    viewers/rtfiffrawview.h
    viewers/multiviewwindow.h
    viewers/multiview.h
    viewers/hpisettingsview.h
    viewers/covariancesettingsview.h
    viewers/settingspanel.h
    viewers/bidsview.h
    viewers/helpers/channeldatamodel.h
    viewers/helpers/channelrhiview.h
    viewers/helpers/channelrhirenderer.h
    viewers/helpers/channelwindow.h
    viewers/helpers/channellabelpanel.h
    viewers/helpers/overviewbarwidget.h
    viewers/helpers/timerulerwidget.h
    viewers/helpers/rtfiffrawviewdelegate.h
    viewers/helpers/rtfiffrawviewmodel.h
    viewers/helpers/evokedsetmodel.h
    viewers/helpers/sceneviewport.h
    viewers/helpers/averagelayoutwidget.h
    viewers/helpers/sensorlayoutwidget.h
    viewers/helpers/selectionitem.h
    viewers/helpers/channelinfomodel.h
    viewers/helpers/mneoperator.h
    viewers/helpers/draggableframelesswidget.h
    viewers/helpers/frequencyspectrumdelegate.h
    viewers/helpers/frequencyspectrummodel.h
    viewers/helpers/bidsviewmodel.h
    3d/core/surfacekeys.h
    3d/core/dataloader.h
    3d/core/viewstate.h
    3d/core/rendertypes.h
    3d/geometry/meshfactory.h
    3d/helpers/geometryinfo.h
    3d/helpers/interpolation.h
    3d/input/cameracontroller.h
    3d/input/raypicker.h
    3d/model/braintreemodel.h
    3d/model/items/abstracttreeitem.h
    3d/model/items/surfacetreeitem.h
    3d/model/items/bemtreeitem.h
    3d/model/items/sensortreeitem.h
    3d/model/items/dipoletreeitem.h
    3d/model/items/sourcespacetreeitem.h
    3d/model/items/digitizertreeitem.h
    3d/model/items/digitizersettreeitem.h
    3d/model/items/networktreeitem.h
    3d/renderable/brainsurface.h
    3d/renderable/sourceestimateoverlay.h
    3d/renderable/dipoleobject.h
    3d/renderable/electrodeobject.h
    3d/renderable/sliceobject.h
    3d/renderable/networkobject.h
    3d/renderable/polylineobject.h
    3d/renderable/videooverlay.h
    3d/scene/multimodalscene.h
    3d/scene/pickresult.h
    3d/scene/sensorfieldmapper.h
    3d/scene/rtsensorstreammanager.h
    3d/scene/sourceestimatemanager.h
    3d/view/brainview.h
    3d/view/brainviewcore.h
    3d/view/brainwindow.h
    3d/view/brainrenderer.h
    3d/view/multiviewlayout.h
    3d/workers/periodictask.h
    3d/workers/stcloadingworker.h
    3d/workers/rtsourcedataworker.h
    3d/workers/rtsourcedatacontroller.h
    3d/workers/rtsensordataworker.h
    3d/workers/rtsensordatacontroller.h
    3d/workers/rtsensorinterpolationmatworker.h
    3d/workers/rtsourceinterpolationmatworker.h
    3d/core/qteigen_p.h
)

set(UI
    viewers/formfiles/applytoview.ui
    viewers/formfiles/coregsettingsview.ui
    viewers/formfiles/dipolefitview.ui
    viewers/formfiles/filterdesignview.ui
    viewers/formfiles/fwdsettingsview.ui
    viewers/formfiles/scalecontrol.ui
    viewers/formfiles/progressview.ui
    viewers/formfiles/channelselectionview.ui
    viewers/formfiles/scalingview.ui
    viewers/formfiles/spharasettingsview.ui
    viewers/formfiles/fiffrawviewsettings.ui
    viewers/formfiles/triggerdetectionview.ui
    viewers/formfiles/quickcontrolview.ui
    viewers/formfiles/connectivitysettingsview.ui
    viewers/formfiles/minimumnormsettingsview.ui
    viewers/formfiles/projectsettingsview.ui
    viewers/formfiles/control3dview.ui
    viewers/formfiles/hpisettingsview.ui
    viewers/formfiles/bidsview.ui
)

set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/viewers/formfiles")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI})

# ── QRhi / shader compilation ───────────────────────────────────────────────
qt_add_shaders(${PROJECT_NAME} "channeldata_shaders"
    BATCHABLE
    PRECOMPILE
    PREFIX "/disp/shaders"
    FILES
        viewers/helpers/shaders/channeldata.vert
        viewers/helpers/shaders/channeldata.frag
        viewers/helpers/shaders/overlay.vert
        viewers/helpers/shaders/overlay.frag
)

qt_add_shaders(${PROJECT_NAME} "brain_shaders"
    PREFIX
        "/"
    BASE
        "3d/shaders"
    FILES
        "3d/shaders/standard.vert"
        "3d/shaders/standard.frag"
        "3d/shaders/holographic.vert"
        "3d/shaders/holographic.frag"
        "3d/shaders/anatomical.vert"
        "3d/shaders/anatomical.frag"
        "3d/shaders/dipole.vert"
        "3d/shaders/dipole.frag"
        "3d/shaders/shownormals.vert"
        "3d/shaders/shownormals.frag"
        "3d/shaders/electrode.vert"
        "3d/shaders/electrode.frag"
        "3d/shaders/slice.vert"
        "3d/shaders/slice.frag"
        "3d/shaders/video_overlay.vert"
        "3d/shaders/video_overlay.frag"
        "3d/shaders/screen_overlay.vert"
        "3d/shaders/screen_overlay.frag"
        "3d/shaders/video_decal.vert"
        "3d/shaders/video_decal.frag"
        "3d/shaders/video_decal_depth.vert"
        "3d/shaders/video_decal_depth.frag"
)

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../..> $<INSTALL_INTERFACE:include>)

target_include_directories(${PROJECT_NAME} PRIVATE .)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_channels
    mne_ieeg
    mne_base
    mne_math
    mne_fiff
    mne_fs
    mne_mne
    mne_mri
    mne_fwd
    mne_inv
    mne_connectivity
    mne_dsp
)

# The public headers include Qt and mne_viz, whose figures, views and view models this library shows
target_link_libraries(${PROJECT_NAME} PUBLIC ${QT_REQUIRED_COMPONENT_LIBS} mne_viz)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Qt6::GuiPrivate
    Eigen3::Eigen
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC MNE_VIZ_QT_3D
    PRIVATE MNE_VIZ_QT_LIBRARY)

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

# Install public headers for SDK
foreach(_header ${HEADERS})
    get_filename_component(_dir "${_header}" DIRECTORY)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/mne/viz/backends/qt/${_dir}
        COMPONENT sdk
    )
endforeach()
