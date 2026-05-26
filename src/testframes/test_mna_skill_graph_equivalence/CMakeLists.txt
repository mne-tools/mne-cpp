cmake_minimum_required(VERSION 3.14)
project(test_mna_skill_graph_equivalence LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

enable_testing()

set(SOURCES test_mna_skill_graph_equivalence.cpp)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_analyze_studio_core
    mne_analyze_studio_temporal_filter_skill
    mne_analyze_studio_source_estimation_skill
    mne_inv
    mne_fiff
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    MNA_WORKFLOWS_SRC_DIR="${CMAKE_SOURCE_DIR}/src/applications/mne_analyze_studio/examples/workflows"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE FALSE
    MACOSX_BUNDLE FALSE
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

# Ensure runtime can find the shared libs we link against.
if(APPLE)
    set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT
        "DYLD_LIBRARY_PATH=$<TARGET_FILE_DIR:mne_analyze_studio_core>:$<TARGET_FILE_DIR:mne_analyze_studio_temporal_filter_skill>:$<TARGET_FILE_DIR:mne_analyze_studio_source_estimation_skill>:$<TARGET_FILE_DIR:mne_inv>:$<TARGET_FILE_DIR:mne_mne>:$<TARGET_FILE_DIR:mne_fiff>:$<TARGET_FILE_DIR:mne_utils>")
elseif(UNIX)
    set_tests_properties(${PROJECT_NAME} PROPERTIES ENVIRONMENT
        "LD_LIBRARY_PATH=$<TARGET_FILE_DIR:mne_analyze_studio_core>:$<TARGET_FILE_DIR:mne_analyze_studio_temporal_filter_skill>:$<TARGET_FILE_DIR:mne_analyze_studio_source_estimation_skill>:$<TARGET_FILE_DIR:mne_inv>:$<TARGET_FILE_DIR:mne_mne>:$<TARGET_FILE_DIR:mne_fiff>:$<TARGET_FILE_DIR:mne_utils>")
endif()
