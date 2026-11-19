# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

# mne_acq_fiff: the FIFF and BIDS bridge of mne_acq (FiffInfo conversion, AcqRecorder).
cmake_minimum_required(VERSION 3.14)
project(mne_acq_fiff
  LANGUAGES CXX
)

set(SOURCES
  acq_fiff.cpp
  acq_fiff_replay.cpp
  acq_recorder.cpp
  detail/acq_direct_file.cpp
)

set(PRIVATE_HEADERS
  detail/acq_direct_file.h
)

set(HEADERS
  acq_fiff_global.h
  acq_fiff.h
  acq_fiff_replay.h
  acq_recorder.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${PRIVATE_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

# FiffInfo appears in acq_fiff.h, so its clients need mne_fiff too.
target_link_libraries(${PROJECT_NAME} PUBLIC mne_acq mne_fiff mne_base)
target_link_libraries(${PROJECT_NAME} PRIVATE
  mne_bids
  Eigen3::Eigen)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_ACQ_FIFF_LIBRARY)
# Physical values are raw * scale + offset exactly as decode() computes them: no FMA contraction.
if(MSVC)
    set_source_files_properties(acq_recorder.cpp PROPERTIES COMPILE_OPTIONS "/fp:precise;/fp:contract-")
else()
    set_source_files_properties(acq_recorder.cpp PROPERTIES COMPILE_OPTIONS "-ffp-contract=off")
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

foreach(_header ${HEADERS})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/acq/fiff
        COMPONENT sdk
    )
endforeach()
