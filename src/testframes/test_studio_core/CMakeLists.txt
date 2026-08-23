cmake_minimum_required(VERSION 3.14)
project(test_studio_core LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Test Widgets Svg Concurrent Network)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(SOURCES
    test_studio_core.cpp
  ../../applications/mne_analyze_studio/workbench/analysisresultwidget.cpp
  ../../applications/mne_analyze_studio/workbench/analysisresultwidget.h
  ../../applications/mne_analyze_studio/workbench/agentchatdockwidget.cpp
  ../../applications/mne_analyze_studio/workbench/agentchatdockwidget.h
    ../../applications/mne_analyze_studio/workbench/extensionhostedviewwidget.cpp
    ../../applications/mne_analyze_studio/workbench/extensionhostedviewwidget.h
    ../../applications/mne_analyze_studio/workbench/llmsettingsdialog.cpp
    ../../applications/mne_analyze_studio/workbench/llmsettingsdialog.h
    ../../applications/mne_analyze_studio/workbench/pillselectorwidget.cpp
    ../../applications/mne_analyze_studio/workbench/pillselectorwidget.h
    ../../applications/mne_analyze_studio/workbench/workflowminimapwidget.cpp
    ../../applications/mne_analyze_studio/workbench/workflowminimapwidget.h
)

if(WITH_CODE_COV)
    add_compile_options("--coverage" "-O1" "-g")
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})
else()
  add_executable(${PROJECT_NAME} ${SOURCES})
endif()

set(MNE_LIBS_REQUIRED
    mne_analyze_studio_core
    mne_fiff
    mne_utils
    mne_math
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ../../applications/mne_analyze_studio/core
    ../../applications/mne_analyze_studio/workbench
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_finalize_executable(${PROJECT_NAME})
endif()
