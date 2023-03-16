cmake_minimum_required(VERSION 3.14)
project(anShared LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets Svg)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    Management/analyzedata.cpp
    Management/pluginmanager.cpp
    Management/event.cpp
    Management/communicator.cpp
    Management/eventmanager.cpp
    Management/statusbar.cpp
    Model/bemdatamodel.cpp
    Model/dipolefitmodel.cpp
    Model/fiffrawviewmodel.cpp
    Model/eventmodel.cpp
    Model/averagingdatamodel.cpp
    Model/mricoordmodel.cpp
    Model/covariancemodel.cpp
    Plugins/abstractplugin.cpp
)

set(HEADERS
    Model/dipolefitmodel.h
    Model/mricoordmodel.h
    Model/covariancemodel.h
    Plugins/abstractplugin.h
    anshared_global.h
    Model/abstractmodel.h
    Management/analyzedata.h
    Management/pluginmanager.h
    Management/event.h
    Management/communicator.h
    Management/eventmanager.h
    Management/statusbar.h
    Utils/metatypes.h
    Utils/types.h
    Model/bemdatamodel.h
    Model/fiffrawviewmodel.h
    Model/eventmodel.h
    Model/averagingdatamodel.h
)

# set(FILE_TO_UPDATE anshared_global.cpp)

# set(SOURCE_PATHS ${SOURCES})
# list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
# set_source_files_properties(${FILE_TO_UPDATE}
#   PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

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

set(QT_REQUIRED_COMPONENT_LIBS QT_REQUIRED_COMPONENTS)
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
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
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME}
  PRIVATE ANSHARED_LIBRARY
  MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}"
  MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
