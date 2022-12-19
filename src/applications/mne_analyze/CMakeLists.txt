option(NO_MNE_ANALYZE_PLUGINS "Skip building MNE Analyze plugins." OFF)

add_subdirectory(mne_analyze)
add_subdirectory(libs)

if(NOT NO_MNE_ANALYZE_PLUGINS)
    add_subdirectory(plugins)
endif()
