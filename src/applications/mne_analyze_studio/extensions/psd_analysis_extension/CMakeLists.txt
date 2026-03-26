# PSD analysis extension widgets for MNE Analyze Studio.

cmake_minimum_required(VERSION 3.15)
project(mne_analyze_studio_psd_analysis_extension LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

add_library(mne_analyze_studio_psd_analysis_extension_widgets ${MNE_ANALYZE_STUDIO_LIB_TYPE}
    psdresultwidget.cpp
    psdresultwidget.h
    spectrumplotwidget.cpp
    spectrumplotwidget.h
)

mne_analyze_studio_configure_library_target(mne_analyze_studio_psd_analysis_extension_widgets)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_include_directories(mne_analyze_studio_psd_analysis_extension_widgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mne_analyze_studio_psd_analysis_extension_widgets PUBLIC
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_analyze_studio_sdk
)
