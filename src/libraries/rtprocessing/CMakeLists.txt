cmake_minimum_required(VERSION 3.14)
project(mne_rtprocessing LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Network)

set(SOURCES
    icp.cpp
    rtcov.cpp
    rtinvop.cpp
    averaging.cpp
    rtaveraging.cpp
    rtnoise.cpp
    rthpis.cpp
    filter.cpp
    rtconnectivity.cpp
    rtprocessing_global.cpp
    sphara.cpp
    detecttrigger.cpp
    helpers/cosinefilter.cpp
    helpers/parksmcclellan.cpp
    helpers/filterkernel.cpp
    helpers/filterio.cpp
)

set(HEADERS
    icp.h
    rtprocessing_global.h
    rtcov.h
    rtinvop.h
    averaging.h
    rtaveraging.h
    rtnoise.h
    rthpis.h
    filter.h
    detecttrigger.h
    sphara.h
    rtconnectivity.h
    helpers/cosinefilter.h
    helpers/parksmcclellan.h
    helpers/filterkernel.h
    helpers/filterio.h
)

set(FILE_TO_UPDATE rtprocessing_global.cpp.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set(FFTW_LIBS "")

if(USE_FFTW)
  if (WIN32)
    set(FFTW_LIBS
      ${FFTW_DIR_LIBS}/libfftw3-3
      ${FFTW_DIR_LIBS}/libfftw3f-3
      ${FFTW_DIR_LIBS}/libfftwf3l-3
    )
  elseif(UNIX AND NOT APPLE)
    set(FFTW_LIBS ${FFTW_DIR_LIBS}/libfftw3.so)
  endif()
  target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_DIR_INCLUDE})
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    eigen
    mne_utils
    mne_fiff
    mne_mne
    mne_connectivity
    mne_fwd
    mne_inverse
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_RTPROCESSING_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
