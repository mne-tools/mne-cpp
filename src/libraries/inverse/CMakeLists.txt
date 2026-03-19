cmake_minimum_required(VERSION 3.14)
project(mne_inv LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Concurrent)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    inv_global.cpp
    minimum_norm/inv_minimum_norm.cpp
    rap_music/inv_rap_music.cpp
    rap_music/inv_pwl_rap_music.cpp
    rap_music/inv_dipole.cpp
    inv_source_coupling.cpp
    inv_source_estimate.cpp
    inv_source_estimate_token.cpp
    dipole_fit/inv_dipole_fit.cpp
    dipole_fit/inv_dipole_fit_data.cpp
    dipole_fit/inv_dipole_fit_settings.cpp
    dipole_fit/inv_dipole_forward.cpp
    dipole_fit/inv_ecd.cpp
    dipole_fit/inv_ecd_set.cpp
    dipole_fit/inv_guess_data.cpp
    hpi/inv_hpi_data_updater.cpp
    hpi/inv_hpi_fit.cpp
    hpi/inv_hpi_fit_data.cpp
    hpi/inv_hpi_model_parameters.cpp
    hpi/inv_sensor_set.cpp
    hpi/inv_signal_model.cpp
)

set(HEADERS
    inv_global.h
    minimum_norm/inv_minimum_norm.h
    rap_music/inv_rap_music.h
    rap_music/inv_pwl_rap_music.h
    rap_music/inv_dipole.h
    inv_source_estimate.h
    inv_source_estimate_token.h
    inv_types.h
    inv_token.h
    inv_focal_dipole.h
    inv_source_coupling.h
    inv_connectivity.h
    dipole_fit/inv_dipole_fit.h
    dipole_fit/inv_dipole_fit_data.h
    dipole_fit/inv_dipole_fit_settings.h
    dipole_fit/inv_dipole_forward.h
    dipole_fit/inv_ecd.h
    dipole_fit/inv_ecd_set.h
    dipole_fit/inv_guess_data.h
    hpi/inv_hpi_data_updater.h
    hpi/inv_hpi_fit.h
    hpi/inv_hpi_fit_data.h
    hpi/inv_hpi_model_parameters.h
    hpi/inv_sensor_set.h
    hpi/inv_signal_model.h
)

set(FILE_TO_UPDATE inv_global.cpp)

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
    mne_math
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

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_INV_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

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

