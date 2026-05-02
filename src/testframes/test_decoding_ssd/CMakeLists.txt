cmake_minimum_required(VERSION 3.14)
project(test_decoding_ssd LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)

set(SOURCES test_decoding_ssd.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    Eigen3::Eigen
    mne_decoding)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
