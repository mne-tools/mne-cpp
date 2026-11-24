# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_rt_server LANGUAGES CXX)

set(SOURCES
    main.cpp
    connectormanager.cpp
    mne_rt_server.cpp
    fiffstreamserver.cpp
    fiffstreamthread.cpp
    commandserver.cpp
    commandthread.cpp
)

set(HEADERS
    connectormanager.h
    mne_rt_server.h
    fiffstreamserver.h
    fiffstreamthread.h
    commandserver.h
    commandthread.h
    mne_rt_commands.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set(MNE_LIBS_REQUIRED
    rtserver_connector
    rtserver_fiffsimulator
    mne_fiff
    mne_base
    mne_com
    mne_math
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
