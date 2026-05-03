# MNA-aware workflow skills extension for MNE Analyze Studio.
# Implements TASK 5 of the v2.3.0 release plan: read / write / execute
# MNA projects from inside the Studio workflow graph.

cmake_minimum_required(VERSION 3.15)
project(mne_analyze_studio_mna_skills LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# The MNA library is optional in some build profiles; skip the extension
# when it isn't present so studio still builds.
if(NOT TARGET mne_mna)
    message(STATUS "mne_analyze_studio_mna_skills: mne_mna not built — skipping")
    return()
endif()

set(QT_REQUIRED_COMPONENTS Core)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

add_library(mne_analyze_studio_mna_skills ${MNE_ANALYZE_STUDIO_LIB_TYPE}
    readmnaskill.cpp
    readmnaskill.h
    writemnaskill.cpp
    writemnaskill.h
    runmnagraphskill.cpp
    runmnagraphskill.h
)

mne_analyze_studio_configure_library_target(mne_analyze_studio_mna_skills)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_include_directories(mne_analyze_studio_mna_skills PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mne_analyze_studio_mna_skills PUBLIC
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_analyze_studio_core
    mne_mna
    mne_utils
)
