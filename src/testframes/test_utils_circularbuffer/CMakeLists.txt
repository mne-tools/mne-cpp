cmake_minimum_required(VERSION 3.14)
project(test_utils_circularbuffer LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets 3DRender Concurrent Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets 3DRender Concurrent Network)

set(SOURCES
    test_utils_circularbuffer.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED ${SOURCES})
    else()
        add_executable(${PROJECT_NAME} ${SOURCES})
    endif()
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
)

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
