# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#[=======================================================================[.rst:
MneTesting
----------

Registers MNE-CPP tests with CTest so that every test states what it is, how
long it may run, and what it needs.

.. command:: mne_add_test

  ::

    mne_add_test(<name>
                 LABELS <label>...
                 TIMEOUT <seconds>
                 [COMMAND <command> [<arg>...]]
                 [WORKING_DIRECTORY <dir>]
                 [ENVIRONMENT <VAR=value>...]
                 [REQUIRES_DATA <path>...]
                 [REQUIRES_TARGETS <target>...]
                 [PLATFORMS <Linux|Darwin|Windows>... SKIP_REASON <text>])

  ``LABELS`` and ``TIMEOUT`` are mandatory. The label vocabulary and the timeout
  band are read from ``tools/quality/test_inventory_policy.json``, the same file
  ``tools/quality/validate_test_inventory.py`` enforces.

  Without ``COMMAND`` the test runs the executable target ``<name>`` exactly as
  a bare ``add_test(NAME <name> COMMAND <name>)`` would; the target's name and
  output location are not touched.

  ``REQUIRES_DATA`` paths are relative to ``MNE_TEST_DATA_DIR`` (default:
  ``resources/data``). They become ``REQUIRED_FILES``, so a missing file makes
  CTest report the test as not run, and the ``requires-data`` label is added.

  When ``REQUIRES_TARGETS`` names a target this configuration does not build,
  or ``PLATFORMS`` excludes the current platform, a placeholder test with the
  same name is registered and reported as *skipped* with the reason. The test
  stays visible in CTest and JUnit instead of disappearing. The caller's
  ``MNE_TEST_SKIPPED`` is set accordingly, so a leaf can register first and
  only then ``return()`` before building against targets that do not exist.

  Every test carries ``MNE_TEST_HELPER`` plus ``MNE_REQUIRES_DATA``,
  ``MNE_REQUIRES_TARGETS``, ``MNE_PLATFORMS`` and ``MNE_SKIP_REASON`` where set,
  all visible in ``ctest --show-only=json-v1``.
#]=======================================================================]

include_guard(GLOBAL)

if(CMAKE_VERSION VERSION_LESS 3.19)
    message(FATAL_ERROR "MneTesting.cmake needs CMake 3.19 or newer for string(JSON).")
endif()

get_filename_component(_mne_testing_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(_mne_testing_policy_file "${_mne_testing_repo_root}/tools/quality/test_inventory_policy.json")

file(READ "${_mne_testing_policy_file}" _mne_testing_policy)
string(JSON _mne_testing_label_count ERROR_VARIABLE _mne_testing_json_error
       LENGTH "${_mne_testing_policy}" allowed_labels)
if(_mne_testing_json_error)
    message(FATAL_ERROR "${_mne_testing_policy_file}: cannot read 'allowed_labels': ${_mne_testing_json_error}")
endif()
set(_mne_testing_allowed_labels "")
math(EXPR _mne_testing_last_label "${_mne_testing_label_count} - 1")
foreach(_mne_testing_index RANGE ${_mne_testing_last_label})
    string(JSON _mne_testing_label GET "${_mne_testing_policy}" allowed_labels ${_mne_testing_index})
    list(APPEND _mne_testing_allowed_labels "${_mne_testing_label}")
endforeach()
string(JSON _mne_testing_timeout_min GET "${_mne_testing_policy}" timeout_seconds min)
string(JSON _mne_testing_timeout_max GET "${_mne_testing_policy}" timeout_seconds max)

set_property(GLOBAL PROPERTY MNE_TESTING_ALLOWED_LABELS "${_mne_testing_allowed_labels}")
set_property(GLOBAL PROPERTY MNE_TESTING_TIMEOUT_MIN "${_mne_testing_timeout_min}")
set_property(GLOBAL PROPERTY MNE_TESTING_TIMEOUT_MAX "${_mne_testing_timeout_max}")
set_property(GLOBAL PROPERTY MNE_TESTING_DEFAULT_DATA_DIR "${_mne_testing_repo_root}/resources/data")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_mne_testing_policy_file}")

function(mne_add_test name)
    cmake_parse_arguments(PARSE_ARGV 1 arg
        ""
        "TIMEOUT;WORKING_DIRECTORY;SKIP_REASON"
        "LABELS;COMMAND;ENVIRONMENT;REQUIRES_DATA;REQUIRES_TARGETS;PLATFORMS")

    set(where "mne_add_test(${name})")
    get_property(allowed_labels GLOBAL PROPERTY MNE_TESTING_ALLOWED_LABELS)
    get_property(timeout_min GLOBAL PROPERTY MNE_TESTING_TIMEOUT_MIN)
    get_property(timeout_max GLOBAL PROPERTY MNE_TESTING_TIMEOUT_MAX)
    string(REPLACE ";" ", " allowed_text "${allowed_labels}")

    if(arg_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "${where}: unexpected argument(s): ${arg_UNPARSED_ARGUMENTS}")
    endif()
    if(arg_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR "${where}: keyword(s) given without a value: ${arg_KEYWORDS_MISSING_VALUES}")
    endif()

    if(NOT arg_LABELS)
        message(FATAL_ERROR "${where}: LABELS is required. Allowed labels: ${allowed_text}.")
    endif()
    foreach(label IN LISTS arg_LABELS)
        if(NOT label IN_LIST allowed_labels)
            message(FATAL_ERROR "${where}: unknown label '${label}'. Allowed labels: ${allowed_text}.")
        endif()
    endforeach()

    if(NOT DEFINED arg_TIMEOUT)
        message(FATAL_ERROR "${where}: TIMEOUT is required, in whole seconds from ${timeout_min} to ${timeout_max}.")
    endif()
    if(NOT arg_TIMEOUT MATCHES "^[0-9]+$")
        message(FATAL_ERROR "${where}: TIMEOUT '${arg_TIMEOUT}' is not a whole number of seconds.")
    endif()
    if(arg_TIMEOUT LESS timeout_min OR arg_TIMEOUT GREATER timeout_max)
        message(FATAL_ERROR "${where}: TIMEOUT ${arg_TIMEOUT} is outside ${timeout_min}..${timeout_max} seconds.")
    endif()

    set(labels ${arg_LABELS})
    set(required_files "")
    if(arg_REQUIRES_DATA)
        if(DEFINED MNE_TEST_DATA_DIR)
            set(data_dir "${MNE_TEST_DATA_DIR}")
        else()
            get_property(data_dir GLOBAL PROPERTY MNE_TESTING_DEFAULT_DATA_DIR)
        endif()
        foreach(data_path IN LISTS arg_REQUIRES_DATA)
            if(IS_ABSOLUTE "${data_path}" OR data_path MATCHES "(^|[/\\\\])\\.\\.([/\\\\]|$)")
                message(FATAL_ERROR "${where}: REQUIRES_DATA '${data_path}' must be a relative path inside ${data_dir}.")
            endif()
            list(APPEND required_files "${data_dir}/${data_path}")
        endforeach()
        list(APPEND labels requires-data)
    endif()
    list(REMOVE_DUPLICATES labels)

    set(known_platforms Linux Darwin Windows)
    foreach(platform IN LISTS arg_PLATFORMS)
        if(NOT platform IN_LIST known_platforms)
            message(FATAL_ERROR "${where}: unknown platform '${platform}'. Use Linux, Darwin, or Windows.")
        endif()
    endforeach()
    if(arg_PLATFORMS AND NOT arg_SKIP_REASON)
        message(FATAL_ERROR "${where}: PLATFORMS needs a SKIP_REASON saying why other platforms cannot run it.")
    endif()
    if(arg_SKIP_REASON AND NOT arg_PLATFORMS)
        message(FATAL_ERROR "${where}: SKIP_REASON is only meaningful together with PLATFORMS.")
    endif()

    set(skip_reasons "")
    if(arg_PLATFORMS AND NOT CMAKE_SYSTEM_NAME IN_LIST arg_PLATFORMS)
        string(REPLACE ";" ", " platform_text "${arg_PLATFORMS}")
        list(APPEND skip_reasons "runs only on ${platform_text}: ${arg_SKIP_REASON}")
    endif()
    set(missing_targets "")
    foreach(target IN LISTS arg_REQUIRES_TARGETS)
        if(NOT TARGET ${target})
            list(APPEND missing_targets ${target})
        endif()
    endforeach()
    if(missing_targets)
        string(REPLACE ";" ", " missing_text "${missing_targets}")
        list(APPEND skip_reasons "this configuration does not build ${missing_text}")
    endif()

    if(skip_reasons)
        list(JOIN skip_reasons ". " skip_reason)
        string(REPLACE ";" "," skip_reason "${skip_reason}")
        add_test(NAME ${name} COMMAND "${CMAKE_COMMAND}" -E echo "MNE_TEST_SKIPPED: ${skip_reason}")
        set_property(TEST ${name} PROPERTY SKIP_REGULAR_EXPRESSION "MNE_TEST_SKIPPED:")
        set_property(TEST ${name} PROPERTY MNE_SKIP_REASON "${skip_reason}")
        set(MNE_TEST_SKIPPED TRUE PARENT_SCOPE)
    else()
        if(arg_COMMAND)
            set(command ${arg_COMMAND})
        else()
            set(command ${name})
            # The executable is usually created after registration, so check it once every target exists.
            # EVAL bakes in the name: DEFER CALL would expand variables only when it runs.
            cmake_language(EVAL CODE
                "cmake_language(DEFER DIRECTORY [[${CMAKE_SOURCE_DIR}]] CALL _mne_testing_check_executable [[${name}]])")
        endif()
        add_test(NAME ${name} COMMAND ${command})
        set(MNE_TEST_SKIPPED FALSE PARENT_SCOPE)
        if(arg_WORKING_DIRECTORY)
            set_property(TEST ${name} PROPERTY WORKING_DIRECTORY "${arg_WORKING_DIRECTORY}")
        endif()
        if(arg_ENVIRONMENT)
            set_property(TEST ${name} PROPERTY ENVIRONMENT ${arg_ENVIRONMENT})
        endif()
        if(required_files)
            set_property(TEST ${name} PROPERTY REQUIRED_FILES ${required_files})
        endif()
    endif()

    set_property(TEST ${name} PROPERTY LABELS ${labels})
    set_property(TEST ${name} PROPERTY TIMEOUT ${arg_TIMEOUT})
    set_property(TEST ${name} PROPERTY MNE_TEST_HELPER mne_add_test)
    if(arg_REQUIRES_DATA)
        set_property(TEST ${name} PROPERTY MNE_REQUIRES_DATA ${arg_REQUIRES_DATA})
    endif()
    if(arg_REQUIRES_TARGETS)
        set_property(TEST ${name} PROPERTY MNE_REQUIRES_TARGETS ${arg_REQUIRES_TARGETS})
    endif()
    if(arg_PLATFORMS)
        set_property(TEST ${name} PROPERTY MNE_PLATFORMS ${arg_PLATFORMS})
    endif()
endfunction()

function(_mne_testing_check_executable name)
    if(NOT TARGET ${name})
        message(FATAL_ERROR "mne_add_test(${name}): no executable target named '${name}' was created. "
                            "Create it, pass COMMAND, or declare REQUIRES_TARGETS.")
    endif()
    get_target_property(target_type ${name} TYPE)
    if(NOT target_type STREQUAL "EXECUTABLE")
        message(FATAL_ERROR "mne_add_test(${name}): target '${name}' is a ${target_type}, not an executable.")
    endif()
endfunction()
