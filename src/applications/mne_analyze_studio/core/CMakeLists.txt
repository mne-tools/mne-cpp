# Shared core library for MNE Analyze Studio.

cmake_minimum_required(VERSION 3.15)
project(mne_analyze_studio_core LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core Network Widgets)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
    fiffbuffer.cpp
    interfaces.cpp
    jsonrpcmessage.cpp
    mcprouter.cpp
    scenecontextregistry.cpp
    viewmanager.cpp
)

set(HEADERS
    fiffbuffer.h
    ibuffer.h
    imcpneuroskill.h
    jsonrpcmessage.h
    mcprouter.h
    scenecontextregistry.h
    studio_core_global.h
    viewmanager.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set_target_properties(${PROJECT_NAME} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib
    RUNTIME_OUTPUT_DIRECTORY ${BINARY_OUTPUT_DIRECTORY}/lib
)

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_fiff
    mne_bids
    mne_fs
    eigen
)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_ANALYZE_STUDIO_CORE_LIBRARY)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)
