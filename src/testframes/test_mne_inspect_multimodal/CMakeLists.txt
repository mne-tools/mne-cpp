cmake_minimum_required(VERSION 3.14)
project(test_mne_inspect_multimodal LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Test Gui)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

if(NOT TARGET inspect_electrodes OR NOT TARGET inspect_mri_slices OR NOT TARGET mne_disp3D OR NOT TARGET mne_inspect_app_core)
    message(STATUS "test_mne_inspect_multimodal: required targets not built — skipping")
    return()
endif()

# The PickReadoutModel lives inside mne_inspect_app_core; the test links
# against that static library so we exercise the same translation units
# as the shipping mne_inspect executable.
set(SOURCES
    test_mne_inspect_multimodal.cpp
)

set(HEADERS
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS})
else()
    add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/applications/mne_inspect/app
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_inspect_app_core
    mne_inspect_demo_fixtures
    inspect_electrodes
    inspect_mri_slices
    mne_disp3D
    mne_fiff
    mne_mri
    mne_utils
    mne_math
    Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_tests_properties(${PROJECT_NAME} PROPERTIES
    ENVIRONMENT "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:mne_disp3D>:$<TARGET_FILE_DIR:mne_inspect>"
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
