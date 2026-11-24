# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_inverse_pipeline LANGUAGES CXX)

set(SOURCES
    main.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

set(MNE_LIBS_REQUIRED
    mne_mna
    mne_inv
    mne_fwd
    mne_mne
    mne_fiff
    mne_fs
    mne_base
    mne_math
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROGRAM_VERSION="${MNE_CPP_VERSION}"
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin COMPONENT runtime
)
