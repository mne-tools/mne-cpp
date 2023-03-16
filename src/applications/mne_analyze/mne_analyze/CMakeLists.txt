cmake_minimum_required(VERSION 3.14)
project(mne_analyze LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets 3DRender Concurrent Network)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    main.cpp
    mainwindow.cpp
    analyzecore.cpp
)

set(HEADERS
    info.h
    mainwindow.h
    analyzecore.h
)

set(RESOURCES
    mne_analyze.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS} ${RESOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::3DRender
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    mne_rtprocessing
    mne_connectivity
    mne_inverse
    mne_fwd
    mne_mne
    mne_fiff
    mne_fs
    mne_utils
    mne_events
    mne_disp
    mne_disp3D
    eigen
    anShared
)

if(NOT BUILD_SHARED_LIBS)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        analyze_averaging
        analyze_channelselection
        analyze_controlmanager
        analyze_coregistration
        analyze_dataloader
        analyze_datamanager
        analyze_dipolefit
        analyze_events
        analyze_filtering
        analyze_rawdataviewer
        analyze_sourcelocalization
        analyze_view3d)

    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER mne-cpp.org
    MACOSX_BUNDLE ${BUILD_MAC_APP_BUNDLE}
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
