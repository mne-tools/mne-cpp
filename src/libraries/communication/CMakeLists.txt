cmake_minimum_required(VERSION 3.14)
project(mne_communication LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Network)

set(SOURCES
    communication_global.cpp
    rtClient/rtclient.cpp
    rtClient/rtdataclient.cpp
    rtClient/rtcmdclient.cpp
    rtCommand/command.cpp
    rtCommand/commandmanager.cpp
    rtCommand/commandparser.cpp
    rtCommand/rawcommand.cpp
)

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    eigen
    mne_utils
    mne_fiff
)

target_compile_definitions(${PROJECT_NAME} PRIVATE communication_LIBRARY)
