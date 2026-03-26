# Workbench executable for MNE Analyze Studio.

cmake_minimum_required(VERSION 3.15)
project(mne_analyze_studio_workbench LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Concurrent Network Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    analysisresultwidget.cpp
    agentchatdockwidget.cpp
    editortabbar.cpp
    extensionsettingswidget.cpp
    extensionhostedviewwidget.cpp
    llmsettingsdialog.cpp
    main.cpp
    mainwindow.cpp
    pillselectorwidget.cpp
    workflowminimapwidget.cpp
)

set(HEADERS
    analysisresultwidget.h
    agentchatdockwidget.h
    editortabbar.h
    extensionsettingswidget.h
    extensionhostedviewwidget.h
    editortabwidget.h
    llmsettingsdialog.h
    mainwindow.h
    pillselectorwidget.h
    workflowminimapwidget.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_analyze_studio_analysis_results_extension_widgets
    mne_analyze_studio_core
    mne_analyze_studio_dummy_3d_extension_widgets
    mne_analyze_studio_fiff_browser_extension_widgets
    mne_analyze_studio_psd_analysis_extension_widgets
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

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
