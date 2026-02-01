cmake_minimum_required(VERSION 3.10)
project(ex_brain_view)

# Find Qt
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui GuiPrivate Widgets ShaderTools)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

# Define the source files
set(SOURCES
    main.cpp
    brainview.cpp
    brainrenderer.cpp
    brainsurface.cpp
    sourceestimateoverlay.cpp
    surfaceinterpolation.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_fs
    mne_fiff
    mne_mne
    mne_disp
    mne_utils
)

# Add shaders (Interface library trick)
add_library(brain_shaders INTERFACE)
qt_add_shaders(${PROJECT_NAME} "brain_shaders"
    PREFIX
        "/"
    BASE
        "shaders"
    FILES
        "shaders/standard.vert"
        "shaders/standard.frag"
        "shaders/holographic.vert"
        "shaders/holographic.frag"
        "shaders/glossy.vert"
        "shaders/glossy.frag"
)

# Link against MNE-CPP libraries and Qt
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER mne-cpp.org
    MACOSX_BUNDLE ${BUILD_MAC_APP_BUNDLE}
    WIN32_EXECUTABLE TRUE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

