# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_show_fiff LANGUAGES CXX)

add_executable(${PROJECT_NAME}
    main.cpp
    mne_fiff_contents.cpp
    mne_show_fiff_settings.cpp
    mne_fiff_contents.h
    mne_show_fiff_settings.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    mne_mne
    mne_fiff
    mne_fs
    mne_base
    mne_math
    Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications
)
