set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib)

if(WIN32)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/apps)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/apps)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/apps)
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

add_subdirectory(utils)

add_subdirectory(fiff)
add_subdirectory(fs)

add_subdirectory(mne)
add_subdirectory(fwd)
add_subdirectory(inverse)
add_subdirectory(communication)
add_subdirectory(rtprocessing)
add_subdirectory(connectivity)

add_subdirectory(events)
add_subdirectory(disp)

if(NOT WASM)
  add_subdirectory(disp3D)
endif()
