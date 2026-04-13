cmake_minimum_required(VERSION 3.14)
project(mne_bids
  LANGUAGES CXX
)

#Handle qt uic, moc, rrc automatically
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
  bids_global.cpp
  bids_path.cpp
  bids_tsv.cpp
  bids_channel.cpp
  bids_electrode.cpp
  bids_coordinate_system.cpp
  bids_event.cpp
  bids_dataset_description.cpp
  bids_raw_data.cpp
  readers/bids_edf_reader.cpp
  readers/bids_brain_vision_reader.cpp
)

set(HEADERS
  bids_global.h
  bids_const.h
  bids_event.h
  bids_channel.h
  bids_electrode.h
  bids_coordinate_system.h
  bids_path.h
  bids_tsv.h
  bids_dataset_description.h
  bids_raw_data.h
  readers/bids_abstract_format_reader.h
  readers/bids_edf_reader.h
  readers/bids_brain_vision_reader.h
)

set(FILE_TO_UPDATE bids_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:include>)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

target_link_libraries(${PROJECT_NAME} PRIVATE
  ${QT_REQUIRED_COMPONENT_LIBS}
  mne_fiff
  mne_utils
  Eigen3::Eigen)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_BIDS_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

# Install public headers for SDK
foreach(_header ${HEADERS})
    get_filename_component(_dir "${_header}" DIRECTORY)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/bids/${_dir}
        COMPONENT sdk
    )
endforeach()
