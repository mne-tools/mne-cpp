# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(rtserver_fiffsimulator LANGUAGES CXX)

set(SOURCES
    fiffsimulator.cpp
)

set(HEADERS
    fiffsimulator.h
)

# Linked into mne_rt_server, whose connector manager creates the FIFF simulator
add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

set(MNE_LIBS_REQUIRED
    rtserver_connector
    mne_base
    mne_math
    mne_fiff
    mne_acq
    mne_acq_fiff
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
