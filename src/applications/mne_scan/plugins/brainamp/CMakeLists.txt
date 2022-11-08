cmake_minimum_required(VERSION 3.14)
project(scan_brainamp LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

set(SOURCES
    brainamp.cpp
    brainamp_global.cpp
    brainampproducer.cpp
    FormFiles/brainampsetupwidget.cpp
    brainampdriver.cpp
    FormFiles/brainampsetupprojectwidget.cpp
)

set(HEADERS
    brainamp.h\
    brainamp_global.h
    brainampproducer.h
    FormFiles/brainampsetupwidget.h
    brainampdriver.h
    FormFiles/brainampsetupprojectwidget.h
)

set(RESOURCES
    brainamp.qrc    
)

set(FILE_TO_UPDATE babymeg_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    eigen
    mne_disp
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
    mne_inverse
    mne_rtprocessing
    mne_connectivity
    mne_events
    scDisp
    scShared
    scMeas
)

target_compile_definitions(${PROJECT_NAME} PRIVATE scan_brainamp_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")
