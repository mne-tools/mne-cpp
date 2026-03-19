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
    mne_source_spaces.cpp
    mne_hemisphere.cpp
    mne_epoch_data.cpp
    mne_epoch_data_list.cpp
    mne_cluster_info.cpp
    mne_bem.cpp
    mne_bem_surface.cpp
    mne_project_to_surface.cpp
    mne_icp.cpp
    mne_cov_matrix.cpp
    mne_ctf_comp_data.cpp
    mne_ctf_comp_data_set.cpp
    mne_deriv.cpp
    mne_deriv_set.cpp
    mne_mne_data.cpp
    mne_named_matrix.cpp
    mne_named_vector.cpp
    mne_proj_item.cpp
    mne_proj_op.cpp
    mne_raw_buf_def.cpp
    mne_raw_data.cpp
    mne_raw_info.cpp
    mne_sss_data.cpp
    mne_triangle.cpp
    mne_vol_geom.cpp
    mne_nearest.cpp
    mne_patch_info.cpp
    mne_source_space.cpp
    mne_surface.cpp
    mne_surface_or_volume.cpp
    filter_thread_arg.cpp
    mne_msh_display_surface.cpp
    mne_msh_display_surface_set.cpp
    mne_msh_picked.cpp
    mne_morph_map.cpp
    mne_msh_color_scale_def.cpp
    mne_proj_data.cpp
    mne_msh_light.cpp
    mne_msh_light_set.cpp
    mne_surface_patch.cpp
    mne_msh_eyes.cpp
    mne_mgh_tag.cpp
    mne_mgh_tag_group.cpp
    mne_description_parser.cpp
    mne_forward_solution.cpp
    mne_inverse_operator.cpp
    mne_meas_data.cpp
    mne_meas_data_set.cpp
)

set(HEADERS
    mne.h
    mne_global.h
    mne_source_spaces.h
    mne_hemisphere.h
    mne_epoch_data.h
    mne_epoch_data_list.h
    mne_cluster_info.h
    mne_bem.h
    mne_bem_surface.h
    mne_project_to_surface.h
    mne_icp.h
    mne_cov_matrix.h
    mne_ctf_comp_data.h
    mne_ctf_comp_data_set.h
    mne_deriv.h
    mne_deriv_set.h
    mne_mne_data.h
    mne_named_matrix.h
    mne_named_vector.h
    mne_sparse_named_matrix.h
    mne_layout_port.h
    mne_layout.h
    mne_ch_selection.h
    mne_event.h
    mne_event_list.h
    mne_filter_def.h
    mne_proj_item.h
    mne_proj_op.h
    mne_raw_buf_def.h
    mne_raw_data.h
    mne_raw_info.h
    mne_sss_data.h
    mne_triangle.h
    mne_types.h
    mne_vol_geom.h
    mne_nearest.h
    mne_patch_info.h
    mne_source_space.h
    mne_surface.h
    mne_surface_or_volume.h
    filter_thread_arg.h
    mne_msh_display_surface.h
    mne_msh_display_surface_set.h
    mne_msh_picked.h
    mne_morph_map.h
    mne_msh_color_scale_def.h
    mne_proj_data.h
    mne_msh_light.h
    mne_msh_light_set.h
    mne_surface_patch.h
    mne_msh_eyes.h
    mne_mgh_tag.h
    mne_mgh_tag_group.h
    mne_process_description.h
    mne_description_parser.h
    mne_forward_solution.h
    mne_inverse_operator.h
    mne_meas_data.h
    mne_meas_data_set.h
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

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED 
    mne_utils
    mne_math
    mne_fiff
    mne_fs
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_MNE_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(MSVC)
    set_source_files_properties(mne_forward_solution.cpp PROPERTIES COMPILE_FLAGS "/bigobj")
endif()

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

