# Embedded FIFF browser extension widgets for MNE Analyze Studio.

cmake_minimum_required(VERSION 3.15)
project(mne_analyze_studio_fiff_browser_extension LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Concurrent Network Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

add_library(mne_analyze_studio_fiff_browser_extension_widgets ${MNE_ANALYZE_STUDIO_LIB_TYPE}
    mnebrowserrawdelegate.cpp
    rawdatabrowserwidget.cpp
    mnebrowserrawdelegate.h
    rawdatabrowserwidget.h
    ../../../mne_browse/Models/rawmodel.cpp
    ../../../mne_browse/Models/eventmodel.cpp
    ../../../mne_browse/Utils/datapackage.cpp
    ../../../mne_browse/Utils/filteroperator.cpp
    ../../../mne_browse/Utils/rawsettings.cpp
)

mne_analyze_studio_configure_library_target(mne_analyze_studio_fiff_browser_extension_widgets)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_include_directories(mne_analyze_studio_fiff_browser_extension_widgets PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../mne_browse
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(mne_analyze_studio_fiff_browser_extension_widgets PRIVATE STATICBUILD)
endif()

target_link_libraries(mne_analyze_studio_fiff_browser_extension_widgets PUBLIC
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_analyze_studio_sdk
    mne_disp
    mne_dsp
    mne_fiff
    mne_fs
    mne_fwd
    mne_inv
    mne_math
    mne_mne
    mne_utils
    eigen
)
