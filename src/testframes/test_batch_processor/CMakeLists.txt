cmake_minimum_required(VERSION 3.14)
project(test_batch_processor LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

# Code-coverage instrumentation for Pattern 2 test
if(WITH_CODE_COV)
  add_compile_options("--coverage" "-O1" "-g")
  add_link_options("--coverage")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)

set(SOURCES
    test_batch_processor.cpp
    ../../tools/preprocessing/mne_process_raw/batchprocessor.cpp
    ../../tools/preprocessing/mne_process_raw/batchprocessor.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    mne_mne
    mne_fiff
    mne_utils
    mne_math
    mne_events
    eigen
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
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
