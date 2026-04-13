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
    capabilitycatalog.cpp
    capabilityutils.cpp
    fiffbuffer.cpp
    interfaces.cpp
    jsonrpcmessage.cpp
    llmtoolplanner.cpp
    manifestparser.cpp
    mcprouter.cpp
    pipelineparser.cpp
    scenecontextregistry.cpp
    viewmanager.cpp
    viewproviderregistry.cpp
    workflowgraph.cpp
    workflowmanager.cpp
)

set(HEADERS
    capabilitycatalog.h
    capabilityutils.h
    fiffbuffer.h
    ibuffer.h
    iskilloperator.h
    imcpneuroskill.h
    jsonrpcmessage.h
    llmtoolplanner.h
    manifestparser.h
    mcprouter.h
    pipelineparser.h
    scenecontextregistry.h
    studio_core_global.h
    viewmanager.h
    viewproviderregistry.h
    workflowgraph.h
    workflowmanager.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

mne_analyze_studio_configure_library_target(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:include>
)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${QT_REQUIRED_COMPONENT_LIBS}
    mne_analyze_studio_sdk
    mne_fiff
    mne_bids
    mne_fs
    Eigen3::Eigen
)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_ANALYZE_STUDIO_CORE_LIBRARY)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)
