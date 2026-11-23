# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(test_mne_scan_plugins LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Test Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SCAN_PLUGINS scan_averaging scan_brainamp scan_covariance scan_hpi scan_neuronalconnectivity scan_noisereduction scan_rtclustmne scan_rtfwd scan_tmsi scan_gusbamp scan_eegosports scan_neuromag scan_ftbuffer scan_fiffsimulator scan_natus scan_babymeg scan_brainflowboard scan_lsladapter)

enable_testing()
mne_add_test(${PROJECT_NAME}
    LABELS integration gui
    TIMEOUT 120
    REQUIRES_DATA
        mne-cpp-test-data/MEG/sample
    REQUIRES_TARGETS ${SCAN_PLUGINS})
if(MNE_TEST_SKIPPED)
    return()
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION test_mne_scan_plugins.cpp)
else()
    add_executable(${PROJECT_NAME} test_mne_scan_plugins.cpp)
endif()

list(TRANSFORM QT_REQUIRED_COMPONENTS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/apps/mne_scan/plugins
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENTS}
    ${SCAN_PLUGINS}
    mne_scan_app_core
    scShared
    scMeas
    scDisp
    mne_fiff
    mne_base
    Eigen3::Eigen
)

# A stand-in for BrainFlow's library, from test_acq_devices
if(TARGET brainflow_stub)
    add_dependencies(${PROJECT_NAME} brainflow_stub)
    target_compile_definitions(${PROJECT_NAME} PRIVATE BRAINFLOW_STUB="$<TARGET_FILE:brainflow_stub>")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

foreach(plugin IN LISTS SCAN_PLUGINS)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:${plugin}> $<TARGET_FILE_DIR:${PROJECT_NAME}>)
endforeach()

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "@executable_path/../lib;@executable_path")
    set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT
        "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:scShared>:$<TARGET_FILE_DIR:${PROJECT_NAME}>;QT_QPA_PLATFORM=offscreen")
elseif(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "\$ORIGIN/../lib:\$ORIGIN")
    set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT
        "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:scShared>:$<TARGET_FILE_DIR:${PROJECT_NAME}>;QT_QPA_PLATFORM=offscreen")
else()
    set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
