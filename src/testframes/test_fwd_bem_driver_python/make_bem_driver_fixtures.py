#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the fixtures for test_fwd_bem_driver_python.

A three-layer BEM of concentric ico-1 spheres (radii 80/85/90 mm, linear
collocation) is solved and written by mne-python. Four point magnetometers,
four EEG electrodes and five dipoles in head coordinates (identity head <-> MRI)
give mne-python's free-orientation gain matrix, saved in gain.txt with one row
per channel (MEG first) and three columns per source.
"""

from pathlib import Path

import numpy as np

import mne
from mne.io.constants import FIFF
from mne.bem import _surfaces_to_bem
from mne.surface import _get_ico_surface

mne.set_log_level("ERROR")
out = Path(__file__).parent / "data"
out.mkdir(exist_ok=True)

ico = _get_ico_surface(1)
layers = [(90.0, FIFF.FIFFV_BEM_SURF_ID_HEAD, 0.3), (85.0, FIFF.FIFFV_BEM_SURF_ID_SKULL, 0.006), (80.0, FIFF.FIFFV_BEM_SURF_ID_BRAIN, 0.3)]
surfs = _surfaces_to_bem([dict(rr=ico["rr"] * r, tris=ico["tris"].copy()) for r, _, _ in layers], [i for _, i, _ in layers], [s for _, _, s in layers])
bem = mne.make_bem_solution(surfs)
mne.write_bem_solution(out / "sphere3-bem-sol.fif", bem, overwrite=True)

meg_pos = np.array([[0.0, 0.0, 0.11], [0.08, 0.0, 0.07], [0.0, -0.09, 0.06], [-0.06, 0.06, 0.07]])
meg_dir = np.array([[0.0, 0.0, 1.0], [0.6, 0.0, 0.8], [0.0, -0.8, 0.6], [-0.3, 0.5, 0.812404]])
eeg_pos = np.array([[0.0, 0.0, 0.09], [0.09, 0.0, 0.0], [0.0, 0.09, 0.0], [-0.0636, -0.0636, 0.0]])
names = [f"MEG{k}" for k in range(4)] + [f"EEG{k}" for k in range(4)]
info = mne.create_info(names, 1000.0, ["mag"] * 4 + ["eeg"] * 4)
for k, ch in enumerate(info["chs"]):
    ch["coord_frame"] = FIFF.FIFFV_COORD_HEAD if k >= 4 else FIFF.FIFFV_COORD_DEVICE
    if k < 4:
        ez = meg_dir[k] / np.linalg.norm(meg_dir[k])
        ex = np.cross([0.0, 1.0, 0.0] if abs(ez[1]) < 0.9 else [1.0, 0.0, 0.0], ez)
        ex /= np.linalg.norm(ex)
        ch["coil_type"] = FIFF.FIFFV_COIL_POINT_MAGNETOMETER
        ch["loc"][:12] = np.concatenate([meg_pos[k], ex, np.cross(ez, ex), ez])
    else:
        ch["loc"][:3] = eeg_pos[k - 4]
with info._unlock():
    info["dev_head_t"] = mne.transforms.Transform("meg", "head")

src_rr = np.array([[0.0, 0.0, 0.05], [0.03, 0.01, 0.04], [-0.02, 0.03, 0.02], [0.01, -0.04, 0.03], [0.0, 0.02, -0.03]])
src_nn = np.tile([0.0, 0.0, 1.0], (len(src_rr), 1))
src = mne.setup_volume_source_space(pos=dict(rr=src_rr, nn=src_nn))
fwd = mne.make_forward_solution(info, mne.transforms.Transform("head", "mri"), src, bem, mindist=0.0)
gain = fwd["sol"]["data"]
np.savetxt(out / "gain.txt", gain, fmt="%.9e")
np.savetxt(out / "geometry.txt", np.vstack([meg_pos, meg_dir / np.linalg.norm(meg_dir, axis=1, keepdims=True), eeg_pos, src_rr]), fmt="%.9e")
print(gain.shape, fwd["src"][0]["rr"][fwd["src"][0]["vertno"]])
