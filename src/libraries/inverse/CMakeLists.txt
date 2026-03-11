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
    inverse_global.cpp
    minimum_norm/minimum_norm.cpp
    rap_music/rap_music.cpp
    rap_music/pwl_rap_music.cpp
    rap_music/dipole.cpp
    mne_inverse_operator.cpp
    mne_meas_data.cpp
    mne_meas_data_set.cpp
    mne_source_estimate.cpp
    mne_cor_source_estimate.cpp
    dipole_fit/dipole_fit.cpp
    dipole_fit/dipole_fit_data.cpp
    dipole_fit/dipole_fit_settings.cpp
    dipole_fit/dipole_forward.cpp
    dipole_fit/ecd.cpp
    dipole_fit/ecd_set.cpp
    dipole_fit/guess_data.cpp
    hpi/hpi_data_updater.cpp
    hpi/hpi_fit.cpp
    hpi/hpi_fit_data.cpp
    hpi/hpi_model_parameters.cpp
    hpi/sensor_set.cpp
    hpi/signal_model.cpp
)

set(HEADERS
    inverse_global.h
    minimum_norm/minimum_norm.h
    rap_music/rap_music.h
    rap_music/pwl_rap_music.h
    rap_music/dipole.h
    mne_inverse_operator.h
    mne_meas_data.h
    mne_meas_data_set.h
    mne_source_estimate.h
    mne_cor_source_estimate.h
    dipole_fit/analyze_types.h
    dipole_fit/dipole_fit.h
    dipole_fit/dipole_fit_data.h
    dipole_fit/dipole_fit_settings.h
    dipole_fit/dipole_forward.h
    dipole_fit/ecd.h
    dipole_fit/ecd_set.h
    dipole_fit/guess_data.h
    hpi/hpi_data_updater.h
    hpi/hpi_fit.h
    hpi/hpi_fit_data.h
    hpi/hpi_model_parameters.h
    hpi/sensor_set.h
    hpi/signal_model.h
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

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED 
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_INVERSE_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

# Install public headers for SDK
foreach(_header ${HEADERS})
    get_filename_component(_dir "${_header}" DIRECTORY)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/${PROJECT_NAME}/${_dir}
        COMPONENT sdk
    )
endforeach()

