#==============================================================================================================
# @file     CMakeLists.txt
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.3.0
# @date     May, 2026
# @brief    Build file for the mne_doc_shots documentation screenshot tool.
#==============================================================================================================

cmake_minimum_required(VERSION 3.14)
project(mne_doc_shots LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets SerialPort)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    main.cpp
    shot_runner.cpp
    shot_widget_mockup.cpp
    shot_scene_only.cpp
    shot_app_common.cpp
    shot_mne_inspect_app.cpp
    shot_mne_align_app.cpp
)

set(HEADERS
    shot_runner.h
    shot_kinds.h
    shot_app_common.h
    shot_mne_inspect_app.h
    shot_mne_align_app.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_inspect_app_core
    mne_inspect_demo_fixtures
    mne_align_app_core
    mne_align_demo_fixtures
    inspect_electrodes
    inspect_mri_slices
    mne_disp3D
    mne_mri
    mne_fiff
    Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# Convenience target: rebuild every PNG referenced from the manual mdx pages.
# Usage:
#   cmake --build <build_dir> --target doc-shots
# The manifest lives at doc/website/screenshots/manifest.json; out_dir defaults
# to doc/website/static/img/manual/auto/ (relative to the manifest) and is
# kept out of git via doc/website/.gitignore.
set(_doc_shots_manifest "${CMAKE_SOURCE_DIR}/doc/website/screenshots/manifest.json")
set(_doc_shots_out      "${CMAKE_SOURCE_DIR}/doc/website/static/img/manual/auto")
add_custom_target(doc-shots
    COMMAND ${CMAKE_COMMAND} -E make_directory "${_doc_shots_out}"
    COMMAND $<TARGET_FILE:${PROJECT_NAME}>
            "${_doc_shots_manifest}"
            --out "${_doc_shots_out}"
    DEPENDS ${PROJECT_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating documentation screenshots via mne_doc_shots"
    USES_TERMINAL
    VERBATIM
)
