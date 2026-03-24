# MNE Analyze Studio application family.

cmake_minimum_required(VERSION 3.15)
project(mne_analyze_studio LANGUAGES CXX)

add_subdirectory(sdk)
add_subdirectory(core)
add_subdirectory(extensions/analysis_results_extension)
add_subdirectory(extensions/dummy_3d_extension)
add_subdirectory(extensions/fiff_browser_extension)
add_subdirectory(extensions/psd_analysis_extension)
add_subdirectory(workbench)
add_subdirectory(neuro_kernel)
add_subdirectory(skill_host)
