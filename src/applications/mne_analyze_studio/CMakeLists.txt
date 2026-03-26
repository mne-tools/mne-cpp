# MNE Analyze Studio application family.

cmake_minimum_required(VERSION 3.15)
project(mne_analyze_studio LANGUAGES CXX)

# WASM (Emscripten) does not support traditional shared libraries: side-module
# .so files embed their static dependencies which causes duplicate-symbol errors
# at final link time. Build everything as STATIC when targeting WASM.
if(EMSCRIPTEN OR WASM)
    set(MNE_ANALYZE_STUDIO_LIB_TYPE STATIC)
else()
    set(MNE_ANALYZE_STUDIO_LIB_TYPE SHARED)
endif()

function(mne_analyze_studio_configure_library_target target_name)
    set(_studio_library_output_directory "${BINARY_OUTPUT_DIRECTORY}/lib")

    if(WIN32)
        set(_studio_library_output_directory "${BINARY_OUTPUT_DIRECTORY}/bin")
    endif()

    set_target_properties(${target_name} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${_studio_library_output_directory}"
        ARCHIVE_OUTPUT_DIRECTORY "${_studio_library_output_directory}"
        RUNTIME_OUTPUT_DIRECTORY "${_studio_library_output_directory}"
    )

    foreach(_config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
        set_property(TARGET ${target_name} PROPERTY "LIBRARY_OUTPUT_DIRECTORY_${_config}"
            "${_studio_library_output_directory}")
        set_property(TARGET ${target_name} PROPERTY "ARCHIVE_OUTPUT_DIRECTORY_${_config}"
            "${_studio_library_output_directory}")
        set_property(TARGET ${target_name} PROPERTY "RUNTIME_OUTPUT_DIRECTORY_${_config}"
            "${_studio_library_output_directory}")
    endforeach()

    get_target_property(_studio_target_type ${target_name} TYPE)
    if(WIN32 AND _studio_target_type STREQUAL "SHARED_LIBRARY")
        set_target_properties(${target_name} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    endif()
endfunction()

add_subdirectory(sdk)
add_subdirectory(core)
add_subdirectory(extensions/analysis_results_extension)
add_subdirectory(extensions/dummy_3d_extension)
add_subdirectory(extensions/fiff_browser_extension)
add_subdirectory(extensions/psd_analysis_extension)
add_subdirectory(extensions/temporal_filter_skill)
add_subdirectory(extensions/source_estimation_skill)
add_subdirectory(workbench)
add_subdirectory(neuro_kernel)
add_subdirectory(skill_host)
