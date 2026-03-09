cmake_minimum_required(VERSION 3.14)
project(test_fwd_bem_data LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Network Test)
qt_standard_project_setup()

qt_add_executable(test_fwd_bem_data test_fwd_bem_data.cpp)

target_compile_definitions(test_fwd_bem_data PRIVATE STATICBUILD)

target_link_libraries(test_fwd_bem_data PRIVATE
    Qt6::Core Qt6::Widgets Qt6::Concurrent Qt6::Network Qt6::Test
    mne_rtprocessing
    mne_connectivity
    mne_inverse
    mne_fwd
    mne_mne
    mne_fiff
    mne_fs
    mne_utils
    mne_events
    mne_disp
    mne_disp3D_rhi
    eigen
)

qt_finalize_executable(test_fwd_bem_data)
