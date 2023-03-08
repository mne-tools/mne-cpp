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
#cmake_path(GET Python_EXECUTABLE PARENT_PATH Python_BIN_DIR)

#if(AWK MATCHES ".+-NOTFOUND")
#    message(FATAL_ERROR "FATAL: awk (and mawk and gawk) could not be found (${AWK}).")
#else()
#    execute_process(
#        COMMAND /bin/sh -c "\"${AWK}\" '$1 ~ /^#define/ && $2 ~ /^LIB_(MAJVER|MINVER|PATCH|BUILD)$/\ { print \"set(\${PRJNAME}_\" $2 \" \" $3 \")\" }' \"${CMAKE_SOURCE_DIR}/version.h\""
#        RESULT_VARIABLE AWK_EXITCODE
#        OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/version.cmake"
#        )
#    message(STATUS "Exit code from awk: ${AWK_EXITCODE}")
#    include(${CMAKE_CURRENT_BINARY_DIR}/version.cmake)
#endif()

#set(Python_EXECUTABLE /autofs/cluster/fusion/juan/anaconda3/bin/python)
#cmake_path(GET Python_EXECUTABLE PARENT_PATH Python_BIN_DIR)
#if (WIN32)
#  execute_process(COMMAND CMD /c echo bar OUTPUT_VARIABLE FOO)
#else()
#  message("holaaaaaaa")
#  execute_process(
#    COMMAND "python -c 'import platform; major, minor, patch = platform.python_version_tuple(); print(minor)'"
#    OUTPUT_VARIABLE Python_version)
#endif (WIN32)
#message("Python executable: " ${Python_EXECUTABLE})
#message("Python_version: " ${Python_version})
#
#    execute_process(
#        COMMAND /bin/sh -c "\"${AWK}\" '$1 ~ /^#define/ && $2 ~ /^LIB_(MAJVER|MINVER|PATCH|BUILD)$/\ { print \"set(\${PRJNAME}_\" $2 \" \" $3 \")\" }' \"${CMAKE_SOURCE_DIR}/version.h\""
#        RESULT_VARIABLE AWK_EXITCODE
#        OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/version.cmake"
#        )
#

set(ANACONDA_DIR /autofs/cluster/fusion/juan/anaconda3)
set(Python_BIN_DIR ${ANACONDA_DIR}/bin)
set(Python_LIB_DIR ${ANACONDA_DIR}/lib)
set(Python_EXECUTABLE ${Python_BIN_DIR}/python)

#execute_process(
#  COMMAND "echo $(dirname $(which ${ANACONDA_DIR}/bin/python))"
#  WORKING_DIRECTORY .
#  OUTPUT_VARIABLE Python_BIN_DIR
#)

execute_process(
  COMMAND ${Python_BIN_DIR}/python -c "import platform; major, minor, patch = platform.python_version_tuple(); print(major)"
  OUTPUT_VARIABLE Python_VERSION_MAJOR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${Python_EXECUTABLE} -c "import platform; major, minor, patch = platform.python_version_tuple(); print(minor)"
  OUTPUT_VARIABLE Python_VERSION_MINOR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
  COMMAND ${Python_BIN_DIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}-config --includes
  OUTPUT_VARIABLE Python_INCLUDE_DIRS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${Python_BIN_DIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}-config --cflags
  OUTPUT_VARIABLE Python_CFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

separate_arguments(Python_CFLAGS_NORM UNIX_COMMAND "${Python_CFLAGS}")

execute_process(
  COMMAND ${Python_BIN_DIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}-config --libs
  OUTPUT_VARIABLE Python_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
  COMMAND ${Python_BIN_DIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}-config --ldflags
  OUTPUT_VARIABLE Python_LDFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

separate_arguments(Python_LDFLAGS_NORM UNIX_COMMAND "${Python_LDFLAGS}")

message(       "python bin dir: " ${Python_BIN_DIR})
message(       "python lib dir: " ${Python_LIB_DIR})
message(    "python executable: " ${Python_EXECUTABLE})
message( "python version major: " ${Python_VERSION_MAJOR})
message( "python version minor: " ${Python_VERSION_MINOR})
message(  "python include dirs: " ${Python_INCLUDE_DIRS})
message(        "python cflags: " ${Python_CFLAGS})
message(   "python cflags norm: " ${Python_CFLAGS_NORM})
message(     "python libraries: " ${Python_LIBS})
message(     "python lib flags: " ${Python_LDFLAGS})
message("python lib flags norm: " ${Python_LDFLAGS_NORM})

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
    WORKING_DIRECTORY ${CMAKE_C
    OUTPUT_FILE fieldline.json)
endif()

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES
  OBJECT_DEPENDS "${SOURCE_PATHS}"
)

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

target_include_directories(${PROJECT_NAME} PUBLIC ../ ${Python_INCLUDE_DIRS})

target_link_directories(${PROJECT_NAME} PRIVATE ${Python_LIB_DIR})

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

separate_arguments(Python_CFLAGS_NORM UNIX_COMMAND "${Python_CFLAGS}")
target_compile_options(${PROJECT_NAME} PRIVATE ${Python_CFLAGS_NORM})
add_link_options(${Python_LDFLAGS_NORM})

target_compile_definitions(${PROJECT_NAME} PRIVATE
  SCAN_FIELDLINE_PLUGIN MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}"
  MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}"
)


if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE
    STATICBUILD QT_STATICPLUGIN)
endif()

