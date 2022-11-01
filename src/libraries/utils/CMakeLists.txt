cmake_minimum_required(VERSION 3.14)
project(mne_utils LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent)

set(SOURCES
    file.cpp
    kmeans.cpp
    mnemath.cpp
    ioutils.cpp
    layoutloader.cpp
    layoutmaker.cpp
    selectionio.cpp
    spectrogram.cpp
    utils_global.cpp
    warp.cpp
    sphere.cpp
    generics/observerpattern.cpp
    generics/applicationlogger.cpp
    spectral.cpp
    mnetracer.cpp
)

set(FILE_TO_UPDATE utils_global.cpp)
set(FILES ${SOURCES})
list(REMOVE_ITEM FILES ${FILE_TO_UPDATE})

add_library(${PROJECT_NAME} ${SOURCES})

add_custom_target(buildinfo
                  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/global_info)

add_custom_command(OUTPUT global_info
                   DEPENDS ${FILES}
                   COMMAND ${CMAKE_COMMAND} -E touch_nocreate ${CMAKE_CURRENT_SOURCE_DIR}/${FILE_TO_UPDATE})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

add_dependencies(${PROJECT_NAME} buildinfo)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    eigen
)

target_compile_definitions(${PROJECT_NAME} PRIVATE utils_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")
