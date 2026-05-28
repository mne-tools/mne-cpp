# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# doc/CMakeLists.txt — wires the `docs_api` custom target which runs
# Doxygen against doc/Doxyfile to emit the XML AST consumed by
# tools/doxy2mdx/doxy2mdx.py (see v2.3.0-requirements.md TASK 18).

# Allow this directory to be configured standalone (handy for CI jobs
# that only build docs and skip the heavy C++ tree); when included via
# the top-level add_subdirectory() these are already in effect.
if(NOT PROJECT_NAME)
    cmake_minimum_required(VERSION 3.15)
    project(mne_cpp_docs LANGUAGES NONE)
endif()

find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(docs_api
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating Doxygen XML for the mne-cpp public API"
        VERBATIM)
else()
    message(STATUS "Doxygen not found — docs_api target unavailable")
endif()
