# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_fiff LANGUAGES CXX)

set(SOURCES
    fiff_tag.cpp
    fiff_coord_trans.cpp
    fiff_ch_info.cpp
    fiff_ch_name.cpp
    fiff_proj.cpp
    fiff_named_matrix.cpp
    fiff_raw_data.cpp
    fiff_ctf_comp.cpp
    fiff_id.cpp
    fiff_info.cpp
    fiff_dig_point.cpp
    fiff_ch_pos.cpp
    fiff_cov.cpp
    fiff_stream.cpp
    fiff_info_base.cpp
    fiff_evoked.cpp
    fiff_evoked_file.h
    fiff_annotations_json.h
    fiff_epochs.cpp
    fiff_evoked_set.cpp
    fiff_events.cpp
    fiff_io.cpp
    fiff_dig_montage.cpp
    fiff_dig_point_set.cpp
    fiff_dir_node.cpp
    fiff_explain.cpp
    fiff_sparse_matrix.cpp
    fiff_digitizer_data.cpp
    fiff_coord_trans_set.cpp
    fiff_file_sharer.cpp
    fiff_annotations.cpp
    fiff_annotation_event_utils.cpp
)

set(HEADERS
    fiff.h
    fiff_global.h
    fiff_explain.h
    fiff_file.h
    fiff_types.h
    fiff_time.h
    fiff_data_ref.h
    fiff_id.h
    fiff_constants.h
    fiff_tag.h
    fiff_coord_trans.h
    fiff_ch_info.h
    fiff_ch_name.h
    fiff_proj.h
    fiff_named_matrix.h
    fiff_ctf_comp.h
    fiff_info.h
    fiff_raw_data.h
    fiff_dir_entry.h
    fiff_raw_dir.h
    fiff_dig_point.h
    fiff_ch_pos.h
    fiff_cov.h
    fiff_stream.h
    fiff_info_base.h
    fiff_evoked.h
    fiff_epochs.h
    fiff_evoked_set.h
    fiff_events.h
    fiff_io.h
    fiff_dig_montage.h
    fiff_dig_point_set.h
    fiff_dir_node.h
    fiff_sparse_matrix.h
    fiff_digitizer_data.h
    fiff_coord_trans_set.h
    fiff_file_sharer.h
    fiff_annotations.h
    fiff_annotation_event_utils.h
    fiff_byte_swap.h
)

# MNE-C's tag, block, unit and channel-kind names, behind FiffExplain, compiled in as bytes
set(FIFF_EXPLANATIONS_TXT "${CMAKE_CURRENT_SOURCE_DIR}/resources/fiff_explanations.txt")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${FIFF_EXPLANATIONS_TXT}")
file(READ "${FIFF_EXPLANATIONS_TXT}" FIFF_EXPLANATIONS_HEX HEX)
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," FIFF_EXPLANATIONS_BYTES "${FIFF_EXPLANATIONS_HEX}")
configure_file(resources/fiff_explanations.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/fiff_explanations.cpp" @ONLY)
list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/fiff_explanations.cpp")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

set(MNE_LIBS_REQUIRED 
    mne_base
    mne_math
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen)

# The public headers return std::expected.
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_FIFF_LIBRARY)

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
        DESTINATION include/mne/fiff/${_dir}
        COMPONENT sdk
    )
endforeach()

