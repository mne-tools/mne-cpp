cmake_minimum_required(VERSION 3.14)
project(test_studio_skill_host LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(WITH_CODE_COV)
    add_compile_options("--coverage" "-O1" "-g")
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network Test)

set(SOURCES
    test_studio_skill_host.cpp
    ../../applications/mne_analyze_studio/skill_host/skillhostservice.cpp
    ../../applications/mne_analyze_studio/skill_host/skillhostservice.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ../../applications/mne_analyze_studio/skill_host
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    MNE_STUDIO_EXTENSIONS_DIR="${CMAKE_SOURCE_DIR}/src/applications/mne_analyze_studio/extensions"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Test
    mne_analyze_studio_core
    mne_analyze_studio_temporal_filter_skill
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_tests_properties(${PROJECT_NAME} PROPERTIES
    LABELS "integration"
    TIMEOUT 60
)
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()