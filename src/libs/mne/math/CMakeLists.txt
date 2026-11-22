# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_math
  LANGUAGES CXX
)

set(SOURCES
  linalg.cpp
  numerics.cpp
  sphere.cpp
  warp.cpp
  kmeans.cpp
  spectral.cpp
)

set(HEADERS
  math_global.h
  linalg.h
  numerics.h
  sphere.h
  warp.h
  simplex_algorithm.h
  matrix_io.h
  kmeans.h
  spectral.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
include(${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/MneWarnings.cmake)
mne_warn_on_narrowing(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

target_link_libraries(${PROJECT_NAME} PUBLIC
  mne_base
  PRIVATE
  Eigen3::Eigen)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_MATH_LIBRARY)

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
        DESTINATION include/mne/math/${_dir}
        COMPONENT sdk
    )
endforeach()
