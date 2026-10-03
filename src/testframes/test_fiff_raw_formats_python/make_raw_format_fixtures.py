#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the raw storage-format fixtures for test_fiff_raw_formats_python.

The same 12 channels x 120 samples of the MNE-CPP sample raw file (with its
SSP projectors and first sample) are saved by mne-python in each of its
storage formats: short (FIFFT_DAU_PACK16), int, single and double. The
values printed at the end are the oracle the test asserts.
"""

import sys
from pathlib import Path

import mne
import numpy as np

src = Path(sys.argv[1])
out = Path(__file__).parent / "data"
raw = mne.io.read_raw_fif(src, preload=True, verbose=False)
picks = ["MEG 0113", "MEG 0112", "MEG 0111", "MEG 0122", "MEG 0123", "MEG 0121", "EEG 001", "EEG 002", "EEG 003", "STI 014", "EOG 061", "EEG 004"]
raw.pick(picks).crop(raw.times[1000], raw.times[1119])
raw.info["bads"] = []
with raw.info._unlock():
    raw.info["hpi_meas"] = []
    raw.info["hpi_results"] = []
# Integer formats store data / (cal * range); give each channel a cal small enough
# that the data span thousands of 16-bit steps without overflowing, as in recorded files.
for ch in raw.info["chs"]:
    ch["range"] = 1.0
    ch["cal"] = 1e-18 if ch["kind"] == mne.io.constants.FIFF.FIFFV_MEG_CH else (1.0 if ch["kind"] == mne.io.constants.FIFF.FIFFV_STIM_CH else 4e-12)
for fmt in ("short", "int", "single", "double"):
    raw.save(out / f"raw_{fmt}_raw.fif", fmt=fmt, overwrite=True, verbose=False)
for fmt in ("short", "int", "single", "double"):
    back = mne.io.read_raw_fif(out / f"raw_{fmt}_raw.fif", verbose=False)
    d = back.get_data()
    p = back.copy().apply_proj().get_data()
    sel = [10, 0, 6]  # EOG 061, MEG 0113, EEG 001
    print(fmt, back.first_samp, back.n_times, repr(float(np.abs(d[:9]).sum())), repr(float(d[6, 7])),
          repr(float(d[9].max())), repr(float(np.abs(p[:9]).sum())), repr(float(p[0, 5])),
          repr(float(np.abs(d[sel, 20:40]).sum())), repr(float(np.abs(p[sel, 20:40]).sum())))
