cmake_minimum_required(VERSION 3.14)
project(mne_edf2fiff LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Network)

set(SOURCES
    main.cpp
    edf_info.cpp
    edf_raw_data.cpp
    edf_ch_info.cpp
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
  Qt${QT_VERSION_MAJOR}::Network
  Qt${QT_VERSION_MAJOR}::Concurrent
  mne_fiff
  mne_utils
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

if(BUILD_SHARED_LIBS EQUAL OFF)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
