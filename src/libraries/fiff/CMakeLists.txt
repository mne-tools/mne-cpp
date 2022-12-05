cmake_minimum_required(VERSION 3.14)
project(mne_fiff LANGUAGES CXX)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Network)

set(SOURCES
    fiff.cpp
    fiff_global.cpp
    fiff_tag.cpp
    fiff_coord_trans.cpp
    fiff_ch_info.cpp
    fiff_proj.cpp
    fiff_named_matrix.cpp
    fiff_raw_data.cpp
    fiff_ctf_comp.cpp
    fiff_id.cpp
    fiff_info.cpp
    fiff_raw_dir.cpp
    fiff_dig_point.cpp
    fiff_ch_pos.cpp
    fiff_cov.cpp
    fiff_stream.cpp
    fiff_dir_entry.cpp
    fiff_info_base.cpp
    fiff_evoked.cpp
    fiff_evoked_set.cpp
    fiff_io.cpp
    fiff_dig_point_set.cpp
    fiff_dir_node.cpp
    c/fiff_coord_trans_old.cpp
    c/fiff_sparse_matrix.cpp
    c/fiff_digitizer_data.cpp
    c/fiff_coord_trans_set.cpp
    fifffilesharer.cpp
)

set(FILE_TO_UPDATE fiff_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    eigen
    mne_utils
)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_FIFF_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(BUILD_SHARED_LIBS EQUAL OFF)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
