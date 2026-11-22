# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_ml LANGUAGES CXX)

set(SOURCES
  ml_tensor.cpp
  ml_onnx_model.cpp
  ml_trainer.cpp
)

set(HEADERS
  ml_global.h
  ml_types.h
  ml_tensor.h
  ml_model.h
  ml_onnx_model.h
  ml_trainer.h
)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
include(${CMAKE_CURRENT_LIST_DIR}/../../../../cmake/MneWarnings.cmake)
mne_warn_on_narrowing(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../..> $<INSTALL_INTERFACE:include>)

set(MNE_LIBS_REQUIRED mne_base mne_math)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen)

# MLTrainer exposes PythonRunner; WebAssembly cannot start processes
if(NOT WASM)
    target_link_libraries(${PROJECT_NAME} PUBLIC mne_pybridge)
endif()

# ONNX Runtime (optional — enabled when USE_ONNXRUNTIME=ON and the package was found)
if(TARGET onnxruntime::onnxruntime)
    target_link_libraries(${PROJECT_NAME} PRIVATE onnxruntime::onnxruntime)
    target_compile_definitions(${PROJECT_NAME} PUBLIC MNE_USE_ONNXRUNTIME)
    message(STATUS "[mne_ml] ONNX Runtime enabled")

    # Copy the ONNX Runtime shared library next to mne_ml, in the build and in the installed SDK, so the runtime
    # linker can find it (via RPATH on macOS/Linux, or same-directory on Windows).
    get_target_property(_ort_loc onnxruntime::onnxruntime IMPORTED_LOCATION)
    if(NOT _ort_loc)
        get_target_property(_ort_loc onnxruntime::onnxruntime IMPORTED_LOCATION_RELEASE)
    endif()
    if(_ort_loc)
        get_filename_component(_ort_dir "${_ort_loc}" DIRECTORY)
        if(WIN32)
            file(GLOB _ort_dlls "${_ort_dir}/*.dll")
            foreach(_dll ${_ort_dlls})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dll}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                    COMMENT "Copying ORT DLL: ${_dll}")
            endforeach()
            install(FILES ${_ort_dlls} DESTINATION bin COMPONENT runtime)
        elseif(APPLE)
            file(GLOB _ort_dylibs "${_ort_dir}/libonnxruntime*.dylib")
            foreach(_dylib ${_ort_dylibs})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dylib}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                    COMMENT "Copying ORT dylib: ${_dylib}")
            endforeach()
            install(FILES ${_ort_dylibs} DESTINATION lib COMPONENT runtime)
        else()
            file(GLOB _ort_sos "${_ort_dir}/libonnxruntime*.so*")
            foreach(_so ${_ort_sos})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_so}" "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
                    COMMENT "Copying ORT so: ${_so}")
            endforeach()
            install(FILES ${_ort_sos} DESTINATION lib COMPONENT runtime)
        endif()
    endif()
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_ML_LIBRARY)

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
        DESTINATION include/mne/ml/${_dir}
        COMPONENT sdk
    )
endforeach()
