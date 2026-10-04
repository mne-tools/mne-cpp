#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the SSS-info fixture for test_fiff_raw_formats_python.

mne-python Maxwell-filters 50 ms of sample_audvis_trunc_raw.fif (origin (0, 0, 40)
mm head, internal order 8, external order 3) and its processing history, which
holds the FIFFB_SSS_INFO block, is copied onto a small 3-channel recording. The
printed values are what mne-python reads back: job, frame, origin, nchan, number
of components, internal and external components in use.
"""

from pathlib import Path

import numpy as np

import mne

mne.set_log_level("ERROR")
here = Path(__file__).parent
sample = here / "../../../resources/data/mne-cpp-test-data/MEG/sample/sample_audvis_trunc_raw.fif"
raw = mne.io.read_raw_fif(sample).crop(0, 0.05).pick(["meg"]).load_data()
raw.info["bads"] = []
sss = mne.preprocessing.maxwell_filter(raw, origin=(0.0, 0.0, 0.04), int_order=8, ext_order=3, coord_frame="head")

info = mne.create_info(["EEG 001", "EEG 002", "MISC 001"], 100.0, ["eeg", "eeg", "misc"])
small = mne.io.RawArray(np.zeros((3, 20)), info)
with small.info._unlock():
    small.info["proc_history"] = sss.info["proc_history"]
out = here / "data" / "raw_sss_raw.fif"
small.save(out, overwrite=True)

si = mne.io.read_raw_fif(out).info["proc_history"][0]["max_info"]["sss_info"]
comps = si["components"]
n_in = 8 * (8 + 2)
print(si["job"], si["frame"], si["origin"], si["nchan"], len(comps), int(comps[:n_in].sum()), len(comps) - n_in)
