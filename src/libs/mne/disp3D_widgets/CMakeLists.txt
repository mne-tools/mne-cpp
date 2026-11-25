# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_disp3D_widgets LANGUAGES CXX)

# The QRhiWidget host of the 3-D brain view; the renderer itself is in mne_disp3D
set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets)
find_package(Qt6 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 QUIET COMPONENTS GuiPrivate)
if(NOT TARGET mne_disp3D OR NOT Qt6GuiPrivate_FOUND)
  message(WARNING "mne_disp3D is not available — disp3D_widgets will not be built")
  return()
endif()

set(SOURCES
    view/brainview.cpp
)

set(HEADERS
    disp3D_widgets_global.h
    view/brainview.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)
target_include_directories(${PROJECT_NAME} PRIVATE .)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt6::")

# The public header includes Qt Widgets and the renderer core
target_link_libraries(${PROJECT_NAME} PUBLIC ${QT_REQUIRED_COMPONENT_LIBS} mne_disp3D)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::GuiPrivate
    mne_base
    mne_fiff
    mne_fs
    mne_mne
    mne_inv
    Eigen3::Eigen
)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_DISP3D_WIDGETS_LIBRARY)

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
        DESTINATION include/mne/disp3D_widgets/${_dir}
        COMPONENT sdk
    )
endforeach()
