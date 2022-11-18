set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/out/${CMAKE_BUILD_TYPE}/apps)
file(COPY ${CMAKE_SOURCE_DIR}/resources DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

add_subdirectory(mne_analyze)
add_subdirectory(mne_scan)
add_subdirectory(mne_anonymize)

add_subdirectory(mne_rt_server)

add_subdirectory(mne_forward_solution)
add_subdirectory(mne_edf2fiff)
add_subdirectory(mne_dipole_fit)
