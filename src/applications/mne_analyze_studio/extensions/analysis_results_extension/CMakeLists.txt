# Analysis result renderer extension widgets for MNE Analyze Studio.

cmake_minimum_required(VERSION 3.15)
project(mne_analyze_studio_analysis_results_extension LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

add_library(mne_analyze_studio_analysis_results_extension_widgets SHARED
    analysisresultswidget.cpp
    analysisresultswidget.h
)

set_target_properties(mne_analyze_studio_analysis_results_extension_widgets PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib
    RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib
)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_include_directories(mne_analyze_studio_analysis_results_extension_widgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mne_analyze_studio_analysis_results_extension_widgets PUBLIC
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_analyze_studio_sdk
)
