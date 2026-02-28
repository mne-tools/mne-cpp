cmake_minimum_required(VERSION 3.14)
project(mne_inspect LANGUAGES CXX)

# Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    main.cpp
    app/mainwindow.cpp
)

set(HEADERS
    app/mainwindow.h
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

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_disp3D_rhi
    mne_fs
    mne_fiff
    mne_fwd
    mne_mne
    mne_disp
    mne_inverse
    mne_utils
    mne_connectivity
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen
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
    qt_finalize_executable(${PROJECT_NAME})
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
