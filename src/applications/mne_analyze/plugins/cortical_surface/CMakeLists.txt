cmake_minimum_required(VERSION 3.14)
project(analyze_cortical_surface LANGUAGES CXX)

# Handle Qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

# This plugin pulls in DISP3D's MultimodalScene; skip on builds without disp3D.
if(NOT TARGET mne_disp3D)
    message(STATUS "analyze_cortical_surface: mne_disp3D not built — skipping")
    return()
endif()

set(SOURCES
    cortical_surface.cpp
    cortical_surface_global.cpp
)

set(HEADERS
    cortical_surface_global.h
    cortical_surface.h
)

set(FILE_TO_UPDATE cortical_surface_global.cpp)
set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_utils
    mne_math
    mne_fiff
    mne_fs
    mne_disp3D
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
    anShared
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    ANALYZE_CORTICAL_SURFACE_PLUGIN
    MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}"
    MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD QT_STATICPLUGIN)
endif()
