cmake_minimum_required(VERSION 3.14)
project(test_fwd_library LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Network Test)
qt_standard_project_setup()

qt_add_executable(test_fwd_library test_fwd_library.cpp)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(test_fwd_library PRIVATE STATICBUILD)
endif()

target_link_libraries(test_fwd_library PRIVATE
    mne_disp3D_rhi
    mne_disp
    mne_dsp
    mne_conn
    mne_inv
    mne_fwd
    mne_mne
    mne_fiff
    mne_fs
    mne_utils
    mne_math
    mne_events
    eigen
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    Qt6::Network
    Qt6::Test
)

qt_finalize_executable(test_fwd_library)
