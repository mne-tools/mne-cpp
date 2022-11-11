set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mne_scan_plugins)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mne_scan_plugins)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

# Sensor Plugin
add_subdirectory(fiffsimulator)
add_subdirectory(ftbuffer)
add_subdirectory(babymeg)
add_subdirectory(natus)

# Algorithm Plugin
add_subdirectory(rtcmne)
add_subdirectory(averaging)
add_subdirectory(covariance)
add_subdirectory(noisereduction)
add_subdirectory(neuronalconnectivity)
add_subdirectory(writetofile)
add_subdirectory(hpi)
add_subdirectory(rtfwd)

#
# add_subdirectory(brainamp)
# add_subdirectory(eegosports)
# add_subdirectory(gusbamp)
# add_subdirectory(tmsi)
# add_subdirectory(brainflowboard)
# add_subdirectory(lsladapter)
