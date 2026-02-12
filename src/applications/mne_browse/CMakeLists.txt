cmake_minimum_required(VERSION 3.14)
project(mne_browse LANGUAGES CXX)

# Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets Concurrent Network Svg OpenGL OpenGLWidgets)

if(NOT WASM)
  list(APPEND QT_REQUIRED_COMPONENTS PrintSupport)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    main.cpp
    Utils/datamarker.cpp
    Utils/rawsettings.cpp
    Utils/filteroperator.cpp
    Utils/filterplotscene.cpp
    Utils/butterflyscene.cpp
    Utils/butterflysceneitem.cpp
    Models/averagemodel.cpp
    Models/rawmodel.cpp
    Models/eventmodel.cpp
    Delegates/averagedelegate.cpp
    Delegates/rawdelegate.cpp
    Delegates/eventdelegate.cpp
    Windows/mainwindow.cpp
    Windows/filterwindow.cpp
    Windows/eventwindow.cpp
    Windows/datawindow.cpp
    Windows/aboutwindow.cpp
    Windows/informationwindow.cpp
    Windows/averagewindow.cpp
    Windows/scalewindow.cpp
    Windows/chinfowindow.cpp
    Utils/datapackage.cpp
    Windows/noisereductionwindow.cpp
)

set(HEADERS
    Utils/datamarker.h
    Utils/rawsettings.h
    Utils/filteroperator.h
    Utils/types.h
    Utils/info.h
    Utils/filterplotscene.h
    Utils/butterflyscene.h
    Utils/butterflysceneitem.h
    Models/averagemodel.h
    Models/rawmodel.h
    Models/eventmodel.h
    Delegates/averagedelegate.h
    Delegates/rawdelegate.h
    Delegates/eventdelegate.h
    Windows/mainwindow.h
    Windows/filterwindow.h
    Windows/eventwindow.h
    Windows/datawindow.h
    Windows/aboutwindow.h
    Windows/informationwindow.h
    Windows/averagewindow.h
    Windows/scalewindow.h
    Windows/chinfowindow.h
    Windows/noisereductionwindow.h
    Utils/datapackage.h
)

set(FORMS
    Windows/eventwindowdock.ui
    Windows/datawindowdock.ui
    Windows/mainwindow.ui
    Windows/aboutwindow.ui
    Windows/informationwindow.ui
    Windows/averagewindow.ui
    Windows/scalewindow.ui
    Windows/chinfowindow.ui
    Windows/filterwindowdock.ui
    Windows/noisereductionwindow.ui
)

set(RESOURCES
    mne_browse.qrc
    Resources/Images/ApplicationIcons/mne_browse.icns
)

if(APPLE)
    set(RESOURCE_GROUPS
        selectionGroups
        2DLayouts
        default_filters
        hpiAlignment
        sensorSurfaces
        3DLayouts
    )

    foreach(GROUP ${RESOURCE_GROUPS})
        file(GLOB GROUP_FILES "${CMAKE_SOURCE_DIR}/resources/general/${GROUP}/*")
        set_source_files_properties(${GROUP_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION "MacOS/../resources/general/${GROUP}")
        list(APPEND SOURCES ${GROUP_FILES})
    endforeach()
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES})
else()
  add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${FORMS} ${RESOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_utils
    mne_fs
    mne_fiff
    mne_mne
    mne_fwd
    mne_inverse
    mne_disp
    mne_rtprocessing
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER org.mne-cpp.mne_browse
  MACOSX_BUNDLE TRUE
  MACOSX_BUNDLE_ICON_FILE mne_browse.icns
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
