#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the morph-map fixture for test_mne_source_space_ops_python.

Two small subjects whose lh/rh.sphere.reg are an ico-2 and an ico-3 sphere of
different radii, the ico-3 one rotated so that vertices fall inside triangles.
mne.read_morph_map computes and saves the maps both ways; the test reads the
saved file and recomputes the maps from the spheres.
"""

from pathlib import Path

import numpy as np
from scipy.spatial.transform import Rotation

import mne

mne.set_log_level("ERROR")
subjects = Path(__file__).parent / "data" / "morph-subjects"
spheres = {"small": (2, 100.0, None), "large": (3, 80.0, [0.11, -0.07, 0.05])}
for subject, (grade, radius, rotvec) in spheres.items():
    ico = mne.surface._get_ico_surface(grade)
    rr, tris = ico["rr"], ico["tris"]
    if rotvec is not None:
        rr = Rotation.from_rotvec(rotvec).apply(rr)
    (subjects / subject / "surf").mkdir(parents=True, exist_ok=True)
    for hemi, flip in (("lh", 1.0), ("rh", -1.0)):
        mne.write_surface(subjects / subject / "surf" / f"{hemi}.sphere.reg", rr * radius * [flip, 1.0, 1.0], tris[:, ::-1] if flip < 0 else tris, overwrite=True)
maps = mne.read_morph_map("small", "large", subjects_dir=subjects)
row = maps[0].tocsr()[[0]]
print([m.shape for m in maps], row.indices, row.data, sorted(p.name for p in (subjects / "morph-maps").iterdir()))
