set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../out/${CMAKE_BUILD_TYPE}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/../out/${CMAKE_BUILD_TYPE}/lib)

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
add_subdirectory(events)

add_subdirectory(mne)
add_subdirectory(fwd)
add_subdirectory(inverse)
add_subdirectory(communication)
add_subdirectory(rtprocessing)
add_subdirectory(connectivity)

add_subdirectory(disp)
add_subdirectory(disp3D)
