cmake_minimum_required(VERSION 3.14)
project(scan_fieldline LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED
    COMPONENTS Core Widgets Network Concurrent
)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED
    COMPONENTS Core Widgets Network Concurrent
)

execute_process(
    COMMAND which python
    OUTPUT_VARIABLE Python_EXECUTABLE
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND dirname ${Python_EXECUTABLE}
    OUTPUT_VARIABLE Python_BIN_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND pwd -P
    WORKING_DIRECTORY ${Python_BIN_DIR}/..
    OUTPUT_VARIABLE Python_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND pwd -P
    WORKING_DIRECTORY ${Python_PREFIX}/lib
    OUTPUT_VARIABLE Python_LIB_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import platform; major, minor, patch = platform.python_version_tuple(); print(major)"
    OUTPUT_VARIABLE Python_VERSION_MAJOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import platform; major, minor, patch = platform.python_version_tuple(); print(minor)"
    OUTPUT_VARIABLE Python_VERSION_MINOR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(EXISTS ${Python_BIN_DIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}-config)
    message(STATUS "python config found")
else()
    message(STATUS "python config not found")
    execute_process(
        COMMAND readlink -f ${Python_EXECUTABLE}
        OUTPUT_VARIABLE Python_EXECUTABLE_TRUE_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND dirname ${Python_EXECUTABLE_TRUE_PATH}
        OUTPUT_VARIABLE Python_BIN_TRUE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT EXISTS ${Python_BIN_TRUE_DIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}-config)
        message(FATAL python-config not found. Python installation missing python-config.)
    else()
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${Python_BIN_TRUE_DIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}-config ${Python_BIN_DIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}-config
    )
        message(STATUS "Creating sym link to python config")
    endif()
endif()

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

message(      "python prefix dir: " ${Python_PREFIX})
message(         "python bin dir: " ${Python_BIN_DIR})
message(         "python lib dir: " ${Python_LIB_DIR})
message(      "python executable: " ${Python_EXECUTABLE})
message(   "python version major: " ${Python_VERSION_MAJOR})
message(   "python version minor: " ${Python_VERSION_MINOR})
message(    "python include dirs: " ${Python_INCLUDE_DIRS})
message(          "python cflags: " ${Python_CFLAGS})
message(     "python cflags norm: " ${Python_CFLAGS_NORM})
message(       "python libraries: " ${Python_LIBS})
message(       "python lib flags: " ${Python_LDFLAGS})
message(  "python lib flags norm: " ${Python_LDFLAGS_NORM})

set(SOURCES
    fieldline_global.cpp
    fieldline.cpp
    fieldline_acqsystem.cpp
    fieldline_view.cpp
    # fieldline_view_chassis.cpp
    ipfinder.cpp
    ipfinder_network.cpp
)

set(HEADERS
    fieldline_global.h
    fieldline_definitions.h
    fieldline.h
    fieldline_acqsystem.h
    fieldline_view.h
    # fieldline_view_chassis.h
    ipfinder.h
    ipfinder_network.h
)

set(UI_FILES
    formfiles/fieldline_view.ui
    formfiles/fieldline_chassis.ui
)

set(FILE_TO_UPDATE fieldline_global.cpp)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/fieldline.json)
    # message("File fieldline.json exists.")
else()
    # message("File fieldline.json does not exist.")
    # message("Creating a new fieldline.json file.")
execute_process(COMMAND "echo \"\""
    WORKING_DIRECTORY ${CMAKE_C
    OUTPUT_FILE fieldline.json
)
endif()

set(SOURCE_PATHS ${SOURCES})

list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES
    OBJECT_DEPENDS "${SOURCE_PATHS}"
)

add_link_options(${Python_LDFLAGS})

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI_FILES})

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

# target_include_directories(${PROJECT_NAME} PUBLIC ../ ${Python_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ../)

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
    scDisp
    scShared
    scMeas
    ${FFTW_LIBS}
    python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}
)

separate_arguments(Python_CFLAGS_NORM UNIX_COMMAND "${Python_CFLAGS}")
separate_arguments(Python_LDFLAGS_NORM UNIX_COMMAND "${Python_LDFLAGS}")

target_compile_options(${PROJECT_NAME} PRIVATE ${Python_CFLAGS_NORM} ${Python_INCLUDE_DIRS})

message(          "python cflags2: " ${Python_CFLAGS})
message(     "python cflags norm2: " ${Python_CFLAGS_NORM})
message(       "python lib flags2: " ${Python_LDFLAGS})
message(  "python lib flags norm2: " ${Python_LDFLAGS_NORM})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    SCAN_FIELDLINE_PLUGIN MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}"
    MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}"
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        STATICBUILD QT_STATICPLUGIN
)
endif()

