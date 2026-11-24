# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_acq_bench LANGUAGES CXX)

# Standard C++ only, like the library it measures
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE mne_acq mne_base)
if(TARGET mne_acq_fiff)
    target_link_libraries(${PROJECT_NAME} PRIVATE mne_acq_fiff)
    target_compile_definitions(${PROJECT_NAME} PRIVATE MNE_ACQ_BENCH_RECORDING)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications
)
