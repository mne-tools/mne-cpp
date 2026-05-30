cmake_minimum_required(VERSION 3.14)
project(test_cortical_surface_inverse LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

if(NOT TARGET analyze_cortical_surface)
    message(STATUS "test_cortical_surface_inverse: analyze_cortical_surface not built — skipping")
    return()
endif()

set(SOURCES
    test_cortical_surface_inverse.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    analyze_cortical_surface
    anShared
    mne_disp3D
    mne_inv
    mne_fs
    mne_mne
    mne_fiff
    mne_utils
    mne_math
    Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/out/${CMAKE_BUILD_TYPE}/tests"
    FOLDER "tests"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# analyze_cortical_surface is an mne_analyze plugin that is built into
# out/<config>/bin/mne_analyze_plugins, not next to the test binary. The CI
# test runner launches the test executable directly (not through CTest), so the
# CTest ENVIRONMENT property below is not honoured in that path; on Windows the
# loader then fails with STATUS_DLL_NOT_FOUND. Copy the plugin next to the test
# binary so the loader resolves it regardless of how the test is launched.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:analyze_cortical_surface>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

set(_repo_lib_dir   "${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/lib")
set(_repo_plugins   "${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/bin/mne_analyze_plugins")
if(APPLE)
    set_tests_properties(${PROJECT_NAME} PROPERTIES
        ENVIRONMENT "DYLD_LIBRARY_PATH=${_repo_lib_dir}:${_repo_plugins}"
    )
elseif(UNIX)
    set_tests_properties(${PROJECT_NAME} PROPERTIES
        ENVIRONMENT "LD_LIBRARY_PATH=${_repo_lib_dir}:${_repo_plugins}"
    )
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
