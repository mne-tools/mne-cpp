cmake_minimum_required(VERSION 3.14)
project(test_wasm_multi_pass LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core Gui Widgets ShaderTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets ShaderTools)
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS GuiPrivate)

set(SOURCES main.cpp)

qt_add_executable(${PROJECT_NAME} MANUAL_FINALIZATION ${SOURCES})

# Compile GLSL → .qsb via qt_add_shaders (cross-backend: SPIR-V, GLSL, MSL, HLSL)
qt_add_shaders(${PROJECT_NAME} "multipass_shaders"
    PREFIX
        "/"
    BASE
        "shaders"
    FILES
        "shaders/multipass.vert"
        "shaders/multipass.frag"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::GuiPrivate
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE FALSE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
