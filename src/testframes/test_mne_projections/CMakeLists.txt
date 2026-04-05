cmake_minimum_required(VERSION 3.14)
project(test_mne_projections LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)
find_package(Eigen3 REQUIRED)

add_executable(${PROJECT_NAME} test_mne_projections.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    Eigen3::Eigen
    mne_fiff
    mne_mne
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
