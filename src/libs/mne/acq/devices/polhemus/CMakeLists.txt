# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_acq_polhemus
  LANGUAGES CXX
)

# Head-device coregistration of digitized points; the digitizer itself is polhemus:// in mne_acq_devices.

set(SOURCES
  polhemus_coregistration.cpp
)

set(HEADERS
  acq_polhemus_global.h
  acquired_points.h
  polhemus_coregistration.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../..> $<INSTALL_INTERFACE:include>)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    mne_base
  PRIVATE
    Eigen3::Eigen)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_ACQ_POLHEMUS_LIBRARY)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

foreach(_header ${HEADERS})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/mne/acq/devices/polhemus
        COMPONENT sdk
    )
endforeach()
