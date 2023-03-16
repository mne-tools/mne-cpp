cmake_minimum_required(VERSION 3.14)
project(mne_inverse LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Concurrent)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    hpiFit/hpidataupdater.cpp
    hpiFit/hpimodelparameters.cpp
    hpiFit/signalmodel.cpp
    inverse_global.cpp
    minimumNorm/minimumnorm.cpp
    rapMusic/rapmusic.cpp
    rapMusic/pwlrapmusic.cpp
    rapMusic/dipole.cpp
    dipoleFit/dipole_fit.cpp
    dipoleFit/dipole_fit_data.cpp
    dipoleFit/dipole_fit_settings.cpp
    dipoleFit/dipole_forward.cpp
    dipoleFit/ecd.cpp
    dipoleFit/ecd_set.cpp
    dipoleFit/guess_data.cpp
    c/mne_inverse_operator.cpp
    c/mne_meas_data.cpp
    c/mne_meas_data_set.cpp
    hpiFit/hpifit.cpp
    hpiFit/hpifitdata.cpp
    hpiFit/sensorset.cpp
)

set(HEADERS
    hpiFit/hpidataupdater.h
    hpiFit/hpimodelparameters.h
    hpiFit/signalmodel.h
    inverse_global.h
    IInverseAlgorithm.h
    minimumNorm/minimumnorm.h
    rapMusic/rapmusic.h
    rapMusic/pwlrapmusic.h
    rapMusic/dipole.h
    dipoleFit/analyze_types.h
    dipoleFit/dipole_fit.h
    dipoleFit/dipole_fit_data.h
    dipoleFit/dipole_fit_settings.h
    dipoleFit/dipole_forward.h
    dipoleFit/ecd.h
    dipoleFit/ecd_set.h
    dipoleFit/guess_data.h
    c/mne_inverse_operator.h
    c/mne_meas_data.h
    c/mne_meas_data_set.h
    hpiFit/hpifit.h
    hpiFit/hpifitdata.h
    hpiFit/sensorset.h
)

set(FILE_TO_UPDATE inverse_global.cpp)

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
    Qt${QT_VERSION_MAJOR}::Concurrent
    eigen
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_INVERSE_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
