cmake_minimum_required(VERSION 3.14)
project(inspect_mri_slices LANGUAGES CXX)

# Handle Qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

# Pulls in DISP3D's SliceObject and MultimodalScene; skip on builds without
# disp3D (e.g. WASM without QRhi).
if(NOT TARGET mne_disp3D)
    message(STATUS "inspect_mri_slices: mne_disp3D not built — skipping")
    return()
endif()

set(SOURCES
    mri_slices.cpp
)

set(HEADERS
    mri_slices.h
)

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_disp3D
    mne_mri
    mne_utils
    mne_math
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
