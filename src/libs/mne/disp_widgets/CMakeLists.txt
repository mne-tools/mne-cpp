# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_disp_widgets LANGUAGES CXX)

# The Qt Widgets half of the 2-D display: plots, viewers and settings panels on top of mne_disp
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
# QRhiWidget hosts include the semi-public QRhi headers
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 REQUIRED COMPONENTS GuiPrivate)

set(SOURCES
    plots/imagesc.cpp
    plots/plot.cpp
    plots/graph.cpp
    plots/tfplot.cpp
    plots/bar.cpp
    plots/spline.cpp
    plots/lineplot.cpp
    plots/filterplot.cpp
    plots/helpers/figureexporter.cpp
    detail/layout_reset.h
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
    viewers/bidsview.cpp
    viewers/helpers/channelrhiview.cpp
    viewers/helpers/channellabelpanel.cpp
    viewers/helpers/overviewbarwidget.cpp
    viewers/helpers/timerulerwidget.cpp
    viewers/helpers/rtfiffrawviewdelegate.cpp
    viewers/helpers/sceneviewport.cpp
    viewers/helpers/averagelayoutwidget.cpp
    viewers/helpers/sensorlayoutwidget.cpp
    viewers/helpers/draggableframelesswidget.cpp
    viewers/helpers/frequencyspectrumdelegate.cpp
)

set(HEADERS
    disp_widgets_global.h
    plots/imagesc.h
    plots/plot.h
    plots/graph.h
    plots/tfplot.h
    plots/bar.h
    plots/spline.h
    plots/lineplot.h
    plots/filterplot.h
    plots/helpers/figureexporter.h
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
    viewers/bidsview.h
    viewers/helpers/channelrhiview.h
    viewers/helpers/channellabelpanel.h
    viewers/helpers/overviewbarwidget.h
    viewers/helpers/timerulerwidget.h
    viewers/helpers/rtfiffrawviewdelegate.h
    viewers/helpers/sceneviewport.h
    viewers/helpers/averagelayoutwidget.h
    viewers/helpers/sensorlayoutwidget.h
    viewers/helpers/draggableframelesswidget.h
    viewers/helpers/frequencyspectrumdelegate.h
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
    viewers/formfiles/filtersettingsview.ui
    viewers/formfiles/spharasettingsview.ui
    viewers/formfiles/fiffrawviewsettings.ui
    viewers/formfiles/triggerdetectionview.ui
    viewers/formfiles/quickcontrolview.ui
    viewers/formfiles/connectivitysettingsview.ui
    viewers/formfiles/minimumnormsettingsview.ui
    viewers/formfiles/averagingsettingsview.ui
    viewers/formfiles/projectsettingsview.ui
    viewers/formfiles/control3dview.ui
    viewers/formfiles/tfsettingsview.ui
    viewers/formfiles/hpisettingsview.ui
    viewers/formfiles/bidsview.ui
)

set(CMAKE_AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_SOURCE_DIR}/viewers/formfiles")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)
target_include_directories(${PROJECT_NAME} PRIVATE .)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt6::")

set(MNE_LIBS_REQUIRED
    mne_channels
    mne_base
    mne_math
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
    mne_inv
    mne_dsp
)

# The public headers include Qt Widgets, the display core and the scene models the plots draw
target_link_libraries(${PROJECT_NAME} PUBLIC ${QT_REQUIRED_COMPONENT_LIBS} mne_disp mne_scene)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::GuiPrivate
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_DISP_WIDGETS_LIBRARY)

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
        DESTINATION include/mne/disp_widgets/${_dir}
        COMPONENT sdk
    )
endforeach()
