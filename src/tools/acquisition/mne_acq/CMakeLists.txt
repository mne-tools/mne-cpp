# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_acq_tool LANGUAGES CXX)

# The target is not called mne_acq: that name is the library.
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE mne_acq mne_acq_devices mne_com mne_acq_fiff mne_acq_lsl mne_lsl mne_base)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME mne_acq
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

# view (live traces) needs the display library and Qt Widgets, confined to acq_view.cpp; the other commands stay
# headless and Qt-free.
if(TARGET mne_viz_qt)
    find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
    target_sources(${PROJECT_NAME} PRIVATE acq_view.cpp acq_view.h)
    target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets mne_viz_qt mne_fiff Eigen3::Eigen)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_ACQ_VIEW)
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications
)
