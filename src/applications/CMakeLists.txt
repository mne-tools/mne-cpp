set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/apps)
if(WIN32)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

if(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
  if(NOT BUILD_MAC_APP_BUNDLE)
    set(CMAKE_MACOSX_RPATH TRUE)
  endif()
else()
    set(CMAKE_INSTALL_RPATH "\${ORIGIN}/../lib")
endif()
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_subdirectory(mne_analyze)
add_subdirectory(mne_scan)
add_subdirectory(mne_anonymize)

add_subdirectory(mne_rt_server)

add_subdirectory(mne_forward_solution)
add_subdirectory(mne_edf2fiff)
add_subdirectory(mne_dipole_fit)

