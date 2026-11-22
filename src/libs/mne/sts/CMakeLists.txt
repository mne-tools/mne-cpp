# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_sts LANGUAGES CXX)

set(SOURCES
    sts_ttest.cpp
    sts_ftest.cpp
    sts_correction.cpp
    sts_adjacency.cpp
    sts_cluster.cpp
    sts_source_metrics.cpp
    sts_cov_estimators.cpp
)

set(HEADERS
    sts_global.h
    sts_types.h
    sts_ttest.h
    sts_ftest.h
    sts_correction.h
    sts_adjacency.h
    sts_cluster.h
    sts_source_metrics.h
    sts_cov_estimators.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
include(${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/MneWarnings.cmake)
mne_warn_on_narrowing(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

set(MNE_LIBS_REQUIRED 
    mne_base
    mne_math
    mne_fiff
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_STS_LIBRARY)

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
        DESTINATION include/mne/sts/${_dir}
        COMPONENT sdk
    )
endforeach()
