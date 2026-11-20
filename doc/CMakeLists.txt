# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# doc/CMakeLists.txt — wires the `mne_cpp_docs_api` custom target which runs
# Doxygen against doc/Doxyfile to emit the XML AST consumed by
# scripts/doxy2mdx/doxy2mdx.py.
#
# The `mne_cpp_` prefix keeps the target apart from a `docs_api` target of a
# project that includes MNE-CPP.

# Allow this directory to be configured standalone (handy for CI jobs
# that only build docs and skip the heavy C++ tree); when included via
# the top-level add_subdirectory() these are already in effect.
if(NOT PROJECT_NAME)
    cmake_minimum_required(VERSION 3.15)
    project(mne_cpp_docs LANGUAGES NONE)
endif()

find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(mne_cpp_docs_api
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating Doxygen XML for the mne-cpp public API"
        VERBATIM)
else()
    message(STATUS "Doxygen not found -- mne_cpp_docs_api target unavailable")
endif()

# Documentation screenshot generator (compiled C++ tool, NOT a user-facing CLI
# tool — lives here rather than src/tools/).  It links the MNE Inspect and MNE
# Align application cores, so it is built only where those are (not without
# applications or without Qt, MNE_BUILD_QT=OFF).
option(BUILD_MNE_DOC_SHOTS "Build MNE documentation screenshot generator" ON)
set(_mne_doc_shots_apps mne_inspect_app_core mne_inspect_demo_fixtures inspect_electrodes inspect_mri_slices
                        mne_align_app_core mne_align_demo_fixtures)
set(_mne_doc_shots_ready ON)
foreach(_mne_doc_shots_app IN LISTS _mne_doc_shots_apps)
    if(NOT TARGET ${_mne_doc_shots_app})
        set(_mne_doc_shots_ready OFF)
    endif()
endforeach()
if(BUILD_MNE_DOC_SHOTS AND NOT WASM AND _mne_doc_shots_ready)
    add_subdirectory(tools/doc_shots)
endif()
