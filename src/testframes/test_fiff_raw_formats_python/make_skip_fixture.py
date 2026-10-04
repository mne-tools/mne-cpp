#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the acquisition-skip fixture for test_fiff_raw_formats_python.

Three channels x 100 samples (ramp data, first_samp 50) saved with 10-sample
buffers; samples 70-89 carry a bad_acq_skip annotation, so mne-python writes a
FIFF_DATA_SKIP of two buffers instead of them. The printed values are what
mne-python reads back (skipped samples read as zeros).
"""

from pathlib import Path

import numpy as np

import mne

mne.set_log_level("ERROR")
out = Path(__file__).parent / "data" / "raw_skip_raw.fif"
info = mne.create_info(["EEG 001", "EEG 002", "MISC 001"], 100.0, ["eeg", "eeg", "misc"])
data = np.vstack([np.arange(100.0), -np.arange(100.0), np.full(100, 7.0)]) * 1e-6
raw = mne.io.RawArray(data, info, first_samp=50)
raw.set_annotations(mne.Annotations(onset=[raw.times[70]], duration=[0.2], description=["bad_acq_skip"]))
raw.save(out, buffer_size_sec=0.1, fmt="single", overwrite=True)
back = mne.io.read_raw_fif(out)
d = back.get_data()
print(out.stat().st_size, back.first_samp, back.last_samp, d[0, 65:95] * 1e6, d[1, 95] * 1e6)
