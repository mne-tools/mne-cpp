# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_decoding LANGUAGES CXX)

set(SOURCES
  decoding_csp.cpp
  decoding_spoc.cpp
  decoding_ssd.cpp
  decoding_ica_label.cpp
)

set(HEADERS
  decoding_global.h
  decoding_csp.h
  decoding_spoc.h
  decoding_ssd.h
  decoding_ica_label.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/MneWarnings.cmake)
mne_warn_on_narrowing(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:include>)

set(MNE_LIBS_REQUIRED mne_base)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_DECODING_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

foreach(_header ${HEADERS})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${_header}
            DESTINATION include/decoding COMPONENT sdk)
endforeach()
