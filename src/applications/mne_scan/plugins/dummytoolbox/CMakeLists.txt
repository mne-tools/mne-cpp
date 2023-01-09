cmake_minimum_required(VERSION 3.14)
project(scan_dummytoolbox LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

set(SOURCES
dummytoolbox.cpp \
FormFiles/dummysetupwidget.cpp \
FormFiles/dummyyourwidget.cpp
)

set(HEADERS
dummytoolbox.h\
dummytoolbox_global.h \
FormFiles/dummysetupwidget.h \
FormFiles/dummyyourwidget.h
)

set(FILE_TO_UPDATE dummytoolbox_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

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
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE SCAN_DUMMYTOOLBOX_PLUGIN MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD QT_STATICPLUGIN)
endif()
