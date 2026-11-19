# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_base
  LANGUAGES CXX
)

find_package(Threads REQUIRED)

# zlib: the system's, or else the pinned release downloaded at configure time and built as a static library.
# Offline builds set FETCHCONTENT_SOURCE_DIR_MNECPP_ZLIB to an unpacked zlib source tree.
set(MNE_CPP_ZLIB_VERSION "1.4" CACHE STRING "zlib release built when the system has no zlib")
set(MNE_CPP_ZLIB_SHA256 "02dd159783677a749c04adbdd02b55b8a2bcc7e054f51b76b549199a916ad584"
    CACHE STRING "SHA-256 of the zlib release tarball")
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    set(ZLIB_TARGET ZLIB::ZLIB)
    message(STATUS "mne_base: Using system ZLIB ${ZLIB_VERSION_STRING}")
else()
    include(FetchContent)
    FetchContent_Declare(mnecpp_zlib
        URL "https://github.com/madler/zlib/releases/download/v${MNE_CPP_ZLIB_VERSION}/zlib-${MNE_CPP_ZLIB_VERSION}.tar.gz"
        URL_HASH SHA256=${MNE_CPP_ZLIB_SHA256}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        # Populate only: zlib's own CMake project would add its shared library, tests and install rules.
        SOURCE_SUBDIR do-not-configure)
    FetchContent_MakeAvailable(mnecpp_zlib)
    enable_language(C)
    set(_zlib_sources adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c infback.c
                      inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c)
    list(TRANSFORM _zlib_sources PREPEND "${mnecpp_zlib_SOURCE_DIR}/")
    add_library(mnecpp_zlib STATIC ${_zlib_sources})
    unset(_zlib_sources)
    # Third-party code: none of MNE-CPP's warning flags (or -Werror) apply.
    set_target_properties(mnecpp_zlib PROPERTIES
        COMPILE_OPTIONS "$<IF:$<C_COMPILER_ID:MSVC>,/W0,-w>"
        DISABLE_PRECOMPILE_HEADERS ON
        POSITION_INDEPENDENT_CODE ON)
    target_compile_definitions(mnecpp_zlib PRIVATE
        $<IF:$<C_COMPILER_ID:MSVC>,_CRT_SECURE_NO_DEPRECATE;_CRT_NONSTDC_NO_DEPRECATE,Z_HAVE_UNISTD_H>)
    target_include_directories(mnecpp_zlib PUBLIC "$<BUILD_INTERFACE:${mnecpp_zlib_SOURCE_DIR}>")
    # A static mne_base passes zlib on to its users, so the archive is part of the SDK.
    install(TARGETS mnecpp_zlib ${MNE_CPP_EXPORT_SET} ARCHIVE DESTINATION lib COMPONENT sdk)
    set(ZLIB_TARGET mnecpp_zlib)
    message(STATUS "mne_base: Using zlib ${MNE_CPP_ZLIB_VERSION} built from source (no system zlib)")
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
  log_qt.h
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

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Threads::Threads
  PRIVATE
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
