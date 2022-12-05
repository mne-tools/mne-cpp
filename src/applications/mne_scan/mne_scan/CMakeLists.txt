cmake_minimum_required(VERSION 3.14)
project(mne_scan LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets 3DRender Concurrent Network Xml)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets 3DRender Concurrent Network Xml)

set(SOURCES
    main.cpp 
    mainsplashscreencloser.cpp 
    startupwidget.cpp 
    mainsplashscreen.cpp 
    pluginscene.cpp 
    pluginitem.cpp 
    plugingui.cpp 
    arrow.cpp 
    mainwindow.cpp
)

set(HEADERS
    info.h 
    mainsplashscreencloser.h 
    startupwidget.h 
    mainsplashscreen.h 
    pluginscene.h 
    pluginitem.h 
    plugingui.h 
    arrow.h 
    mainwindow.h
)

set(RESOURCES
    mne_scan.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS} ${RESOURCES})
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS} ${RESOURCES})
    else()
        add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Widgets
  Qt${QT_VERSION_MAJOR}::3DRender
  Qt${QT_VERSION_MAJOR}::Concurrent
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Xml
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
  scDisp
  scShared
  scMeas
)

if(BUILD_SHARED_LIBS EQUAL OFF)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        scan_averaging
        scan_babymeg
        scan_covariance
        scan_fiffsimulator
        scan_ftbuffer
        scan_hpi
        scan_natus
        scan_neuronalconnectivity
        scan_noisereduction
        scan_rtcmne
        scan_rtfwd
        scan_writetofile)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE ${BUILD_MAC_APP_BUNDLE}
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
