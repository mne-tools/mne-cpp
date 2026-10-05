#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write a shuffled, radially scaled ico-4 sphere and the indices of its ico-2 vertices.

The ico-2 vertices of mne-python's icosahedron are a subset of ico-4, so a correct
icosahedral downsampling of the shuffled sphere must select exactly these indices.
"""

from pathlib import Path

import numpy as np
from mne.surface import _get_ico_surface
from scipy.spatial import cKDTree

data = Path(__file__).parent / "data"
ico4 = _get_ico_surface(4)["rr"]
perm = np.random.default_rng(0).permutation(len(ico4))
np.savetxt(data / "ico4-perm.txt", ico4[perm] * np.linspace(0.08, 0.12, len(ico4))[:, None], fmt="%.9g")
dist, idx = cKDTree(ico4).query(_get_ico_surface(2)["rr"])
assert dist.max() == 0
expected = np.sort(np.argsort(perm)[idx])
(data / "ico2-in-ico4-perm.txt").write_text(" ".join(map(str, expected)) + "\n")
