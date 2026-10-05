# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the 6x5x4 gradient volume used by ex_mri as MGZ and NIfTI (nibabel)."""

from pathlib import Path

import nibabel as nib
import numpy as np

here = Path(__file__).parent
data = np.fromfunction(lambda i, j, k: i + 10 * j + 50 * k, (6, 5, 4)).astype(np.uint8)
affine = np.array([[-1.0, 0, 0, 3.0], [0, 0, 1.5, -2.0], [0, -2.0, 0, 4.0], [0, 0, 0, 1]])
nib.save(nib.MGHImage(data, affine), here / "gradient.mgz")
nib.save(nib.Nifti1Image(data.astype(np.int16), affine), here / "gradient.nii.gz")
