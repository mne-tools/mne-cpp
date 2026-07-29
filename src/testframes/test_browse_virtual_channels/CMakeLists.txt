cmake_minimum_required(VERSION 3.14)
project(test_browse_virtual_channels LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Gui Test Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

# mne_browse is an executable target rather than a library, so the model under
# test is compiled into this test directly. Only virtualchannelmodel is pulled
# in: it has no dependency on the browse windows, which would drag the whole
# application in behind them.
set(MNE_BROWSE_DIR ${CMAKE_SOURCE_DIR}/src/applications/mne_browse)

set(SOURCES
    test_browse_virtual_channels.cpp
    ${MNE_BROWSE_DIR}/Models/virtualchannelmodel.cpp
)

set(HEADERS
    ${MNE_BROWSE_DIR}/Models/virtualchannelmodel.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES} ${HEADERS})
else()
  add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
endif()

set(MNE_LIBS_REQUIRED
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
    ${MNE_BROWSE_DIR}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../../out/$<CONFIG>/tests
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  qt_finalize_executable(${PROJECT_NAME})
endif()
