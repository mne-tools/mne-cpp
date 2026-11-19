# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# A downstream project that uses an installed MNE-CPP SDK through find_package, as an application would:
#   cmake -S scripts/ci/sdk_consumer -B consumer -DCMAKE_PREFIX_PATH="<sdk prefix>"
#   cmake --build consumer && ctest --test-dir consumer
# It needs no Qt: the components it uses are Qt-free, also in an SDK built with Qt.

cmake_minimum_required(VERSION 3.21)
project(mne_cpp_sdk_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(MNE-CPP REQUIRED COMPONENTS fiff dsp acq)

# Qt-free components must not pull in Qt.
foreach(_qt Qt6::Core Qt5::Core)
    if(TARGET ${_qt})
        message(FATAL_ERROR "find_package(MNE-CPP COMPONENTS fiff dsp acq) found ${_qt}")
    endif()
endforeach()

add_executable(sdk_consumer main.cpp)
target_link_libraries(sdk_consumer PRIVATE MNE-CPP::mne_fiff MNE-CPP::mne_dsp MNE-CPP::mne_acq)

enable_testing()
add_test(NAME sdk_consumer COMMAND sdk_consumer)
if(WIN32)
    # Windows has no RPATH: the SDK's DLLs must be on the PATH.
    set_tests_properties(sdk_consumer PROPERTIES ENVIRONMENT
        "PATH=$<TARGET_FILE_DIR:MNE-CPP::mne_fiff>\\;$ENV{PATH}")
endif()
