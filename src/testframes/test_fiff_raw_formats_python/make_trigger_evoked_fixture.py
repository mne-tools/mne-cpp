#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the trigger-channel evoked fixture for test_fiff_raw_formats_python.

Two EEG channels and STI 014 (cal 2.0), 8 samples, saved as an evoked file by
mne-python. MNE-C's mne_read_meas_data keeps the trigger channel as stim14,
divided by its calibration, i.e. the stored trigger codes.
"""

from pathlib import Path

import numpy as np

import mne

mne.set_log_level("ERROR")
out = Path(__file__).parent / "data" / "trigger-ave.fif"
info = mne.create_info(["EEG 001", "EEG 002", "STI 014"], 100.0, ["eeg", "eeg", "stim"])
info["chs"][2]["cal"] = 2.0
data = np.vstack([np.arange(8.0) * 1e-6, np.arange(8.0) * -1e-6, np.array([0, 6, 6, 0, 0, 10, 10, 0], float)])
evoked = mne.EvokedArray(data, info, tmin=0.0, nave=3, comment="trigger")
evoked.save(out, overwrite=True)
back = mne.read_evokeds(out)[0]
print(out.stat().st_size, back.data[2], back.info["chs"][2]["cal"])
