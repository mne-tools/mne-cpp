# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_mna
  LANGUAGES CXX
)

set(SOURCES
  mna_types.cpp
  mna_file_ref.cpp
  mna_recording.cpp
  mna_session.cpp
  mna_subject.cpp
  mna_project.cpp
  mna_io.cpp
  mna_port.cpp
  mna_node.cpp
  mna_op_schema.cpp
  mna_op_registry.cpp
  mna_registry_loader.cpp
  mna_param_binding.cpp
  mna_param_tree.cpp
  mna_graph.cpp
  mna_graph_executor.cpp
  mna_script.cpp
  mna_verification.cpp
)

set(HEADERS
  mna_global.h
  mna_types.h
  mna_codec.h
  mna_file_ref.h
  mna_recording.h
  mna_session.h
  mna_subject.h
  mna_project.h
  mna_io.h
  mna_port.h
  mna_node.h
  mna_op_schema.h
  mna_op_registry.h
  mna_registry_loader.h
  mna_param_binding.h
  mna_param_tree.h
  mna_graph.h
  mna_graph_executor.h
  mna_script.h
  mna_verification.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:include>)

# The public headers use the base JSON, CBOR and error types
target_link_libraries(${PROJECT_NAME} PUBLIC mne_base)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_MNA_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

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
        DESTINATION include/mna/${_dir}
        COMPONENT sdk
    )
endforeach()
