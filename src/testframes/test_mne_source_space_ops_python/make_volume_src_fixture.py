#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the volume source space fixture for test_mne_source_space_ops_python.

mne-python builds a 25 mm volume grid from the sample subject's T1.mgz bounded
by the inner skull of sample-5120-bem.fif and saves it with its neighbourhood,
voxel and MRI transforms and parent-MRI block. The MRI dimensions are set to
32^3 so the (empty) interpolator stays small. The printed values are the oracle.
"""

import os
import sys
from pathlib import Path

import mne
import numpy as np

subjects_dir, bem = sys.argv[1], sys.argv[2]
mne.set_log_level("ERROR")
src = mne.setup_volume_source_space("sample", pos=25.0, mri="T1.mgz", bem=bem, mindist=5.0,
                                    subjects_dir=subjects_dir, add_interpolator=False)
s = src[0]
s["mri_volume_name"] = "T1.mgz"
s["mri_width"] = s["mri_height"] = s["mri_depth"] = 32
out = Path(__file__).parent / "data" / "sample-vol25-src.fif"
src.save(out, overwrite=True)

b = mne.read_source_spaces(out)[0]
nv = b["neighbor_vert"]
used = b["rr"][b["vertno"]]
print("np", b["np"], "nuse", b["nuse"], "shape", tuple(int(x) for x in b["shape"]), "mri", b["mri_width"])
print("neighbours", sum(len(n) for n in nv), int(sum((np.asarray(n) >= 0).sum() for n in nv)),
      int(sum(np.asarray(n)[np.asarray(n) >= 0].sum() for n in nv)))
print("vertno sum", int(b["vertno"].sum()), "rr used sum", repr(float(used.sum())))
print("src_mri_t", b["src_mri_t"]["trans"][:3].tolist())
print("vox_mri_t", b["vox_mri_t"]["trans"][:3].tolist())
print("mri_ras_t", b["mri_ras_t"]["trans"][:3].tolist())
print("size", os.path.getsize(out))
