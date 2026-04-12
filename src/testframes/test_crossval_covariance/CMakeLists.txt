cmake_minimum_required(VERSION 3.14)
project(test_crossval_covariance LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Test Core)
find_package(Eigen3 REQUIRED)

enable_testing()

set(SOURCES
    test_crossval_covariance.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    Qt6::Test
    Qt6::Core
    eigen
    mne_sts
    mne_fiff
    mne_utils
    mne_math
)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
