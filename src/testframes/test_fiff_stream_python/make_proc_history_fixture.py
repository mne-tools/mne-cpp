#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the processing-history fixture for test_fiff_stream_python.

mne-python's BSD-3-Clause test file mne/io/tests/data/test_chpi_raw_sss.fif
(three MaxFilter runs in its processing history) is cut to three channels and
three samples, without the HPI measurement block. The printed values are what
mne-python reads back from the processing history.
"""

import sys
from pathlib import Path

import mne

mne.set_log_level("ERROR")
raw = mne.io.read_raw_fif(Path(sys.argv[1]))
raw.crop(0, 2 / raw.info["sfreq"]).pick(raw.ch_names[:3])
with raw.info._unlock():
    raw.info["hpi_meas"] = []
    raw.info["hpi_results"] = []
out = Path(__file__).parent / "data" / "sss_history_raw.fif"
raw.save(out, overwrite=True)
ph = mne.io.read_info(out)["proc_history"]
print(out.stat().st_size, len(ph), [p["max_info"]["sss_info"].get("in_order") for p in ph],
      [p["max_info"]["sss_info"].get("nchan") for p in ph], [len(p["max_info"]["sss_ctc"]) for p in ph])
