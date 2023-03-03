cmake_minimum_required(VERSION 3.14)
project(scan_fieldline LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED
  COMPONENTS Core Widgets Network Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED
  COMPONENTS Core Widgets Network Concurrent)
 find_package(Python REQUIRED COMPONENTS Interpreter Development)
 cmake_path(GET Python_EXECUTABLE PARENT_PATH Python_BIN_DIR)

 set(PYTHON_COMPILE_FLAGS "--cflags")
 execute_process(COMMAND
   ${Python_BIN_DIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}-config ${PYTHON_COMPILE_FLAGS}
   OUTPUT_VARIABLE Python_FLAGS
 )

set(SOURCES
    fieldline_global.cpp
    fieldline.cpp
    fieldline_acq_system_controller.cpp
    fieldline_plugin_gui.cpp
)

set(HEADERS
    fieldline_global.h
    fieldline.h
    fieldline_acq_system_controller.h
    fieldline_plugin_gui.h
)

set(FILE_TO_UPDATE fieldline_global.cpp)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/fieldline.json)
  # message("File fieldline.json exists.")
else()
  # message("File fieldline.json does not exist.")
  # message("Creating a new fieldline.json file.")
  execute_process(COMMAND "echo \"\""
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_FILE fieldline.json)
endif()

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES
  OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set(FFTW_LIBS "")

if(USE_FFTW)
  if (WIN32)
    set(FFTW_LIBS
      ${FFTW_DIR_LIBS}/libfftw3-3.dll
      ${FFTW_DIR_LIBS}/libfftw3f-3.dll
      ${FFTW_DIR_LIBS}/libfftwf3l-3.dll
    )
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_DIR_INCLUDE})
  elseif(UNIX AND NOT APPLE)
    set(FFTW_LIBS ${FFTW_DIR_LIBS}/lib/libfftw3.so)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_DIR_INCLUDE}/api)
  endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Concurrent
    eigen
    mne_disp
    mne_utils
    mne_fiff
    # mne_fs
    # mne_mne
    # mne_fwd
    # mne_inverse
    # mne_rtprocessing
    # mne_connectivity
    # mne_events
    scDisp
    scShared
    scMeas
    ${FFTW_LIBS}
    python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}
  )

 target_link_directories(${PROJECT_NAME} PRIVATE ${Python_LIBRARY_DIRS})
 separate_arguments(Python_FLAGS_NORM UNIX_COMMAND "${Python_FLAGS}")
 target_compile_options(${PROJECT_NAME} PRIVATE ${Python_FLAGS_NORM})

target_compile_definitions(${PROJECT_NAME} PRIVATE
  SCAN_FIELDLINE_PLUGIN MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}"
  MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}"
)

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    STATICBUILD QT_STATICPLUGIN)
endif()
