cmake_minimum_required(VERSION 3.14)
project(test_mri_nifti_io LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

# System zlib (re-used for synthesising a .nii.gz fixture inside the test).
find_package(ZLIB QUIET)
if(NOT ZLIB_FOUND)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ZlibPrivate)
    set(ZLIB_TARGET Qt${QT_VERSION_MAJOR}::ZlibPrivate)
else()
    set(ZLIB_TARGET ZLIB::ZLIB)
endif()

set(SOURCES test_mri_nifti_io.cpp)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_mri
    mne_fiff
    mne_utils
    mne_math
    Eigen3::Eigen
    ${ZLIB_TARGET}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER mne-cpp.org
    MACOSX_BUNDLE OFF
    WIN32_EXECUTABLE FALSE
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_tests_properties(${PROJECT_NAME} PROPERTIES
    ENVIRONMENT "DYLD_LIBRARY_PATH=${BINARY_OUTPUT_DIRECTORY}/lib"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
