# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_connectivity LANGUAGES CXX)

set(SOURCES
    metrics/abstractmetric.cpp
    metrics/correlation.cpp
    metrics/crosscorrelation.cpp
    metrics/coherency.cpp
    metrics/coherence.cpp
    metrics/imagcoherence.cpp
    metrics/unbiasedsquaredphaselagindex.cpp
    metrics/phaselockingvalue.cpp
    metrics/weightedphaselagindex.cpp
    metrics/debiasedsquaredweightedphaselagindex.cpp
    metrics/phaselagindex.cpp
    metrics/mvar_model.cpp
    metrics/granger_causality.cpp
    metrics/directed_transfer_function.cpp
    metrics/partial_directed_coherence.cpp
    network/network.cpp
    network/networkedge.cpp
    connectivitysettings.cpp
    compute_connectivity.cpp
)

set(HEADERS
    connectivity_global.h
    metrics/abstractmetric.h
    metrics/correlation.h
    metrics/crosscorrelation.h
    metrics/coherency.h
    metrics/coherence.h
    metrics/imagcoherence.h
    metrics/unbiasedsquaredphaselagindex.h
    metrics/phaselockingvalue.h
    metrics/weightedphaselagindex.h
    metrics/debiasedsquaredweightedphaselagindex.h
    metrics/phaselagindex.h
    metrics/mvar_model.h
    metrics/granger_causality.h
    metrics/directed_transfer_function.h
    metrics/partial_directed_coherence.h
    network/network.h
    network/networknode.h
    network/networkedge.h
    connectivitysettings.h
    connectivity.h
    compute_connectivity.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

set(MNE_LIBS_REQUIRED 
    mne_base
    mne_math
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_CONNECTIVITY_LIBRARY)

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
        DESTINATION include/mne/connectivity/${_dir}
        COMPONENT sdk
    )
endforeach()

