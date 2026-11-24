# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

cmake_minimum_required(VERSION 3.14)
project(mne_anonymize LANGUAGES CXX)

# Standard C++ only. The anonymization core (fiffanonymizer, settingscontrollercl) is compiled into this tool, into
# the GUI application src/apps/mne_anonymize_gui and into test_mne_anonymize.
add_executable(${PROJECT_NAME}
    main.cpp
    fiffanonymizer.cpp
    settingscontrollercl.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    mne_fiff
    mne_base
    Eigen3::Eigen
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE FALSE
    WIN32_EXECUTABLE FALSE
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION . COMPONENT applications
    RUNTIME DESTINATION bin COMPONENT applications
    LIBRARY DESTINATION lib COMPONENT applications)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE STATICBUILD)
endif()
