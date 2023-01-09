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

set(HEADERS
    fiff.h
    fiff_global.h
    fiff_explain.h
    fiff_file.h
    fiff_types.h
    fiff_id.h
    fiff_constants.h
    fiff_tag.h
    fiff_coord_trans.h
    fiff_ch_info.h
    fiff_proj.h
    fiff_named_matrix.h
    fiff_ctf_comp.h
    fiff_info.h
    fiff_raw_data.h
    fiff_dir_entry.h
    fiff_raw_dir.h
    fiff_dig_point.h
    fiff_ch_pos.h
    fiff_cov.h
    fiff_stream.h
    fiff_info_base.h
    fiff_evoked.h
    fiff_evoked_set.h
    fiff_io.h
    fiff_dig_point_set.h
    fiff_dir_node.h
    c/fiff_coord_trans_old.h
    c/fiff_sparse_matrix.h
    c/fiff_types_mne-c.h
    c/fiff_digitizer_data.h
    c/fiff_coord_trans_set.h
    fifffilesharer.h
)

set(FILE_TO_UPDATE fiff_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set(FFTW_LIBS "")

if(USE_FFTW)
  if (WIN32)
    set(FFTW_LIBS
      ${FFTW_DIR_LIBS}/libfftw3-3.dll
      ${FFTW_DIR_LIBS}/libfftw3f-3.dll
      ${FFTW_DIR_LIBS}/libfftwf3l-3.dll
    )
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_DIR_INCLUDE})
  elseif(UNIX AND NOT APPLE)
    set(FFTW_LIBS ${FFTW_DIR_LIBS}/lib/libfftw3.so)
    target_include_directories(${PROJECT_NAME} PRIVATE ${FFTW_DIR_INCLUDE}/api)
  endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ../)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    eigen
    mne_utils
    ${FFTW_LIBS})

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_FIFF_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
