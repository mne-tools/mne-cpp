#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the CTF compensation fixtures for test_mne_ctf_comp_python.

The input is mne-python's BSD-3-Clause test file
mne/io/tests/data/test_ctf_comp_raw.fif (third-order gradient compensation).
It is cut down to every 40th MEG channel, all reference channels and 21
samples. mne-python then writes the same data at compensation grades 0 and 1;
those files are the oracle.
"""

import sys
from pathlib import Path

import mne

src = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(mne.__file__).parent / "io/tests/data/test_ctf_comp_raw.fif"
out = Path(__file__).parent / "data"
raw = mne.io.read_raw_fif(src, preload=True, verbose=False)
megs = mne.pick_types(raw.info, meg=True, ref_meg=False)
refs = mne.pick_types(raw.info, meg=False, ref_meg=True)
keep = [raw.ch_names[i] for i in list(megs[::40]) + list(refs)]
raw.pick(keep).crop(0, 20 / raw.info["sfreq"])
assert raw.compensation_grade == 3
raw.save(out / "ctf_grade3_raw.fif", overwrite=True)
for grade in (0, 1):
    raw.copy().apply_gradient_compensation(grade).save(out / f"ctf_grade{grade}_raw.fif", overwrite=True)
print(mne.__version__)
