cmake_minimum_required(VERSION 3.14)
project(mne_inspect LANGUAGES CXX)

# Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_disp3D
    mne_fs
    mne_fiff
    mne_fwd
    mne_mne
    mne_mna
    mne_mri
    mne_disp
    mne_inv
    mne_utils
    mne_math
    mne_conn
)

# Standalone static-library plugins (see app/plugins/README.md).
add_subdirectory(app/plugins)

set(INSPECT_PLUGIN_LIBS "")
if(TARGET inspect_electrodes)
    list(APPEND INSPECT_PLUGIN_LIBS inspect_electrodes)
endif()
if(TARGET inspect_mri_slices)
    list(APPEND INSPECT_PLUGIN_LIBS inspect_mri_slices)
endif()

#---------------------------------------------------------------------------
# mne_inspect_app_core: the full MainWindow + PickReadoutModel as a static
# library so the executable, the test_mne_inspect_multimodal frame, and the
# mne_doc_shots screenshot tool can all link against the live widget tree
# without rebuilding the sources three times.
#---------------------------------------------------------------------------
set(MNE_INSPECT_CORE_SOURCES
    app/mainwindow.cpp
    app/pickreadoutmodel.cpp
)

set(MNE_INSPECT_CORE_HEADERS
    app/mainwindow.h
    app/pickreadoutmodel.h
)

add_library(mne_inspect_app_core STATIC
    ${MNE_INSPECT_CORE_SOURCES}
    ${MNE_INSPECT_CORE_HEADERS}
)

target_include_directories(mne_inspect_app_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/app
)

target_link_libraries(mne_inspect_app_core PUBLIC
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    ${INSPECT_PLUGIN_LIBS}
    Eigen3::Eigen
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(mne_inspect_app_core PUBLIC STATICBUILD)
endif()

set_target_properties(mne_inspect_app_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

#---------------------------------------------------------------------------
# mne_inspect_demo_fixtures: synthetic ElectrodeArray/MriVolData payloads
# shared between the screenshot tool (mne_doc_shots) and the multimodal
# test frame so visual docs and tests render the exact same scene.
#---------------------------------------------------------------------------
add_library(mne_inspect_demo_fixtures STATIC
    fixtures/inspect_demo_fixtures.h
    fixtures/inspect_demo_fixtures.cpp
)

target_include_directories(mne_inspect_demo_fixtures PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mne_inspect_demo_fixtures PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    mne_disp3D
    mne_mri
    mne_fiff
    Eigen3::Eigen
)

set_target_properties(mne_inspect_demo_fixtures PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(mne_inspect_demo_fixtures PUBLIC STATICBUILD)
endif()

#---------------------------------------------------------------------------
# mne_inspect: the application bundle. Only owns main.cpp + bundle
# resources; all UI logic comes from mne_inspect_app_core.
#---------------------------------------------------------------------------
set(SOURCES
    main.cpp
)

set(HEADERS
)

set(RESOURCES
    resources/images/appIcons/mne_inspect.icns
)

set_source_files_properties(resources/images/appIcons/mne_inspect.icns PROPERTIES
    MACOSX_PACKAGE_LOCATION Resources)

if(APPLE)
    set(RESOURCE_GROUPS
        selectionGroups
        2DLayouts
        default_filters
        hpiAlignment
        sensorSurfaces
        coilDefinitions
        3DLayouts
    )

    foreach(GROUP ${RESOURCE_GROUPS})
        file(GLOB GROUP_FILES "${PROJECT_RESOURCES_DIR}/general/${GROUP}/*")
        set_source_files_properties(${GROUP_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "MacOS/../resources/general/${GROUP}")
        list(APPEND SOURCES ${GROUP_FILES})
    endforeach()
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS} ${RESOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RESOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    .
    app
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    mne_inspect_app_core
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.mne-cpp.mne_inspect
    MACOSX_BUNDLE ${BUILD_MAC_APP_BUNDLE}
    MACOSX_BUNDLE_ICON_FILE mne_inspect.icns
    WIN32_EXECUTABLE TRUE
)

# Install
install(
    TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications
)

if(QT_VERSION_MAJOR EQUAL 6)
    # WASM: increase pthread pool to avoid deadlocks when QtConcurrent or
    # QThread spawn additional threads (e.g. SCDC geodesic distance computation).
    # Qt default is 4, which is too small for our background loading pipeline.
    # Must be set BEFORE qt_finalize_executable() which reads this property.
    if(EMSCRIPTEN)
        set_target_properties(${PROJECT_NAME} PROPERTIES QT_WASM_PTHREAD_POOL_SIZE 8)

        # Embed sensor surface files into the Emscripten virtual filesystem.
        # QCoreApplication::applicationDirPath() returns "/" on WASM, so
        # the code resolves "/../resources/general/sensorSurfaces/306m.fif"
        # to "/resources/general/sensorSurfaces/306m.fif".
        target_link_options(${PROJECT_NAME} PRIVATE
            "SHELL:--preload-file ${PROJECT_RESOURCES_DIR}/general/sensorSurfaces@/resources/general/sensorSurfaces"
            "SHELL:--preload-file ${PROJECT_RESOURCES_DIR}/general/coilDefinitions@/resources/general/coilDefinitions"
        )
    endif()
    qt_finalize_executable(${PROJECT_NAME})
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
