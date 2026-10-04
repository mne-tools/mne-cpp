#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the old-style evoked fixture for test_fiff_evoked_python.

Old evoked files carry their own channel information inside the evoked block
(FIFF_NCHAN, FIFF_SFREQ, FIFF_CH_INFO) and store one FIFF_EPOCH tag per
channel instead of a single matrix. The measurement info lists six channels
at 100 Hz with unit calibration; the evoked block redefines them at 200 Hz
with calibrations 0.5 ... 5.5 (mne-python does not update nchan or the names
from local channel information, so both must match). The aspect has a comment of its own and no
FIFF_NAVE (so nave is 1). The values printed at the end are the oracle the
test asserts (mne.read_evokeds).
"""

from pathlib import Path

import mne
import numpy as np
from mne._fiff.constants import FIFF
from mne._fiff.meas_info import write_meas_info
from mne._fiff.write import (
    end_block,
    start_and_end_file,
    start_block,
    write_ch_info,
    write_float,
    write_id,
    write_int,
    write_string,
)

out = Path(__file__).parent / "data" / "old_style-ave.fif"
info = mne.create_info(["MAG 1", "MAG 2", "MAG 3", "EEG 1", "EEG 2", "EEG 3"], 100.0,
                       ["mag"] * 3 + ["eeg"] * 3)
local = [dict(ch) for ch in info["chs"]]
for k, ch in enumerate(local):
    ch["cal"] = 0.5 + k
first, last = -4, 15
rng = np.random.default_rng(23)
epochs = rng.standard_normal((6, last - first + 1)).astype(np.float32)

with start_and_end_file(out) as fid:
    start_block(fid, FIFF.FIFFB_MEAS)
    write_id(fid, FIFF.FIFF_BLOCK_ID)
    write_meas_info(fid, info)
    start_block(fid, FIFF.FIFFB_PROCESSED_DATA)
    start_block(fid, FIFF.FIFFB_EVOKED)
    write_string(fid, FIFF.FIFF_COMMENT, "evoked block comment")
    write_int(fid, FIFF.FIFF_FIRST_SAMPLE, first)
    write_int(fid, FIFF.FIFF_LAST_SAMPLE, last)
    write_int(fid, FIFF.FIFF_NCHAN, len(local))
    write_float(fid, FIFF.FIFF_SFREQ, 200.0)
    for ch in local:
        write_ch_info(fid, ch)
    start_block(fid, FIFF.FIFFB_ASPECT)
    write_string(fid, FIFF.FIFF_COMMENT, "aspect comment")
    write_int(fid, FIFF.FIFF_ASPECT_KIND, FIFF.FIFFV_ASPECT_STD_ERR)
    for row in epochs:
        write_float(fid, FIFF.FIFF_EPOCH, row)
    end_block(fid, FIFF.FIFFB_ASPECT)
    end_block(fid, FIFF.FIFFB_EVOKED)
    end_block(fid, FIFF.FIFFB_PROCESSED_DATA)
    end_block(fid, FIFF.FIFFB_MEAS)

ev_info = mne._fiff.meas_info.Info
set_item = ev_info.__setitem__


def _set_unlocked(self, key, val):
    with self._unlock():
        set_item(self, key, val)


# mne-python 1.11 (and main) assigns info["chs"] for local channel information
# on a locked Info and raises; unlock it so its reader can serve as the oracle.
ev_info.__setitem__ = _set_unlocked
ev = mne.read_evokeds(out, 0, baseline=None, proj=False, verbose=False)
print(ev.ch_names, ev.info["sfreq"], ev.nave, ev.kind, repr(ev.comment), ev.data.shape)
print(repr(float(ev.times[0])), repr(float(ev.data[0, 0])), repr(float(ev.data[5, 19])),
      repr(float(np.abs(ev.data).sum())))
