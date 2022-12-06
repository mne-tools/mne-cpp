cmake_minimum_required(VERSION 3.14)
project(scan_gusbamp LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

set(SOURCES
    gusbamp.cpp 
    gusbamp_global.cpp 
    gusbampproducer.cpp 
    gusbampdriver.cpp 
    FormFiles/gusbampsetupwidget.cpp 
    FormFiles/gusbampsetupprojectwidget.cpp

)

set(HEADERS
    gusbamp.h
    gusbamp_global.h 
    gusbampproducer.h 
    gusbampdriver.h 
    FormFiles/gusbampsetupwidget.h 
    FormFiles/gusbampsetupprojectwidget.h
)

set(RESOURCES
    gusbamp.qrc    
)

set(FILE_TO_UPDATE gusbamp_global.cpp)

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

target_compile_definitions(${PROJECT_NAME} PRIVATE SCAN_GUSBAMP_PLUGIN MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
