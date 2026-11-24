# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_anonymize_gui LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

# The Qt-free anonymization core of the mne_anonymize command-line tool
set(MNE_ANONYMIZE_CORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../tools/preprocessing/mne_anonymize)

set(SOURCES
    main.cpp
    mainwindow.cpp
    settingscontrollergui.cpp
    ${MNE_ANONYMIZE_CORE_DIR}/fiffanonymizer.cpp
    ${MNE_ANONYMIZE_CORE_DIR}/settingscontrollercl.cpp
)

set(HEADERS
    mainwindow.h
    settingscontrollergui.h
    ${MNE_ANONYMIZE_CORE_DIR}/fiffanonymizer.h
    ${MNE_ANONYMIZE_CORE_DIR}/settingscontrollercl.h
    ${MNE_ANONYMIZE_CORE_DIR}/mne_anonymize.h
)

set(FORMS
    mainwindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS} ${FORMS})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${FORMS})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${MNE_ANONYMIZE_CORE_DIR})

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_fiff
    mne_base
    Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.mne-cpp.mne_anonymize_gui
    MACOSX_BUNDLE ${BUILD_MAC_APP_BUNDLE}
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
