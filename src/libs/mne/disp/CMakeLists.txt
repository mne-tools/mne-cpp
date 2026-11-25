# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   johaenns <j.vorw01@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_disp LANGUAGES CXX)

# Handle qt moc and rcc automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt Gui only: the widgets are in mne_disp_widgets
set(QT_REQUIRED_COMPONENTS Core Gui)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

if(NOT (Qt6_FOUND OR QT_VERSION_MAJOR EQUAL 6))
    message(FATAL_ERROR "mne_disp now requires Qt 6 for the QRhi-based raw browser renderer.")
endif()

# QRhi currently relies on Qt private headers here by design; suppress the
# upstream reminder so real project warnings stay visible in configure output.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 REQUIRED COMPONENTS GuiPrivate ShaderTools)
set(MNE_DISP_HAS_RHIWIDGET TRUE)
message(STATUS "mne_disp: QRhi GPU rendering enabled as the only supported raw-browser renderer")

set(SOURCES
    plots/helpers/colormap.cpp
    plots/helpers/svgwriter.cpp
    plots/helpers/scenepainter.cpp
    detail/map_value.h
    viewers/helpers/channeldatamodel.cpp
    viewers/helpers/channelrhirenderer.cpp
    viewers/helpers/channelwindow.cpp
    viewers/helpers/rtfiffrawviewmodel.cpp
    viewers/helpers/evokedsetmodel.cpp
    viewers/helpers/channelinfomodel.cpp
    viewers/helpers/mneoperator.cpp
    viewers/helpers/frequencyspectrummodel.cpp
    viewers/helpers/bidsviewmodel.cpp
)

set(HEADERS
    disp_global.h
    plots/helpers/colormap.h
    plots/helpers/svgwriter.h
    plots/helpers/scenepainter.h
    viewers/helpers/channeldatamodel.h
    viewers/helpers/channelrhirenderer.h
    viewers/helpers/channelwindow.h
    viewers/helpers/rtfiffrawviewmodel.h
    viewers/helpers/evokedsetmodel.h
    viewers/helpers/selectionitem.h
    viewers/helpers/channelinfomodel.h
    viewers/helpers/mneoperator.h
    viewers/helpers/frequencyspectrummodel.h
    viewers/helpers/bidsviewmodel.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# ── QRhi / shader compilation ───────────────────────────────────────────────
if(MNE_DISP_HAS_RHIWIDGET)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::GuiPrivate)

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
endif()

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

target_include_directories(${PROJECT_NAME} PRIVATE .)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

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

# The public headers include Qt and the scene models the plots draw
target_link_libraries(${PROJECT_NAME} PUBLIC ${QT_REQUIRED_COMPONENT_LIBS} mne_scene)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_DISP_LIBRARY)

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
        DESTINATION include/mne/disp/${_dir}
        COMPONENT sdk
    )
endforeach()
