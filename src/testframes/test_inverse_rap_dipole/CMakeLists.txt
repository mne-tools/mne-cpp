cmake_minimum_required(VERSION 3.14)
project(test_inverse_rap_dipole LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets Concurrent Network Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES test_inverse_rap_dipole.cpp)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED
    mne_dsp
    mne_conn
    mne_inv
    mne_fwd
    mne_mne
    mne_fiff
    mne_fs
    mne_utils
    mne_math
    mne_events
    mne_disp
    mne_disp3D
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${QT_REQUIRED_COMPONENT_LIBS} ${MNE_LIBS_REQUIRED} eigen)

set_target_properties(${PROJECT_NAME} PROPERTIES
  WIN32_EXECUTABLE FALSE
  MACOSX_BUNDLE FALSE
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()


# Register with CTest
enable_testing()
add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_tests_properties(${PROJECT_NAME} PROPERTIES TIMEOUT 300)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
