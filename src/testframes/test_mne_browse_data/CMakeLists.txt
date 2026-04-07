cmake_minimum_required(VERSION 3.14)
project(test_mne_browse_data LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)

# Code-coverage instrumentation for Pattern 2 test
if(WITH_CODE_COV)
  add_compile_options("--coverage" "-O1" "-g")
  add_link_options("--coverage")
endif()
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Widgets Concurrent Network Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    test_mne_browse_data.cpp

    # Models (non-GUI)
    ../../applications/mne_browse/Models/rawmodel.cpp
    ../../applications/mne_browse/Models/annotationmodel.cpp
    ../../applications/mne_browse/Models/eventmodel.cpp
    ../../applications/mne_browse/Models/averagemodel.cpp
    ../../applications/mne_browse/Models/epochmodel.cpp
    ../../applications/mne_browse/Models/fiffblockreader.cpp
    ../../applications/mne_browse/Models/virtualchannelmodel.cpp

    # Utils (non-GUI only)
    ../../applications/mne_browse/Utils/datapackage.cpp
    ../../applications/mne_browse/Utils/filteroperator.cpp
    ../../applications/mne_browse/Utils/rawsettings.cpp
    ../../applications/mne_browse/Utils/sessionfilter.cpp

    # Headers for MOC
    ../../applications/mne_browse/Models/rawmodel.h
    ../../applications/mne_browse/Models/annotationmodel.h
    ../../applications/mne_browse/Models/eventmodel.h
    ../../applications/mne_browse/Models/averagemodel.h
    ../../applications/mne_browse/Models/epochmodel.h
    ../../applications/mne_browse/Models/fiffblockreader.h
    ../../applications/mne_browse/Models/virtualchannelmodel.h
    ../../applications/mne_browse/Utils/rawsettings.h
    ../../applications/mne_browse/Utils/sessionfilter.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

# Add mne_browse source directory to include path so relative includes work
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../applications/mne_browse
)

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
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
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
