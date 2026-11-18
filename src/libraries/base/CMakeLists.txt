# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_base
  LANGUAGES CXX
)

# Qt Core only: Log forwards to Qt's message handler by default.
set(QT_REQUIRED_COMPONENTS Core)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Threads REQUIRED)

# Try system ZLIB first, fall back to Qt's bundled zlib
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    set(ZLIB_TARGET ZLIB::ZLIB)
    message(STATUS "mne_base: Using system ZLIB")
else()
    find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS ZlibPrivate)
    if(TARGET Qt${QT_VERSION_MAJOR}::ZlibPrivate)
        set(ZLIB_TARGET Qt${QT_VERSION_MAJOR}::ZlibPrivate)
        message(STATUS "mne_base: Using Qt bundled ZLIB")
    else()
        message(FATAL_ERROR "Could not find ZLIB (neither system nor Qt bundled). "
            "Please install zlib development packages.")
    endif()
endif()

set(SOURCES
  base_global.cpp
  byte_stream.cpp
  cbor.cpp
  json.cpp
  json_stream_parser.cpp
  xml.cpp
  log.cpp
  process.cpp
  sha256.cpp
  string_utils.cpp
  thread_pool.cpp
)

set(HEADERS
  base_global.h
  buildinfo.h
  byte_stream.h
  endian.h
  error.h
  circular_buffer.h
  cbor.h
  json.h
  json_stream_parser.h
  xml.h
  log.h
  process.h
  sha256.h
  string_utils.h
  thread_pool.h
)

set(FILE_TO_UPDATE base_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:include>)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Threads::Threads
  PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${ZLIB_TARGET})

if(WIN32)
  # GetAdaptersAddresses() for hardware_addresses()
  target_link_libraries(${PROJECT_NAME} PRIVATE iphlpapi)
endif()

# The public headers use std::expected and std::format.
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_BASE_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

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
        DESTINATION include/base
        COMPONENT sdk
    )
endforeach()
