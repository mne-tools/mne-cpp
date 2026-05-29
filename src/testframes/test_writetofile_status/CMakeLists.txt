cmake_minimum_required(VERSION 3.14)
project(test_writetofile_status LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Test Gui Widgets Svg)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

if(NOT TARGET scan_writetofile)
    message(STATUS "${PROJECT_NAME}: scan_writetofile not built — skipping")
    return()
endif()

set(SOURCES
    test_writetofile_status.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/applications/mne_scan/plugins
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    scan_writetofile
    scShared
    scMeas
    scDisp
    mne_fiff
    mne_utils
    mne_math
    mne_disp
    Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

# scan_writetofile is a runtime-loaded shared plugin that is built into
# bin/mne_scan_plugins, not next to the test binary nor in the usual lib
# directory. The CI test runner launches the test executable directly
# (not through CTest), so the CTest ENVIRONMENT property below is not
# honoured in that path. Copy the plugin next to the test binary and add
# the binary's own directory to the runtime search path so the loader can
# resolve scan_writetofile regardless of how the test is launched. The
# plugin's own dependencies (scShared, mne_* …) keep resolving through the
# test executable's @executable_path/../lib ($ORIGIN/../lib) entry.
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:scan_writetofile>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
)

if(APPLE)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "@executable_path/../lib;@executable_path")
elseif(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "\$ORIGIN/../lib:\$ORIGIN")
endif()

enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

if(APPLE)
    set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT
        "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:scShared>:$<TARGET_FILE_DIR:scan_writetofile>;QT_QPA_PLATFORM=offscreen")
elseif(UNIX)
    set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT
        "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:scShared>:$<TARGET_FILE_DIR:scan_writetofile>;QT_QPA_PLATFORM=offscreen")
else()
    set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT
        "QT_QPA_PLATFORM=offscreen")
endif()

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
