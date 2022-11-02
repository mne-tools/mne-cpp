cmake_minimum_required(VERSION 3.14)
project(mne_fwd LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent)

set(SOURCES
    computeFwd/compute_fwd_settings.cpp
    computeFwd/compute_fwd.cpp
    fwd_bem_model.cpp
    fwd_bem_solution.cpp
    fwd_coil.cpp
    fwd_coil_set.cpp
    fwd_comp_data.cpp
    fwd_eeg_sphere_layer.cpp
    fwd_eeg_sphere_model.cpp
    fwd_eeg_sphere_model_set.cpp
    fwd_global.cpp
    fwd_thread_arg.cpp
)

set(FILE_TO_UPDATE fwd_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    eigen
    mne_utils
    mne_fiff
    mne_fs
    mne_mne
)

target_compile_definitions(${PROJECT_NAME} PRIVATE fwd_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")
