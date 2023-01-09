cmake_minimum_required(VERSION 3.14)
project(mne_fwd LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent)

set(SOURCES
    computeFwd/compute_fwd_settings.cpp
    computeFwd/compute_fwd.cpp
    fwd_bem_model.cpp
    fwd_bem_solution.cpp
    fwd_coil.cpp
    fwd_coil_set.cpp
    fwd_comp_data.cpp
    fwd_eeg_sphere_layer.cpp
    fwd_eeg_sphere_model.cpp
    fwd_eeg_sphere_model_set.cpp
    fwd_global.cpp
    fwd_thread_arg.cpp
)

set(HEADERS
    fwd_global.h
    computeFwd/compute_fwd_settings.h
    computeFwd/compute_fwd.h
    fwd_bem_model.h
    fwd_bem_solution.h
    fwd_coil.h
    fwd_coil_set.h
    fwd_comp_data.h
    fwd_eeg_sphere_layer.h
    fwd_eeg_sphere_model.h
    fwd_eeg_sphere_model_set.h
    fwd_thread_arg.h
    fwd_types.h
)

set(FILE_TO_UPDATE fwd_global.cpp)

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
    eigen
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_FWD_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
