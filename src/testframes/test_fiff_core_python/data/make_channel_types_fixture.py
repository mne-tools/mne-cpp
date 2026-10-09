#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write a raw file with one channel of every mne-python channel type for test_fiff_core_python.

The channel kinds, units and coil types are those mne.create_info assigns (mne-python 1.11).
Run from this directory: python make_channel_types_fixture.py
"""

import numpy as np

import mne

TYPES = list(mne.io.get_channel_type_constants())
info = mne.create_info([f"CH{k:02d}" for k in range(len(TYPES))], 100.0, TYPES)
raw = mne.io.RawArray(np.zeros((len(TYPES), 10)), info)
raw.save("channel-types-raw.fif", overwrite=True)
for k, ch_type in enumerate(TYPES):
    print(f"{k} {mne.channel_type(raw.info, k)}")
