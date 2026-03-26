# SDK interfaces and shared registries for MNE Analyze Studio extensions.

cmake_minimum_required(VERSION 3.15)
project(mne_analyze_studio_sdk LANGUAGES CXX)

set(QT_REQUIRED_COMPONENTS Core)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

add_library(${PROJECT_NAME} SHARED
    extensionviewfactoryregistry.cpp
    extensionviewfactoryregistry.h
    iextensionviewfactory.h
    iresultrendererfactory.h
    iresultrendererwidget.h
    irawdataview.h
    resultrendererfactoryregistry.cpp
    resultrendererfactoryregistry.h
)

mne_analyze_studio_configure_library_target(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
)
