# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

# mne_acq depends on the C++20 standard library only (no Qt, no Eigen), so this file also works as a
# top-level project for embedded and cross builds: cmake -S src/libs/mne/acq -B build-acq
cmake_minimum_required(VERSION 3.21)
project(mne_acq
  LANGUAGES CXX
)

if(PROJECT_IS_TOP_LEVEL)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS OFF)
endif()

set(SOURCES
  acq_global.cpp
  acq_error.cpp
  acq_thread.cpp
  acq_timebase.cpp
  acq_stream.cpp
  acq_reader.cpp
  acq_convert.cpp
  detail/acq_ring_memory.cpp
  detail/acq_shared_library.cpp
  acq_uri.cpp
  acq_backend.cpp
  acq_device.cpp
  acq_device_registry.cpp
  backends/acq_synthetic_backend.cpp
)

set(PRIVATE_HEADERS
  detail/acq_diagnostics.h
  detail/acq_stream_core.h
  detail/acq_ring_memory.h
  detail/acq_backend_impl.h
  detail/acq_control_queue.h
  detail/acq_uri_params.h
  detail/acq_synthetic_backend.h
  detail/acq_shared_library.h
  detail/acq_fiff_codes.h
)

set(HEADERS
  acq.h
  acq_global.h
  acq_error.h
  acq_memory.h
  acq_sample_format.h
  acq_clock_model.h
  acq_thread.h
  acq_timebase.h
  acq_stream_info.h
  acq_block_view.h
  acq_convert.h
  acq_reader.h
  acq_stream.h
  acq_backend.h
  acq_control.h
  acq_device.h
  acq_device_registry.h
  acq_eigen.h
  acq_dlpack.h
)

set(FILE_TO_UPDATE acq_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${PRIVATE_HEADERS})

find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
target_link_libraries(${PROJECT_NAME} PRIVATE ${CMAKE_DL_LIBS}) # runtime-loaded device libraries
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE avrt) # MMCSS for realtime threads
endif()

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
# Export only ACQSHARED_EXPORT symbols everywhere, as Windows DLLs do, so a missing export fails on every OS.
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_ACQ_LIBRARY)
# The converters define their arithmetic (multiply, then add): no FMA contraction on any target.
if(MSVC)
    set_source_files_properties(acq_convert.cpp PROPERTIES COMPILE_OPTIONS "/fp:precise;/fp:contract-")
else()
    set_source_files_properties(acq_convert.cpp PROPERTIES COMPILE_OPTIONS "-ffp-contract=off")
endif()
# Only the file that reports the commit sees its hash, so every other compile command stays the same across commits
if(DEFINED MNE_GIT_HASH_SHORT)
    set_property(SOURCE acq_global.cpp APPEND PROPERTY COMPILE_DEFINITIONS MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

foreach(_header ${HEADERS})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/mne/acq
        COMPONENT sdk
    )
endforeach()
