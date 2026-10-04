#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the .stc and .w fixtures for test_source_estimate_io with mne-python.

Seven sources (four left, three right; vertex 70000 needs the third byte of the
.w vertex index), four time points from -100 ms in 4 ms steps, standard
normal data from seed 0. The test asserts these values after reading.
"""

from pathlib import Path

import mne
import numpy as np

out = Path(__file__).parent / "data"
out.mkdir(exist_ok=True)
data = np.random.RandomState(0).randn(7, 4)
vertices = [np.array([3, 17, 42, 1000]), np.array([5, 99, 70000])]
mne.SourceEstimate(data, vertices, tmin=-0.1, tstep=0.004).save(
    out / "py", ftype="stc", overwrite=True
)
mne.SourceEstimate(data[:, :1], vertices, tmin=0, tstep=1).save(
    out / "pyw", ftype="w", overwrite=True
)
print(data.astype(np.float32))
