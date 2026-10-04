#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the unit-multiplier fixture for test_fiff_raw_formats_python.

Two EEG channels and STI 014, 20 samples, saved as int by mne-python. EEG 002
has unit_mul -6 (stored in micro-units) and STI 014 a range of 2.0. MNE-C's
mne_open_raw_data folds unit_mul into cal and resets the trigger range to 1,
so the stim channel reads as the integer trigger codes. The printed values are
mne-python's reading, which keeps the stored range and does not apply unit_mul.
"""

from pathlib import Path

import numpy as np

import mne
from mne.io.constants import FIFF

mne.set_log_level("ERROR")
out = Path(__file__).parent / "data" / "raw_unit_mul_raw.fif"
info = mne.create_info(["EEG 001", "EEG 002", "STI 014"], 100.0, ["eeg", "eeg", "stim"])
data = np.vstack([np.arange(20.0) * 1e-6, np.arange(20.0) * -2e-6, np.tile([0.0, 2.0, 4.0, 0.0], 5)])
raw = mne.io.RawArray(data, info)
for ch in raw.info["chs"]:
    ch["cal"] = 1e-7 if ch["kind"] == FIFF.FIFFV_EEG_CH else 1.0
raw.info["chs"][1]["unit_mul"] = -6
raw.info["chs"][2]["range"] = 2.0
raw.save(out, fmt="int", overwrite=True)
back = mne.io.read_raw_fif(out)
print(out.stat().st_size, [(c["cal"], c["range"], c["unit_mul"]) for c in back.info["chs"]], back.get_data()[:, :4])
