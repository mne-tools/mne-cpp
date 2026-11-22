# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_fwd LANGUAGES CXX)

set(SOURCES
    compute_fwd/compute_fwd.cpp
    compute_fwd/compute_fwd_settings.cpp
    fwd_bem_model.cpp
    fwd_bem_solution.cpp
    fwd_coil.cpp
    fwd_coil_set.cpp
    fwd_comp_data.cpp
    fwd_eeg_sphere_layer.cpp
    fwd_eeg_sphere_model.cpp
    fwd_eeg_sphere_model_set.cpp
    fwd_field_map.cpp
    fwd_thread_arg.cpp
)

set(HEADERS
    fwd.h
    fwd_global.h
    compute_fwd/compute_fwd.h
    compute_fwd/compute_fwd_settings.h
    fwd_bem_model.h
    fwd_bem_solution.h
    fwd_coil.h
    fwd_field_map.h
    fwd_coil_set.h
    fwd_comp_data.h
    fwd_eeg_sphere_layer.h
    fwd_eeg_sphere_model.h
    fwd_eeg_sphere_model_set.h
    fwd_thread_arg.h
    fwd_types.h
)

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

set(MNE_LIBS_REQUIRED 
    mne_base
    mne_math
    mne_fiff
    mne_fs
    mne_mne
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_FWD_LIBRARY)

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
        DESTINATION include/fwd/${_dir}
        COMPONENT sdk
    )
endforeach()

