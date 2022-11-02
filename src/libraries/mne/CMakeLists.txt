cmake_minimum_required(VERSION 3.14)
project(mne_mne LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Network)

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

set(FILE_TO_UPDATE mne_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Network
    eigen
    mne_utils
    mne_fiff
    mne_fs
)

target_compile_definitions(${PROJECT_NAME} PRIVATE mne_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")
