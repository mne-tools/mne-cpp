# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib)

if(WIN32)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/bin)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/bin)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

  message(STATUS CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE
    =${CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE})
  message(STATUS CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG
    =${CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG})
  message(STATUS CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE
    =${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE})
  message(STATUS CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG
    =${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG})
  message(STATUS CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE
    =${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
  message(STATUS CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG
    =${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})
endif()

message(STATUS CMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
message(STATUS CMAKE_ARCHIVE_OUTPUT_DIRECTORY=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
message(STATUS CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

if(APPLE)
  set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
  if(NOT BUILD_MAC_APP_BUNDLE)
    set(CMAKE_MACOSX_RPATH TRUE)
  endif()
else()
  set(CMAKE_INSTALL_RPATH "\${ORIGIN}/")
endif()

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# ── Code-coverage instrumentation ──────────────────────────────────────
# When WITH_CODE_COV is ON the --coverage flag is applied here so that
# library code produces .gcno/.gcda files.  The same flag is also applied
# in src/apps/ and src/tools/ CMakeLists.txt.
if(WITH_CODE_COV)
  add_compile_options("--coverage" "-O1" "-g")
  add_link_options("--coverage")
endif()

add_subdirectory(mne/base)
add_subdirectory(mne/math)
add_subdirectory(mne/scene)
add_subdirectory(mne/channels)
if(NOT WASM) # WebAssembly cannot start processes
  add_subdirectory(mne/pybridge)
endif()
add_subdirectory(mne/report)
add_subdirectory(mne/lsl)

add_subdirectory(mne/fiff)
add_subdirectory(mne/bids)
add_subdirectory(mne/ieeg)
add_subdirectory(mne/fs)
add_subdirectory(mne/mri)

add_subdirectory(mne/mne)
add_subdirectory(mne/fwd)
add_subdirectory(mne/inv)
add_subdirectory(mne/com)
add_subdirectory(mne/connectivity)
add_subdirectory(mne/dsp)
add_subdirectory(mne/acq)
add_subdirectory(mne/acq/fiff)
add_subdirectory(mne/acq/lsl)
add_subdirectory(mne/acq/devices)
if(NOT WASM) # WebAssembly has no serial ports
  add_subdirectory(mne/acq/devices/polhemus)
endif()

add_subdirectory(mne/mna)
add_subdirectory(mne/ml)
add_subdirectory(mne/decoding)
add_subdirectory(mne/sts)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/MneQtFree.cmake)
mne_require_qt_free(${CMAKE_CURRENT_SOURCE_DIR})

# The display libraries are Qt by design and the only libraries that may use it.
if(MNE_BUILD_QT)
  list(APPEND CMAKE_INSTALL_RPATH ${MNE_QT_RPATH})
  add_subdirectory(mne/disp)
  add_subdirectory(mne/disp3D)
  # Qt Widgets only here: plots, viewers and the QRhiWidget hosts on top of the two display cores
  add_subdirectory(mne/disp_widgets)
  add_subdirectory(mne/disp3D_widgets)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/MnePrecompiledHeaders.cmake)
mne_precompile_headers(${CMAKE_CURRENT_SOURCE_DIR})
