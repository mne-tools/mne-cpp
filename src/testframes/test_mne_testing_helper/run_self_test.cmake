# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

# Self-test for cmake/MneTesting.cmake. Configures throwaway projects under
# WORK_DIR and checks what the helper accepts, what it refuses at configure
# time, and what it publishes through ctest --show-only=json-v1.

cmake_minimum_required(VERSION 3.19)

foreach(required MNE_TESTING_MODULE WORK_DIR GENERATOR)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "run_self_test.cmake: -D${required}=... is required.")
    endif()
endforeach()

file(REMOVE_RECURSE "${WORK_DIR}")
file(MAKE_DIRECTORY "${WORK_DIR}")
set_property(GLOBAL PROPERTY MNE_SELF_TEST_FAILURES "")

function(_fail case text)
    message(STATUS "FAIL ${case}: ${text}")
    set_property(GLOBAL APPEND PROPERTY MNE_SELF_TEST_FAILURES "${case}")
endfunction()

function(_pass case)
    message(STATUS "PASS ${case}")
endfunction()

# Writes and configures a fixture project; LANGUAGES NONE keeps the refusal cases compiler-free.
function(_configure case languages body out_result out_output)
    set(source_dir "${WORK_DIR}/${case}/src")
    set(binary_dir "${WORK_DIR}/${case}/build")
    file(WRITE "${source_dir}/CMakeLists.txt"
        "cmake_minimum_required(VERSION 3.19)\n"
        "project(${case} LANGUAGES ${languages})\n"
        "enable_testing()\n"
        "include([[${MNE_TESTING_MODULE}]])\n"
        "${body}\n")

    set(args -S "${source_dir}" -B "${binary_dir}" -G "${GENERATOR}")
    if(GENERATOR_PLATFORM)
        list(APPEND args -A "${GENERATOR_PLATFORM}")
    endif()
    if(GENERATOR_TOOLSET)
        list(APPEND args -T "${GENERATOR_TOOLSET}")
    endif()
    if(languages STREQUAL "CXX" AND CXX_COMPILER AND NOT GENERATOR MATCHES "^Visual Studio")
        list(APPEND args "-DCMAKE_CXX_COMPILER=${CXX_COMPILER}")
    endif()

    execute_process(COMMAND "${CMAKE_COMMAND}" ${args}
        RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE output)
    # CMake wraps diagnostics at ~80 columns; collapse whitespace so messages can be matched.
    string(REGEX REPLACE "[ \t\r\n]+" " " output "${output}")
    set(${out_result} "${result}" PARENT_SCOPE)
    set(${out_output} "${output}" PARENT_SCOPE)
endfunction()

function(_expect_refusal case body pattern)
    _configure(${case} NONE "${body}" result output)
    if(result EQUAL 0)
        _fail(${case} "configured successfully but must be a configure error")
    elseif(NOT output MATCHES "${pattern}")
        _fail(${case} "configure failed without the expected message '${pattern}':\n${output}")
    else()
        _pass(${case})
    endif()
endfunction()

function(_find_test json name out)
    set(${out} "" PARENT_SCOPE)
    string(JSON count LENGTH "${json}" tests)
    if(count EQUAL 0)
        return()
    endif()
    math(EXPR last "${count} - 1")
    foreach(index RANGE ${last})
        string(JSON test_name GET "${json}" tests ${index} name)
        if(test_name STREQUAL name)
            string(JSON test GET "${json}" tests ${index})
            set(${out} "${test}" PARENT_SCOPE)
            return()
        endif()
    endforeach()
endfunction()

# Returns a JSON array as a CMake list, a number without its trailing ".0", or "<unset>".
function(_property test name out)
    set(${out} "<unset>" PARENT_SCOPE)
    string(JSON count ERROR_VARIABLE json_error LENGTH "${test}" properties)
    if(json_error OR count EQUAL 0)
        return()
    endif()
    math(EXPR last "${count} - 1")
    foreach(index RANGE ${last})
        string(JSON property_name GET "${test}" properties ${index} name)
        if(NOT property_name STREQUAL name)
            continue()
        endif()
        string(JSON type TYPE "${test}" properties ${index} value)
        if(type STREQUAL "ARRAY")
            set(values "")
            string(JSON length LENGTH "${test}" properties ${index} value)
            if(length GREATER 0)
                math(EXPR last_value "${length} - 1")
                foreach(value_index RANGE ${last_value})
                    string(JSON value GET "${test}" properties ${index} value ${value_index})
                    list(APPEND values "${value}")
                endforeach()
            endif()
            set(value "${values}")
        else()
            string(JSON value GET "${test}" properties ${index} value)
            if(type STREQUAL "NUMBER")
                string(REGEX REPLACE "\\.0+$" "" value "${value}")
            endif()
        endif()
        set(${out} "${value}" PARENT_SCOPE)
        return()
    endforeach()
endfunction()

function(_expect_property case test name expected)
    _property("${test}" ${name} actual)
    if(NOT actual STREQUAL "<unset>" AND NOT expected STREQUAL "<unset>")
        list(SORT actual)
        list(SORT expected)
    endif()
    if(NOT "${actual}" STREQUAL "${expected}")
        _fail(${case} "${name} is '${actual}', expected '${expected}'")
    endif()
endfunction()

# --------------------------------------------------------------------------------------------------------
# Refusals: every one of these must stop the configure step (AC-T1.1-2).
# --------------------------------------------------------------------------------------------------------

_expect_refusal(missing_labels
    "mne_add_test(t TIMEOUT 60 COMMAND \${CMAKE_COMMAND} -E true)"
    "LABELS is required")
_expect_refusal(missing_timeout
    "mne_add_test(t LABELS unit COMMAND \${CMAKE_COMMAND} -E true)"
    "TIMEOUT is required")
_expect_refusal(unknown_label
    "mne_add_test(t LABELS unit fast TIMEOUT 60 COMMAND \${CMAKE_COMMAND} -E true)"
    "unknown label 'fast'")
_expect_refusal(timeout_below_band
    "mne_add_test(t LABELS unit TIMEOUT 1 COMMAND \${CMAKE_COMMAND} -E true)"
    "TIMEOUT 1 is outside")
_expect_refusal(timeout_above_band
    "mne_add_test(t LABELS unit TIMEOUT 999999 COMMAND \${CMAKE_COMMAND} -E true)"
    "TIMEOUT 999999 is outside")
_expect_refusal(timeout_not_integer
    "mne_add_test(t LABELS unit TIMEOUT 1.5 COMMAND \${CMAKE_COMMAND} -E true)"
    "not a whole number")
_expect_refusal(unexpected_argument
    "mne_add_test(t LABELS unit TIMEOUT 60 SERIAL COMMAND \${CMAKE_COMMAND} -E true)"
    "unexpected argument")
_expect_refusal(unknown_platform
    "mne_add_test(t LABELS unit TIMEOUT 60 PLATFORMS Solaris SKIP_REASON x COMMAND \${CMAKE_COMMAND} -E true)"
    "unknown platform 'Solaris'")
_expect_refusal(platforms_without_reason
    "mne_add_test(t LABELS unit TIMEOUT 60 PLATFORMS Linux COMMAND \${CMAKE_COMMAND} -E true)"
    "PLATFORMS needs a SKIP_REASON")
_expect_refusal(reason_without_platforms
    "mne_add_test(t LABELS unit TIMEOUT 60 SKIP_REASON x COMMAND \${CMAKE_COMMAND} -E true)"
    "SKIP_REASON is only meaningful")
_expect_refusal(data_path_escapes
    "mne_add_test(t LABELS unit TIMEOUT 60 REQUIRES_DATA ../secret COMMAND \${CMAKE_COMMAND} -E true)"
    "must be a relative path")
_expect_refusal(missing_executable
    "mne_add_test(test_never_created LABELS unit TIMEOUT 60)"
    "no executable target named 'test_never_created'")
_expect_refusal(target_not_executable
    "mne_add_test(test_interface LABELS unit TIMEOUT 60)\nadd_library(test_interface INTERFACE)"
    "is a INTERFACE_LIBRARY, not an executable")

# --------------------------------------------------------------------------------------------------------
# Accepted registrations and the metadata they publish (AC-T1.1-1, AC-T1.1-3).
# --------------------------------------------------------------------------------------------------------

if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(OTHER_PLATFORM Linux)
else()
    set(OTHER_PLATFORM Windows)
endif()
set(DATA_DIR "${WORK_DIR}/data")
file(WRITE "${DATA_DIR}/sample/raw.fif" "fixture")

set(accepted_body [==[
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/out/tests)
file(WRITE ${CMAKE_BINARY_DIR}/main.cpp "int main() { return 0; }\n")
set(MNE_TEST_DATA_DIR [[@DATA_DIR@]])

mne_add_test(test_helper
    LABELS unit
    TIMEOUT 120
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    ENVIRONMENT QT_QPA_PLATFORM=offscreen MNE_FIXTURE=1
    REQUIRES_DATA sample/raw.fif)
if(MNE_TEST_SKIPPED)
    message(FATAL_ERROR "test_helper must not be reported as skipped")
endif()
add_executable(test_helper ${CMAKE_BINARY_DIR}/main.cpp)

add_executable(test_plain ${CMAKE_BINARY_DIR}/main.cpp)
add_test(NAME test_plain COMMAND test_plain)

mne_add_test(test_skip_target LABELS gui TIMEOUT 60 REQUIRES_TARGETS mne_target_that_is_never_built)
if(NOT MNE_TEST_SKIPPED)
    message(FATAL_ERROR "test_skip_target must be reported as skipped")
endif()

mne_add_test(test_skip_platform LABELS unit TIMEOUT 60 PLATFORMS @OTHER_PLATFORM@ SKIP_REASON "fixture constraint")
]==])
string(CONFIGURE "${accepted_body}" accepted_body @ONLY)

_configure(accepted CXX "${accepted_body}" result output)
if(NOT result EQUAL 0)
    _fail(accepted "configure failed:\n${output}")
else()
    set(binary_dir "${WORK_DIR}/accepted/build")
    string(REGEX REPLACE "([][+.*()^$?|\\\\])" "\\\\\\1" binary_dir_regex "${binary_dir}")
    # CTest lists a test's command only once the executable exists.
    execute_process(COMMAND "${CMAKE_COMMAND}" --build "${binary_dir}" --config Release
        RESULT_VARIABLE build_result OUTPUT_VARIABLE build_output ERROR_VARIABLE build_output)
    if(NOT build_result EQUAL 0)
        _fail(accepted "fixture build failed:\n${build_output}")
    endif()
    execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" -C Release --show-only=json-v1
        WORKING_DIRECTORY "${binary_dir}"
        RESULT_VARIABLE ctest_result OUTPUT_VARIABLE ctest_json ERROR_VARIABLE ctest_error)
    if(NOT ctest_result EQUAL 0)
        _fail(accepted "ctest --show-only=json-v1 failed: ${ctest_error}")
    else()
        _find_test("${ctest_json}" test_helper helper)
        _find_test("${ctest_json}" test_plain plain)
        _find_test("${ctest_json}" test_skip_target skip_target)
        _find_test("${ctest_json}" test_skip_platform skip_platform)

        if(NOT helper OR NOT plain OR NOT skip_target OR NOT skip_platform)
            _fail(accepted "CTest does not list every fixture test:\n${ctest_json}")
        else()
            # AC-T1.1-1: the helper runs the same binary a bare add_test() would.
            string(JSON helper_command ERROR_VARIABLE helper_error GET "${helper}" command 0)
            string(JSON plain_command ERROR_VARIABLE plain_error GET "${plain}" command 0)
            get_filename_component(helper_dir "${helper_command}" DIRECTORY)
            get_filename_component(plain_dir "${plain_command}" DIRECTORY)
            get_filename_component(helper_stem "${helper_command}" NAME_WE)
            if(helper_error OR plain_error)
                _fail(output_path "CTest has no resolvable command: ${helper_error} ${plain_error}")
            elseif(NOT helper_dir STREQUAL plain_dir)
                _fail(output_path "helper runs from '${helper_dir}', add_test() from '${plain_dir}'")
            elseif(NOT helper_dir MATCHES "/out/tests(/[^/]+)?$" OR NOT helper_dir MATCHES "^${binary_dir_regex}")
                _fail(output_path "'${helper_dir}' is not inside CMAKE_RUNTIME_OUTPUT_DIRECTORY")
            elseif(NOT helper_stem STREQUAL "test_helper")
                _fail(output_path "executable is '${helper_stem}', expected 'test_helper'")
            else()
                _pass(output_path)
            endif()

            # AC-T1.1-3: the declared metadata is visible to CTest.
            _expect_property(metadata "${helper}" LABELS "requires-data;unit")
            _expect_property(metadata "${helper}" TIMEOUT 120)
            _expect_property(metadata "${helper}" ENVIRONMENT "QT_QPA_PLATFORM=offscreen;MNE_FIXTURE=1")
            _expect_property(metadata "${helper}" WORKING_DIRECTORY "${binary_dir}")
            _expect_property(metadata "${helper}" REQUIRED_FILES "${DATA_DIR}/sample/raw.fif")
            _expect_property(metadata "${helper}" MNE_REQUIRES_DATA "sample/raw.fif")
            _expect_property(metadata "${helper}" MNE_TEST_HELPER mne_add_test)
            _expect_property(metadata "${plain}" MNE_TEST_HELPER "<unset>")

            _expect_property(skip_target "${skip_target}" LABELS gui)
            _expect_property(skip_target "${skip_target}" TIMEOUT 60)
            _expect_property(skip_target "${skip_target}" MNE_REQUIRES_TARGETS mne_target_that_is_never_built)
            _expect_property(skip_target "${skip_target}" MNE_SKIP_REASON
                "this configuration does not build mne_target_that_is_never_built")
            _expect_property(skip_target "${skip_target}" SKIP_REGULAR_EXPRESSION "MNE_TEST_SKIPPED:")

            _expect_property(skip_platform "${skip_platform}" MNE_PLATFORMS ${OTHER_PLATFORM})
            _expect_property(skip_platform "${skip_platform}" MNE_SKIP_REASON
                "runs only on ${OTHER_PLATFORM}: fixture constraint")

            get_property(failures GLOBAL PROPERTY MNE_SELF_TEST_FAILURES)
            if(NOT failures MATCHES "metadata|skip_target|skip_platform")
                _pass(metadata)
            endif()
        endif()

        # A skipped registration must surface as skipped in the run, not vanish or pass.
        execute_process(COMMAND "${CMAKE_CTEST_COMMAND}" -C Release -R "^test_skip_"
                --output-junit "${binary_dir}/skipped.xml"
            WORKING_DIRECTORY "${binary_dir}"
            RESULT_VARIABLE run_result OUTPUT_VARIABLE run_output ERROR_VARIABLE run_output)
        file(READ "${binary_dir}/skipped.xml" junit)
        string(REGEX MATCHALL "SKIP_REGULAR_EXPRESSION_MATCHED" skipped_cases "${junit}")
        list(LENGTH skipped_cases skipped_count)
        if(NOT run_result EQUAL 0 OR NOT skipped_count EQUAL 2)
            _fail(skipped_run "expected two skipped JUnit cases and success:\n${run_output}\n${junit}")
        else()
            _pass(skipped_run)
        endif()
    endif()
endif()

get_property(failures GLOBAL PROPERTY MNE_SELF_TEST_FAILURES)
if(failures)
    list(REMOVE_DUPLICATES failures)
    message(FATAL_ERROR "MneTesting self-test failed: ${failures}")
endif()
message(STATUS "MneTesting self-test passed.")
