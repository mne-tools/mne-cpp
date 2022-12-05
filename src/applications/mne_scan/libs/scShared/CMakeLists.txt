cmake_minimum_required(VERSION 3.14)
project(scShared LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Svg Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Svg Network)

set(SOURCES
    Management/pluginmanager.cpp 
    Management/pluginconnector.cpp 
    Management/plugininputconnector.cpp 
    Management/pluginoutputconnector.cpp 
    Management/plugininputdata.cpp 
    Management/pluginoutputdata.cpp 
    Management/pluginconnectorconnection.cpp 
    Management/pluginconnectorconnectionwidget.cpp 
    Management/pluginscenemanager.cpp 
    Management/displaymanager.cpp
)

set(HEADERS
    scshared_global.h 
    Plugins/abstractplugin.h 
    Plugins/abstractsensor.h 
    Plugins/abstractalgorithm.h
    Management/pluginmanager.h 
    Management/pluginconnector.h 
    Management/plugininputconnector.h 
    Management/pluginoutputconnector.h 
    Management/plugininputdata.h 
    Management/pluginoutputdata.h 
    Management/pluginconnectorconnection.h 
    Management/pluginconnectorconnectionwidget.h 
    Management/pluginscenemanager.h 
    Management/displaymanager.h
)

# set(FILE_TO_UPDATE scShared_global.cpp)

# set(SOURCE_PATHS ${SOURCES})
# list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
# set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Network
    eigen
    mne_disp
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
    mne_inverse
    mne_rtprocessing
    mne_events
    scMeas
    scDisp
)

target_compile_definitions(${PROJECT_NAME} PRIVATE SCSHARED_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(BUILD_SHARED_LIBS EQUAL OFF)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
