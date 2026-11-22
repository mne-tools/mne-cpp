# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_lsl
  LANGUAGES CXX
)

set(SOURCES
  lsl_stream_info.cpp
  lsl_stream_inlet.cpp
  lsl_stream_outlet.cpp
  lsl_stream_discovery.cpp
  lsl_protocol.cpp
)

set(HEADERS
  lsl.h
  lsl_global.h
  lsl_stream_info.h
  lsl_stream_inlet.h
  lsl_stream_outlet.h
  lsl_stream_discovery.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} lsl_protocol.h)

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:include>)

# Sockets of mne_com; lsl_protocol.h (internal, not installed) includes its UdpSocket
target_link_libraries(${PROJECT_NAME} PRIVATE
  mne_base
  mne_com)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_LSL_LIBRARY)

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
        DESTINATION include/lsl/${_dir}
        COMPONENT sdk
    )
endforeach()
