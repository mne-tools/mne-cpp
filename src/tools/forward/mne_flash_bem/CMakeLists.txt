# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_flash_bem LANGUAGES CXX)

set(SOURCES
    main.cpp
    mne_flash_bem_settings.cpp
    flashbem.cpp
)

set(HEADERS
    mne_flash_bem_settings.h
    flashbem.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

set(MNE_LIBS_REQUIRED
    mne_mne
    mne_fiff
    mne_fs
    mne_mri
    mne_base
    mne_math
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${MNE_LIBS_REQUIRED}
    Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

# Install
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
