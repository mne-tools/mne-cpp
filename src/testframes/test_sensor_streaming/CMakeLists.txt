cmake_minimum_required(VERSION 3.14)
project(test_sensor_streaming LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    test_sensor_streaming.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_disp
    mne_fiff
    mne_utils
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen
)

# Include the ex_brain_view source directory for worker headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/examples/ex_brain_view
)

# Add the worker source files directly (they are part of ex_brain_view, not a library)
target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/examples/ex_brain_view/workers/rtsensordataworker.cpp
    ${CMAKE_SOURCE_DIR}/examples/ex_brain_view/workers/rtsensordatacontroller.cpp
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

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
