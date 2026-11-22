# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2012-2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

# mne_com: TCP, UDP and serial transports to devices and servers. Standard C++ and the system's sockets only.
cmake_minimum_required(VERSION 3.21)
project(mne_com LANGUAGES CXX)

set(SOURCES
    serial_port.cpp
    tcp_server.cpp
    tcp_socket.cpp
    udp_socket.cpp
)

set(PRIVATE_HEADERS
    detail/socket_platform.h
)

set(HEADERS
    com_global.h
    serial_port.h
    tcp_server.h
    tcp_socket.h
    udp_socket.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${PRIVATE_HEADERS} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_COM_LIBRARY)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

# Install public headers for SDK
foreach(_header ${HEADERS})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/mne/com
        COMPONENT sdk
    )
endforeach()
