set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mne_scan_plugins)
if(WIN32)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
endif()
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mne_scan_plugins)
if(WIN32)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
endif()

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

# Sensor Plugin
add_subdirectory(fiffsimulator)
add_subdirectory(ftbuffer)
add_subdirectory(babymeg)
add_subdirectory(natus)
add_subdirectory(randomdata)

# Algorithm Plugin
add_subdirectory(rtcmne)
add_subdirectory(averaging)
add_subdirectory(covariance)
add_subdirectory(noisereduction)
add_subdirectory(neuronalconnectivity)
add_subdirectory(writetofile)
add_subdirectory(hpi)
add_subdirectory(rtfwd)

option(WITH_BRAINFLOW   "Build brainflow plugin"    OFF)
option(WITH_LSL         "Build LSL plugin"          OFF)
option(WITH_BRAINAMP    "Build BrainAMP pluign"     OFF)
option(WITH_EEGO        "Build Eego Sport plugin"   OFF)
option(WITH_GUSBAMP     "Build GUSBAmp plugin"      OFF)
option(WITH_TMSI        "Build TMSI plugin"         OFF)

if(WITH_BRAINAMP)
    add_subdirectory(brainamp)
endif()

if(WITH_EEGO)
    add_subdirectory(eegosports)
endif()

if(WITH_GUSBAMP)
    add_subdirectory(gusbamp)
endif()

if(WITH_TMSI)
    add_subdirectory(tmsi)
endif()

if(WITH_BRAINFLOW)
    add_subdirectory(brainflowboard)
endif()

if(WITH_LSL)
    add_subdirectory(lsladapter)
endif()
