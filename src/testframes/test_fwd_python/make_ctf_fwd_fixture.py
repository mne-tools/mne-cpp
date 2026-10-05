#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the two-dipole source space and the mne-python gain oracle for test_fwd_python's CTF forward.

Dipoles at (30, 20, 70) and (-20, 10, 60) mm (MRI = head coordinates); the gain matrix
and its position derivatives (central differences) are mne-python's make_forward_solution
on the CTF fixtures of test_mne_ctf_comp_python
with a sphere at (0, 0, 40) mm, at compensation grade 3 and grade 0.
"""

from pathlib import Path

import mne
import numpy as np

here = Path(__file__).parent
pos = np.array([[0.03, 0.02, 0.07], [-0.02, 0.01, 0.06]])
src = mne.setup_volume_source_space(pos=dict(rr=pos, nn=np.tile([0.0, 0.0, 1.0], (2, 1))), verbose=False)
(here / "data").mkdir(exist_ok=True)
src.save(here / "data" / "two-dipole-src.fif", overwrite=True, verbose=False)
sphere = mne.make_sphere_model(r0=(0, 0, 0.04), head_radius=None, verbose=False)
for grade in (3, 0):
    raw = here.parent / "test_mne_ctf_comp_python" / "data" / f"ctf_grade{grade}_raw.fif"
    info = mne.io.read_raw_fif(raw, verbose=False).info
    fwd = mne.make_forward_solution(info, None, src, sphere, meg=True, eeg=False, mindist=0, verbose=False)
    np.savetxt(here / "data" / f"ctf_grade{grade}_gain.txt", fwd["sol"]["data"], fmt="%.17g")
    # Position derivatives by central differences (h = 0.1 mm), ordered source, component, axis.
    h = 1e-4
    grad = np.zeros((fwd["sol"]["data"].shape[0], 18))
    for s in range(2):
        for d in range(3):
            g = []
            for sign in (1, -1):
                p = pos.copy()
                p[s, d] += sign * h
                sp = mne.setup_volume_source_space(pos=dict(rr=p, nn=np.tile([0.0, 0.0, 1.0], (2, 1))), verbose=False)
                g.append(mne.make_forward_solution(info, None, sp, sphere, meg=True, eeg=False, mindist=0, verbose=False)["sol"]["data"])
            diff = (g[0] - g[1]) / (2 * h)
            for c in range(3):
                grad[:, 9 * s + 3 * c + d] = diff[:, 3 * s + c]
    np.savetxt(here / "data" / f"ctf_grade{grade}_grad.txt", grad, fmt="%.17g")

# One-layer BEM (ico-3 sphere of radius 90 mm at (0, 0, 40) mm, inner skull, sigma 0.3) and the
# compensated BEM gains; the -sol file is not stored, MNE-CPP computes the solution itself.
from mne.bem import _surfaces_to_bem  # noqa: E402
from mne.surface import _get_ico_surface  # noqa: E402

ico = _get_ico_surface(3)
surf = dict(rr=ico["rr"] * 90 + [0, 0, 40], tris=ico["tris"], id=1, coord_frame=5, np=len(ico["rr"]), ntri=len(ico["tris"]))
bem_surfs = _surfaces_to_bem([surf], [1], [0.3], incomplete="ignore")
mne.write_bem_surfaces(here / "data" / "one-layer-bem.fif", bem_surfs, overwrite=True, verbose=False)
bem = mne.make_bem_solution(bem_surfs, verbose=False)
for grade in (3, 0):
    raw = here.parent / "test_mne_ctf_comp_python" / "data" / f"ctf_grade{grade}_raw.fif"
    info = mne.io.read_raw_fif(raw, verbose=False).info
    fwd = mne.make_forward_solution(info, None, src, bem, meg=True, eeg=False, mindist=0, verbose=False)
    np.savetxt(here / "data" / f"ctf_grade{grade}_bem_gain.txt", fwd["sol"]["data"], fmt="%.17g")
