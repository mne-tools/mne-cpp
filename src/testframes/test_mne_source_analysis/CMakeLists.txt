cmake_minimum_required(VERSION 3.14)
project(test_mne_source_analysis LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

set(QT_REQUIRED_COMPONENTS Core Test)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES test_mne_source_analysis.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Test
    eigen
    mne_fiff
    mne_mne
    mne_fs
    mne_inv
    mne_utils
    mne_math)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
