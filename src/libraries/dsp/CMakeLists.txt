cmake_minimum_required(VERSION 3.14)
project(mne_dsp
  LANGUAGES CXX
)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Concurrent Network)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
  dsp_global.cpp
  filterkernel.cpp
  cosinefilter.cpp
  parksmcclellan.cpp
  filterio.cpp
  spectrogram.cpp
  sphara.cpp
  rt/rt_cov.cpp
  rt/rt_inv_op.cpp
  rt/rt_averaging.cpp
  rt/rt_noise.cpp
  rt/rt_hpis.cpp
  rt/rt_filter.cpp
  rt/rt_connectivity.cpp
  rt/rt_detect_trigger.cpp
  ica.cpp
  iirfilter.cpp
  sss.cpp
  xdawn.cpp
  artifact_detect.cpp
  firfilter.cpp
  resample.cpp
  epoch_extractor.cpp
  bad_channel_detect.cpp
  welch_psd.cpp
  morlet_tfr.cpp
  dpss.cpp
  multitaper_psd.cpp
  multitaper_tfr.cpp
  channel_derivation.cpp
  csd.cpp
  extended_infomax.cpp
)

set(HEADERS
  dsp_global.h
  filterkernel.h
  cosinefilter.h
  parksmcclellan.h
  filterio.h
  spectrogram.h
  sphara.h
  rt/rt_cov.h
  rt/rt_inv_op.h
  rt/rt_averaging.h
  rt/rt_noise.h
  rt/rt_hpis.h
  rt/rt_filter.h
  rt/rt_detect_trigger.h
  rt/rt_connectivity.h
  ica.h
  iirfilter.h
  sss.h
  xdawn.h
  artifact_detect.h
  firfilter.h
  resample.h
  epoch_extractor.h
  bad_channel_detect.h
  welch_psd.h
  morlet_tfr.h
  dpss.h
  multitaper_psd.h
  multitaper_tfr.h
  channel_derivation.h
  csd.h
  extended_infomax.h
)

set(FILE_TO_UPDATE dsp_global.cpp)

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

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:include>)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_utils
    mne_math
    mne_fiff
    mne_mne
    mne_conn
    mne_fwd
    mne_inv
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_DSP_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

# Install public headers for SDK
foreach(_header ${HEADERS})
    get_filename_component(_dir "${_header}" DIRECTORY)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/dsp/${_dir}
        COMPONENT sdk
    )
endforeach()
