# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2022-2026 MNE-CPP Authors
#   Gabriel Motta <gabrielbenmotta@gmail.com>

option(NO_MNE_SCAN_PLUGINS "Skip building MNE Scan plugins." OFF)

add_subdirectory(mne_scan)
add_subdirectory(libs)

if(NOT NO_MNE_SCAN_PLUGINS)
    add_subdirectory(plugins)
endif()
