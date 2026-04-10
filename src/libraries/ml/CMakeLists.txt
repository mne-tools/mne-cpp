cmake_minimum_required(VERSION 3.14)
project(mne_ml LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(QT_REQUIRED_COMPONENTS Core)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_REQUIRED_COMPONENTS})

set(SOURCES
  ml_global.cpp
  ml_tensor.cpp
  ml_onnx_model.cpp
  ml_linear_model.cpp
  ml_scaler.cpp
  ml_pipeline.cpp
)

set(HEADERS
  ml_global.h
  ml_types.h
  ml_tensor.h
  ml_model.h
  ml_onnx_model.h
  ml_linear_model.h
  ml_scaler.h
  ml_pipeline.h
)

set(FILE_TO_UPDATE ml_global.cpp)
set(SOURCE_PATHS ${SOURCES})
list(TRANSFORM SOURCE_PATHS PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
set_source_files_properties(${FILE_TO_UPDATE} PROPERTIES OBJECT_DEPENDS "${SOURCE_PATHS}")

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..> $<INSTALL_INTERFACE:include>)

set(QT_REQUIRED_COMPONENT_LIBS ${QT_REQUIRED_COMPONENTS})
list(TRANSFORM QT_REQUIRED_COMPONENT_LIBS PREPEND "Qt${QT_VERSION_MAJOR}::")

set(MNE_LIBS_REQUIRED mne_utils mne_math)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${QT_REQUIRED_COMPONENT_LIBS}
    ${MNE_LIBS_REQUIRED}
    eigen)

# ONNX Runtime (optional — enabled when USE_ONNXRUNTIME=ON and the package was found)
if(TARGET onnxruntime::onnxruntime)
    target_link_libraries(${PROJECT_NAME} PRIVATE onnxruntime::onnxruntime)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_USE_ONNXRUNTIME)
    message(STATUS "[mne_ml] ONNX Runtime enabled")
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_ML_LIBRARY MNE_GIT_HASH_SHORT="${MNE_GIT_HASH_SHORT}" MNE_GIT_HASH_LONG="${MNE_GIT_HASH_LONG}")

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()

install(TARGETS ${PROJECT_NAME} ${MNE_CPP_EXPORT_SET}
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT sdk
)

foreach(_header ${HEADERS})
    get_filename_component(_dir "${_header}" DIRECTORY)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/${_header}"
        DESTINATION include/ml/${_dir}
        COMPONENT sdk
    )
endforeach()
