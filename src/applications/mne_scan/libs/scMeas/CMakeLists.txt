cmake_minimum_required(VERSION 3.14)
project(scMeas LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Svg)

set(SOURCES
    realtimesourceestimate.cpp
    realtimeconnectivityestimate.cpp
    realtimemultisamplearray.cpp
    realtimesamplearraychinfo.cpp
    numeric.cpp
    measurement.cpp
    measurementtypes.cpp
    realtimeevokedset.cpp
    realtimecov.cpp
    realtimehpiresult.cpp
    realtimespectrum.cpp
    realtimefwdsolution.cpp
)

set(HEADERS
    scmeas_global.h
    realtimesourceestimate.h
    realtimeconnectivityestimate.h
    realtimemultisamplearray.h
    realtimesamplearraychinfo.h
    numeric.h
    measurement.h
    measurementtypes.h
    realtimeevokedset.h
    realtimecov.h
    realtimehpiresult.h
    realtimespectrum.h
    realtimefwdsolution.h
)

# set(FILE_TO_UPDATE scMeas_global.cpp)

# set(SOURCE_PATHS ${SOURCES})
# list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
# set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    eigen
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
    mne_connectivity
)

target_compile_definitions(${PROJECT_NAME} PRIVATE SCMEAS_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
