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

add_subdirectory(anShared)
