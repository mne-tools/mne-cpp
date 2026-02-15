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
    app/mainwindow.cpp
    core/dataloader.cpp
    core/viewstate.cpp
    geometry/meshfactory.cpp
    input/cameracontroller.cpp
    input/raypicker.cpp
    view/brainview.cpp
    view/brainrenderer.cpp
    view/multiviewlayout.cpp
    scene/sensorfieldmapper.cpp
    renderable/brainsurface.cpp
    renderable/sourceestimateoverlay.cpp
    renderable/dipoleobject.cpp
    renderable/networkobject.cpp
    workers/stcloadingworker.cpp
    workers/rtsourcedataworker.cpp
    workers/rtsourcedatacontroller.cpp
    model/items/abstracttreeitem.cpp
    model/items/surfacetreeitem.cpp
    model/items/bemtreeitem.cpp
    model/braintreemodel.cpp
    model/items/sensortreeitem.cpp
    model/items/dipoletreeitem.cpp
    model/items/sourcespacetreeitem.cpp
    model/items/digitizertreeitem.cpp
    model/items/digitizersettreeitem.cpp
    model/items/networktreeitem.cpp
    helpers/geometryinfo.cpp
    helpers/interpolation.cpp
    helpers/field_map.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    .
    app
    core
    geometry
    input
    view
    scene
    renderable
    workers
    model
    model/items
    helpers
)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_fs
    mne_fiff
    mne_fwd
    mne_mne
    mne_disp
    mne_inverse
    mne_utils
    mne_connectivity
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
        "shaders/anatomical.vert"
        "shaders/anatomical.frag"
        "shaders/dipole.vert"
        "shaders/dipole.frag"
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

