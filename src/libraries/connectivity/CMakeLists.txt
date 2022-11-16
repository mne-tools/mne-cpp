cmake_minimum_required(VERSION 3.14)
project(mne_connectivity LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Network)

set(SOURCES
    connectivity_global.cpp
    metrics/abstractmetric.cpp
    metrics/correlation.cpp
    metrics/crosscorrelation.cpp
    metrics/coherency.cpp
    metrics/coherence.cpp
    metrics/imagcoherence.cpp
    metrics/unbiasedsquaredphaselagindex.cpp
    metrics/phaselockingvalue.cpp
    metrics/weightedphaselagindex.cpp
    metrics/debiasedsquaredweightedphaselagindex.cpp
    metrics/phaselagindex.cpp
    network/network.cpp
    network/networknode.cpp
    network/networkedge.cpp
    connectivitysettings.cpp
    connectivity.cpp
)

set(FILE_TO_UPDATE connectivity_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    eigen
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_CONNECTIVITY_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")
