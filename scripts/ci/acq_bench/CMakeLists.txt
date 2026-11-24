# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

# Builds mne_acq and mne_acq_bench from one MNE-CPP source tree, without the rest of MNE-CPP, so compare.py can
# run the suite of two commits side by side. Usage:
#   cmake -S scripts/ci/acq_bench -B build-bench -DACQ_ROOT=<checkout> -DCMAKE_BUILD_TYPE=Release
cmake_minimum_required(VERSION 3.21)
project(mne_acq_bench_ab LANGUAGES CXX)

set(ACQ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." CACHE PATH "MNE-CPP checkout to build mne_acq_bench from")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

# Commits before the move to src/libs/mne compare against this one
if(EXISTS "${ACQ_ROOT}/src/libs/mne/acq")
  add_subdirectory("${ACQ_ROOT}/src/libs/mne/acq" acq)
else()
  add_subdirectory("${ACQ_ROOT}/src/libraries/acq" acq)
endif()
add_executable(mne_acq_bench "${ACQ_ROOT}/src/tools/acquisition/mne_acq_bench/main.cpp")
target_link_libraries(mne_acq_bench PRIVATE mne_acq)
# The tool is standard C++ since 3.0; older commits need Qt Core
file(READ "${ACQ_ROOT}/src/tools/acquisition/mne_acq_bench/main.cpp" _bench_source)
if(_bench_source MATCHES "QCoreApplication")
  find_package(Qt6 REQUIRED COMPONENTS Core)
  target_link_libraries(mne_acq_bench PRIVATE Qt6::Core)
endif()
unset(_bench_source)
# The tool logs through mne_base since 3.0; older commits do not need it
if(EXISTS "${ACQ_ROOT}/src/libs/mne/base")
  add_subdirectory("${ACQ_ROOT}/src/libs/mne/base" base)
  target_link_libraries(mne_acq_bench PRIVATE mne_base)
endif()
