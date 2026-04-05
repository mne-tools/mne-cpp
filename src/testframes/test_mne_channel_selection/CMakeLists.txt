cmake_minimum_required(VERSION 3.14)
project(test_mne_channel_selection LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Test)
find_package(Eigen3 REQUIRED)

set(SOURCES test_mne_channel_selection.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libraries)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Test
    Eigen3::Eigen
    mne_mne
    mne_fiff
    mne_utils
    mne_math)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
