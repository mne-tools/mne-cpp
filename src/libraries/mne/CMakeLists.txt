cmake_minimum_required(VERSION 3.14)
project(mne_mne LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Concurrent Network)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    mne.cpp
    mne_global.cpp
    mne_sourcespace.cpp
    mne_forwardsolution.cpp
    mne_sourceestimate.cpp
    mne_hemisphere.cpp
    mne_inverse_operator.cpp
    mne_epoch_data.cpp
    mne_epoch_data_list.cpp
    mne_cluster_info.cpp
    mne_surface.cpp
    mne_corsourceestimate.cpp
    mne_bem.cpp
    mne_bem_surface.cpp
    mne_project_to_surface.cpp
    c/mne_cov_matrix.cpp
    c/mne_ctf_comp_data.cpp
    c/mne_ctf_comp_data_set.cpp
    c/mne_deriv.cpp
    c/mne_deriv_set.cpp
    c/mne_mne_data.cpp
    c/mne_named_matrix.cpp
    c/mne_proj_item.cpp
    c/mne_proj_op.cpp
    c/mne_raw_buf_def.cpp
    c/mne_raw_data.cpp
    c/mne_raw_info.cpp
    c/mne_sss_data.cpp
    c/mne_triangle.cpp
    c/mne_vol_geom.cpp
    c/mne_nearest.cpp
    c/mne_patch_info.cpp
    c/mne_source_space_old.cpp
    c/mne_surface_old.cpp
    c/mne_surface_or_volume.cpp
    c/filter_thread_arg.cpp
    c/mne_msh_display_surface.cpp
    c/mne_msh_display_surface_set.cpp
    c/mne_msh_picked.cpp
    c/mne_morph_map.cpp
    c/mne_msh_color_scale_def.cpp
    c/mne_proj_data.cpp
    c/mne_msh_light.cpp
    c/mne_msh_light_set.cpp
    c/mne_surface_patch.cpp
    c/mne_msh_eyes.cpp
    c/mne_mgh_tag.cpp
    c/mne_mgh_tag_group.cpp
)

set(HEADERS
    mne.h
    mne_global.h
    mne_sourcespace.h
    mne_hemisphere.h
    mne_forwardsolution.h
    mne_sourceestimate.h
    mne_inverse_operator.h
    mne_epoch_data.h
    mne_epoch_data_list.h
    mne_cluster_info.h
    mne_surface.h
    mne_corsourceestimate.h
    mne_bem.h
    mne_bem_surface.h
    mne_project_to_surface.h
    c/mne_cov_matrix.h
    c/mne_ctf_comp_data.h
    c/mne_ctf_comp_data_set.h
    c/mne_deriv.h
    c/mne_deriv_set.h
    c/mne_mne_data.h
    c/mne_named_matrix.h
    c/mne_proj_item.h
    c/mne_proj_op.h
    c/mne_raw_buf_def.h
    c/mne_raw_data.h
    c/mne_raw_info.h
    c/mne_sss_data.h
    c/mne_triangle.h
    c/mne_types.h
    c/mne_types_mne-c.h
    c/mne_vol_geom.h
    c/mne_nearest.h
    c/mne_patch_info.h
    c/mne_source_space_old.h
    c/mne_surface_old.h
    c/mne_surface_or_volume.h
    c/filter_thread_arg.h
    c/mne_msh_display_surface.h
    c/mne_msh_display_surface_set.h
    c/mne_msh_picked.h
    c/mne_morph_map.h
    c/mne_msh_color_scale_def.h
    c/mne_proj_data.h
    c/mne_msh_light.h
    c/mne_msh_light_set.h
    c/mne_surface_patch.h
    c/mne_msh_eyes.h
    c/mne_mgh_tag.h
    c/mne_mgh_tag_group.h
)

set(FILE_TO_UPDATE mne_global.cpp)

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
    Qt${QT_VERSION_MAJOR}::Network
    eigen
    mne_utils
    mne_fiff
    mne_fs
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_MNE_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
