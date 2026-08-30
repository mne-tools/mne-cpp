cmake_minimum_required(VERSION 3.14)
project(test_tool_data_pipeline LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Test)

set(SOURCES
    test_tool_data_pipeline.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Test
    mne_fiff
    mne_inv
    mne_mne
    Eigen3::Eigen
)

foreach(CLI_TOOL IN ITEMS
    mne_add_triggers
    mne_average_estimates
    mne_check_eeg_locations
    mne_compute_mne
    mne_convert_ncov
    mne_dacq_annotator
    mne_evoked_data_summary
    mne_fix_stim14
    mne_insert_4D_comp
    mne_list_coil_def
    mne_list_proj
    mne_label_ssp
    mne_map_data
    mne_make_uniform_stc
    mne_process_stc
    mne_sensor_locations
    mne_transform_points
    mne_volume_data2mri
)
    if(TARGET ${CLI_TOOL})
        add_dependencies(${PROJECT_NAME} ${CLI_TOOL})
    endif()
endforeach()

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_tests_properties(${PROJECT_NAME} PROPERTIES TIMEOUT 600)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
