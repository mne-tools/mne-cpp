# MNE Analyze Studio application family.

cmake_minimum_required(VERSION 3.15)
project(mne_analyze_studio LANGUAGES CXX)

add_subdirectory(core)
add_subdirectory(workbench)
add_subdirectory(neuro_kernel)
add_subdirectory(skill_host)
