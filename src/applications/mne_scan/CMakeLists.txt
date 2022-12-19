option(NO_MNE_SCAN_PLUGINS "Skip building MNE Scan plugins." OFF)

add_subdirectory(mne_scan)
add_subdirectory(libs)

if(NOT NO_MNE_SCAN_PLUGINS)
    add_subdirectory(plugins)
endif()
