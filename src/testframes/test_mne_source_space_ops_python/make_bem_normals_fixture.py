#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the BEM normals fixture for test_mne_source_space_ops_python.

Two 162-vertex icosahedra (grade 2) of radius 80 and 90 mm, shifted so that
no normal is purely radial: the inner skull without stored vertex normals and
the outer skull with every stored normal set to +z. mne-python computes the
normals from the triangles when none are stored, and with patch_stats=True
also replaces stored ones. The values printed at the end are the oracle the
test asserts.
"""

from pathlib import Path

import mne
import numpy as np
from mne.io.constants import FIFF
from mne.surface import _get_ico_surface

out = Path(__file__).parent / "data" / "ico2-bem.fif"
surfs = []
for sid, radius in ((FIFF.FIFFV_BEM_SURF_ID_BRAIN, 0.08), (FIFF.FIFFV_BEM_SURF_ID_SKULL, 0.09)):
    ico = _get_ico_surface(2)
    rr = ico["rr"] * radius * np.array([1.0, 0.9, 1.1]) + [0.001, -0.002, 0.04]
    surf = dict(id=sid, sigma=0.3, np=len(rr), ntri=len(ico["tris"]), coord_frame=FIFF.FIFFV_COORD_MRI,
                rr=rr, tris=ico["tris"])
    if sid == FIFF.FIFFV_BEM_SURF_ID_SKULL:
        surf["nn"] = np.tile([0.0, 0.0, 1.0], (len(rr), 1))
    surfs.append(surf)
mne.write_bem_surfaces(out, surfs, overwrite=True)

for patch_stats in (False, True):
    for s in mne.read_bem_surfaces(out, patch_stats=patch_stats, verbose=False):
        print(patch_stats, s["id"], repr(float(np.abs(s["nn"]).sum())), repr(s["nn"][17].tolist()))
