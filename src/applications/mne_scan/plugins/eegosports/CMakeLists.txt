cmake_minimum_required(VERSION 3.14)
project(scan_eegosports LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets Svg)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

    set(SOURCES
    FormFiles/eegosportsimpedancewidget.cpp 
    eegosports.cpp 
    eegosports_global.cpp 
    eegosportselectrodeitem.cpp 
    eegosportsimpedancescene.cpp 
    eegosportsimpedanceview.cpp 
    eegosportsproducer.cpp 
    FormFiles/eegosportssetupwidget.cpp 
    eegosportsdriver.cpp 
    FormFiles/eegosportssetupprojectwidget.cpp 
)

set(HEADERS
    FormFiles/eegosportsimpedancewidget.h 
    eegosports.h
    eegosports_global.h 
    eegosportselectrodeitem.h 
    eegosportsimpedancescene.h 
    eegosportsimpedanceview.h 
    eegosportsproducer.h 
    FormFiles/eegosportssetupwidget.h 
    eegosportsdriver.h 
    FormFiles/eegosportssetupprojectwidget.h 
)

set(UI
    FormFiles/eegosportsimpedancewidget.ui
    FormFiles/eegosportssetup.ui
    FormFiles/eegosportssetupprojectwidget.ui
)

set(RESOURCES
    eegosports.qrc    
)

set(FILE_TO_UPDATE eegosports_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES} ${UI})

set(FFTW_LIBS "")

if(USE_FFTW)
  if (WIN32)
    set(FFTW_LIBS
      ${FFTW_DIR_LIBS}/libfftw3-3.dll
      ${FFTW_DIR_LIBS}/libfftw3f-3.dll
      ${FFTW_DIR_LIBS}/libfftwf3l-3.dll
    )
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_DIR_INCLUDE})
  elseif(UNIX AND NOT APPLE)
    set(FFTW_LIBS ${FFTW_DIR_LIBS}/lib/libfftw3.so)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_DIR_INCLUDE}/api)
  endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ../)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED 
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
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen
    scDisp
    scShared
    scMeas
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE SCAN_EEGOSPORTS_PLUGIN MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD QT_STATICPLUGIN)
endif()
