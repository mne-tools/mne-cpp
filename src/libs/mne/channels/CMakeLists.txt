# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_channels
  LANGUAGES CXX
)

set(SOURCES
  layoutloader.cpp
  layoutmaker.cpp
  selectionio.cpp
  standard_montage.cpp
)

set(HEADERS
  channels_global.h
  layoutloader.h
  layoutmaker.h
  selectionio.h
  standard_montage.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# MNE-Python's standard_1005 electrode table, behind StandardMontage, embedded as a string literal
set(_elc "${CMAKE_CURRENT_SOURCE_DIR}/data/standard_1005.elc")
file(READ "${_elc}" STANDARD_1005_ELC)
file(CONFIGURE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/standard_1005_elc.inc" CONTENT "R\"elc(@STANDARD_1005_ELC@)elc\"\n" @ONLY)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_elc}")

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    mne_base
    Eigen3::Eigen
  PRIVATE
    mne_math)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_CHANNELS_LIBRARY)

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
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/mne/channels
        COMPONENT sdk
    )
endforeach()
