cmake_minimum_required(VERSION 3.14)
project(mne_mna
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
  mna_global.cpp
  mna_file_ref.cpp
  mna_step.cpp
  mna_recording.cpp
  mna_session.cpp
  mna_subject.cpp
  mna_project.cpp
  mna_io.cpp
  mna_port.cpp
  mna_node.cpp
  mna_op_schema.cpp
  mna_op_registry.cpp
  mna_registry_loader.cpp
  mna_param_binding.cpp
  mna_param_tree.cpp
  mna_graph.cpp
  mna_graph_executor.cpp
  mna_script.cpp
  mna_verification.cpp
)

set(HEADERS
  mna_global.h
  mna_types.h
  mna_file_ref.h
  mna_step.h
  mna_recording.h
  mna_session.h
  mna_subject.h
  mna_project.h
  mna_io.h
  mna_port.h
  mna_node.h
  mna_op_schema.h
  mna_op_registry.h
  mna_registry_loader.h
  mna_param_binding.h
  mna_param_tree.h
  mna_graph.h
  mna_graph_executor.h
  mna_script.h
  mna_verification.h
)

set(FILE_TO_UPDATE mna_global.cpp)

set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:include>)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED mne_utils)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen)

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_MNA_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

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
        DESTINATION include/mna/${_dir}
        COMPONENT sdk
    )
endforeach()
