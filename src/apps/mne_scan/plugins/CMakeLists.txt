# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

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

if(WIN32)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mne_scan_plugins)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/mne_scan_plugins)
endif()

# Hidden by default as on Windows, so a class that the tests or other modules use without exporting it
# fails to link on every platform, not only with MSVC
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_STATIC_LIBRARY_PREFIX "")

# Plugins live in bin/mne_scan_plugins/, one level deeper than the executables, so their own libraries
# (e.g. libmne_acq_devices, which no executable links) must be found relative to the plugin itself.
if(APPLE)
  list(APPEND CMAKE_INSTALL_RPATH "@loader_path/../../lib")
elseif(UNIX)
  list(APPEND CMAKE_INSTALL_RPATH "\${ORIGIN}/../../lib")
endif()

# Sensor Plugin
add_subdirectory(fiffsimulator)
add_subdirectory(ftbuffer)
add_subdirectory(babymeg)
add_subdirectory(natus)
add_subdirectory(brainflowboard)
add_subdirectory(brainamp)
add_subdirectory(tmsi)
add_subdirectory(gusbamp)
add_subdirectory(eegosports)
add_subdirectory(neuromag)

# Algorithm Plugin
add_subdirectory(rtclustmne)
add_subdirectory(averaging)
add_subdirectory(covariance)
add_subdirectory(noisereduction)
add_subdirectory(neuronalconnectivity)
add_subdirectory(writetofile)
add_subdirectory(hpi)
add_subdirectory(rtfwd)

add_subdirectory(lsladapter)
