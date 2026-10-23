# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#[=======================================================================[.rst:
MneVersionInfo
--------------

Gives every first-party Windows executable and DLL a version resource with
``ProductName = MNE-CPP`` and ``ProductVersion = MNE_CPP_VERSION``, as the
SignPath Foundation requires for each file it signs.

.. command:: mne_add_windows_version_info

  ::

    mne_add_windows_version_info(<directory> <version>)

  Adds ``mne_version_info.rc.in`` to every executable, shared library and
  module target defined in ``<directory>`` or below. Static libraries carry
  no resources; targets under ``src/external`` are third-party and skipped.
  No-op outside Windows.
#]=======================================================================]

set(_MNE_VERSION_INFO_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/mne_version_info.rc.in")

function(_mne_collect_targets directory out)
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    get_property(children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
    foreach(child IN LISTS children)
        _mne_collect_targets("${child}" child_targets)
        list(APPEND targets ${child_targets})
    endforeach()
    set(${out} ${targets} PARENT_SCOPE)
endfunction()

function(mne_add_windows_version_info directory version)
    if(NOT WIN32)
        return()
    endif()
    if(NOT version MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)$")
        message(FATAL_ERROR "mne_add_windows_version_info: '${version}' is not MAJOR.MINOR.PATCH")
    endif()
    set(MNE_RC_VERSION "${version}")
    set(MNE_RC_VERSION_COMMA "${CMAKE_MATCH_1},${CMAKE_MATCH_2},${CMAKE_MATCH_3},0")
    _mne_collect_targets("${directory}" targets)
    foreach(target IN LISTS targets)
        get_target_property(type ${target} TYPE)
        get_target_property(source_dir ${target} SOURCE_DIR)
        if(source_dir MATCHES "/src/external(/|$)")
            continue()
        endif()
        if(type STREQUAL "EXECUTABLE")
            set(MNE_RC_FILETYPE "0x1L")
        elseif(type STREQUAL "SHARED_LIBRARY" OR type STREQUAL "MODULE_LIBRARY")
            set(MNE_RC_FILETYPE "0x2L")
        else()
            continue()
        endif()
        set(MNE_RC_INTERNAL_NAME "${target}")
        set(MNE_RC_DESCRIPTION "MNE-CPP ${target}")
        set(rc "${CMAKE_BINARY_DIR}/version_info/${target}.rc")
        configure_file("${_MNE_VERSION_INFO_TEMPLATE}" "${rc}" @ONLY)
        target_sources(${target} PRIVATE "${rc}")
    endforeach()
endfunction()
