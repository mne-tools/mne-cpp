cmake_minimum_required(VERSION 3.14)
project(test_studio_neuro_kernel LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(WITH_CODE_COV)
    add_compile_options("--coverage" "-O1" "-g")
endif()

set(QT_REQUIRED_COMPONENTS Core Network Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    test_studio_neuro_kernel.cpp
    ../../applications/mne_analyze_studio/neuro_kernel/neurokernelservice.cpp
    ../../applications/mne_analyze_studio/neuro_kernel/neurokernelservice.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ../../applications/mne_analyze_studio/neuro_kernel
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Test
    mne_analyze_studio_core
    mne_dsp
    mne_mne
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()