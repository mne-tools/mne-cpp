cmake_minimum_required(VERSION 3.14)
project(rtserver_fiffsimulator LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network)

set(SOURCES
    fiffsimulator.cpp 
    fiffproducer.cpp
)

set(HEADERS
    fiffsimulator.h
    fiffsimulator_global.h 
    fiffproducer.h 
    ../../mne_rt_server/IConnector.h
)

set(FILE_TO_UPDATE channelselection_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    eigen
    mne_utils
    mne_fiff
    mne_communication
)

target_compile_definitions(${PROJECT_NAME} PRIVATE RTSERVER_FIFFSIMULATOR_PLUGIN MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(BUILD_SHARED_LIBS EQUAL OFF)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
