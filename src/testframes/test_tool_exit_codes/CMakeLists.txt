cmake_minimum_required(VERSION 3.14)
project(test_tool_exit_codes LANGUAGES CXX)

# Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED
    COMPONENTS ${QT_REQUIRED_COMPONENTS}
)

set(SOURCES
    test_tool_exit_codes.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_utils
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
)

# Keep process-driven tests tied to the current tool sources. Optional tools
# that are disabled in a build are not targets and remain runtime skips.
set(CLI_TOOL_TARGETS
    mne_brain_vision2fiff
    mne_convert_ctf_markers
    mne_convert_dig_data
    mne_convert_lspcov
    mne_convert_ncov
    mne_convert_surface
    mne_ctf_dig2fiff
    mne_ctf2fiff
    mne_edf2fiff
    mne_epochs2mat
    mne_eximia2fiff
    mne_kit2fiff
    mne_make_cor_set
    mne_mna_bids_converter
    mne_raw2mat
    mne_tufts2fiff
    mne_average_forward_solutions
    mne_check_surface
    mne_fit_sphere_to_surf
    mne_forward_solution
    mne_make_scalp_surfaces
    mne_make_source_space
    mne_make_sphere_bem
    mne_prepare_bem_model
    mne_transform_points
    mne_check_eeg_locations
    mne_collect_transforms
    mne_compare_fif_files
    mne_evoked_data_summary
    mne_list_bem
    mne_list_coil_def
    mne_list_proj
    mne_list_source_space
    mne_sensor_locations
    mne_show_mna
    mne_average_estimates
    mne_compute_cmne
    mne_compute_mne
    mne_compute_raw_inverse
    mne_dipole_fit
    mne_inverse_operator
    mne_inverse_pipeline
    mne_label_ssp
    mne_make_movie
    mne_make_uniform_stc
    mne_map_data
    mne_process_stc
    mne_sensitivity_map
    mne_smooth
    mne_volume_data2mri
    mne_add_to_meas_info
    mne_add_triggers
    mne_anonymize
    mne_change_baselines
    mne_change_nave
    mne_compensate_data
    mne_copy_processing_history
    mne_cov2proj
    mne_create_comp_data
    mne_dacq_annotator
    mne_fix_mag_coil_types
    mne_fix_stim14
    mne_insert_4D_comp
    mne_make_derivations
    mne_mark_bad_channels
    mne_process_raw
    mne_rename_channels
    mne_toggle_skips
    mne_add_patch_info
    mne_annot2labels
    mne_make_eeg_layout
    mne_make_morph_maps
    mne_morph_labels
    mne_volume_source_space
    mne_rt_server
    mne_simu
    mne_screenshot_regression
)

foreach(CLI_TOOL_TARGET IN LISTS CLI_TOOL_TARGETS)
    if(TARGET ${CLI_TOOL_TARGET})
        add_dependencies(${PROJECT_NAME} ${CLI_TOOL_TARGET})
    endif()
endforeach()

set_target_properties(${PROJECT_NAME} PROPERTIES
  WIN32_EXECUTABLE FALSE
  MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# Register with CTest. This test spawns one child process per case, so it is
# slower than an in-process unit test but nowhere near the long-running ones.
enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_tests_properties(${PROJECT_NAME} PROPERTIES
    LABELS "integration"
    TIMEOUT 300
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
