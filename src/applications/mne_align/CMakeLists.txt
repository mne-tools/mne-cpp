cmake_minimum_required(VERSION 3.14)
project(mne_align LANGUAGES CXX)

# Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Widgets SerialPort)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_utils
    mne_math
    mne_fiff
    mne_fs
    mne_mne
    mne_disp
    mne_disp3D
)

#---------------------------------------------------------------------------
# mne_align_app_core: the wizard + 3-D view + main window as a STATIC
# library so the executable, the mne_doc_shots screenshot tool, and any
# future test frame can all link against the live widget tree without
# rebuilding the sources multiple times. Mirrors the mne_inspect_app_core
# pattern.
#---------------------------------------------------------------------------
set(MNE_ALIGN_CORE_SOURCES
    mne_align.cpp
    align_wizard.cpp
    align_3d_view.cpp
)

set(MNE_ALIGN_CORE_HEADERS
    mne_align.h
    align_wizard.h
    align_3d_view.h
    acquired_points.h
    polhemus_connection.h
)

add_library(mne_align_app_core STATIC
    ${MNE_ALIGN_CORE_SOURCES}
    ${MNE_ALIGN_CORE_HEADERS}
)

target_include_directories(mne_align_app_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mne_align_app_core PUBLIC
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
)

set_target_properties(mne_align_app_core PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(mne_align_app_core PUBLIC STATICBUILD)
endif()

#---------------------------------------------------------------------------
# mne_align_demo_fixtures: synthetic AcquiredPoints payloads shared
# between the screenshot tool (mne_doc_shots) and any future test frame so
# visual docs and tests render the exact same digitisation session.
#---------------------------------------------------------------------------
add_library(mne_align_demo_fixtures STATIC
    fixtures/align_demo_fixtures.h
    fixtures/align_demo_fixtures.cpp
)

target_include_directories(mne_align_demo_fixtures PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(mne_align_demo_fixtures PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    mne_utils
    mne_fiff
    Eigen3::Eigen
)

set_target_properties(mne_align_demo_fixtures PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(mne_align_demo_fixtures PUBLIC STATICBUILD)
endif()

#---------------------------------------------------------------------------
# mne_align: the application bundle. Only owns main.cpp; all UI logic
# comes from mne_align_app_core.
#---------------------------------------------------------------------------
set(SOURCES
    main.cpp
)

set(HEADERS
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE .)

target_link_libraries(${PROJECT_NAME} PRIVATE
    mne_align_app_core
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER org.mne-cpp.mne_align
    MACOSX_BUNDLE ${BUILD_MAC_APP_BUNDLE}
    WIN32_EXECUTABLE TRUE
)

install(
    TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
