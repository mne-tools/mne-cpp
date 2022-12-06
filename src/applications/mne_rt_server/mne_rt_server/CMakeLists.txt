cmake_minimum_required(VERSION 3.14)
project(mne_rt_server LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Concurrent Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Concurrent Network)

set(SOURCES
    main.cpp 
    connectormanager.cpp 
    mne_rt_server.cpp 
    fiffstreamserver.cpp 
    fiffstreamthread.cpp 
    commandserver.cpp 
    commandthread.cpp
)

set(HEADERS
    IConnector.h 
    connectormanager.h 
    mne_rt_server.h 
    fiffstreamserver.h 
    fiffstreamthread.h 
    commandserver.h 
    commandthread.h 
    mne_rt_commands.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED ${SOURCES} ${HEADERS})
    else()
        add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
    endif()
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    mne_connectivity
    mne_communication
    mne_fiff
    mne_utils
    eigen
)

if(BUILD_SHARED_LIBS EQUAL OFF)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        rtserver_fiffsimulator)
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

if(BUILD_SHARED_LIBS EQUAL OFF)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
