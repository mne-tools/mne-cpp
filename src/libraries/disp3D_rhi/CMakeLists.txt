cmake_minimum_required(VERSION 3.14)
project(mne_disp3D_rhi LANGUAGES CXX)

# Handle Qt uic, moc, rcc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui GuiPrivate Widgets ShaderTools)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    disp3D_rhi_global.cpp
    core/dataloader.cpp
    core/viewstate.cpp
    geometry/meshfactory.cpp
    helpers/geometryinfo.cpp
    helpers/interpolation.cpp
    helpers/field_map.cpp
    input/cameracontroller.cpp
    input/raypicker.cpp
    model/braintreemodel.cpp
    model/items/abstracttreeitem.cpp
    model/items/surfacetreeitem.cpp
    model/items/bemtreeitem.cpp
    model/items/sensortreeitem.cpp
    model/items/dipoletreeitem.cpp
    model/items/sourcespacetreeitem.cpp
    model/items/digitizertreeitem.cpp
    model/items/digitizersettreeitem.cpp
    model/items/networktreeitem.cpp
    renderable/brainsurface.cpp
    renderable/sourceestimateoverlay.cpp
    renderable/dipoleobject.cpp
    renderable/networkobject.cpp
    scene/sensorfieldmapper.cpp
    view/brainview.cpp
    view/brainrenderer.cpp
    view/multiviewlayout.cpp
    workers/stcloadingworker.cpp
    workers/rtsourcedataworker.cpp
    workers/rtsourcedatacontroller.cpp
    workers/rtsensordataworker.cpp
    workers/rtsensordatacontroller.cpp
)

set(HEADERS
    disp3D_rhi_global.h
    core/surfacekeys.h
    core/dataloader.h
    core/viewstate.h
    geometry/meshfactory.h
    helpers/geometryinfo.h
    helpers/interpolation.h
    helpers/field_map.h
    input/cameracontroller.h
    input/raypicker.h
    model/braintreemodel.h
    model/items/abstracttreeitem.h
    model/items/surfacetreeitem.h
    model/items/bemtreeitem.h
    model/items/sensortreeitem.h
    model/items/dipoletreeitem.h
    model/items/sourcespacetreeitem.h
    model/items/digitizertreeitem.h
    model/items/digitizersettreeitem.h
    model/items/networktreeitem.h
    renderable/brainsurface.h
    renderable/sourceestimateoverlay.h
    renderable/dipoleobject.h
    renderable/networkobject.h
    scene/sensorfieldmapper.h
    view/brainview.h
    view/brainrenderer.h
    view/multiviewlayout.h
    workers/stcloadingworker.h
    workers/rtsourcedataworker.h
    workers/rtsourcedatacontroller.h
    workers/rtsensordataworker.h
    workers/rtsensordatacontroller.h
)

# Build-info stamp
set(FILE_TO_UPDATE disp3D_rhi_global.cpp)
set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Compile shaders into the library
qt_add_shaders(${PROJECT_NAME} "disp3d_rhi_shaders"
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

target_include_directories(${PROJECT_NAME}
    PUBLIC ../
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
    mne_fwd
    mne_inverse
    mne_disp
    mne_connectivity
)

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

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE
    MNE_DISP3D_RHI_LIBRARY
    MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}"
    MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

# Install public headers preserving subdirectory structure
foreach(_header ${HEADERS})
    get_filename_component(_dir "${_header}" DIRECTORY)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/${PROJECT_NAME}/${_dir}
        COMPONENT sdk
    )
endforeach()
