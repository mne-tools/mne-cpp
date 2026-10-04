#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the single-precision covariance fixture for test_fiff_stream_python.

MNE-C stores covariances as floats; mne-python only writes doubles, so the
blocks are written with mne-python's low-level FIFF writers: a full noise
covariance (lower triangle, with eigen decomposition) and a diagonal source
covariance. The printed values are what mne-python reads back.
"""

from pathlib import Path

import numpy as np

import mne
from mne._fiff.constants import FIFF
from mne._fiff.open import fiff_open
from mne._fiff.write import end_block, start_and_end_file, start_block, write_double, write_float, write_float_matrix, write_int, write_name_list, write_string
from mne.cov import _read_cov

mne.set_log_level("ERROR")
out = Path(__file__).parent / "data" / "float_cov.fif"
rng = np.random.default_rng(7)
a = rng.standard_normal((3, 3)).astype(np.float32)
full = (a @ a.T + np.eye(3, dtype=np.float32)).astype(np.float32)
eig, vec = np.linalg.eigh(full.astype(np.float64))
with start_and_end_file(out) as fid:
    start_block(fid, FIFF.FIFFB_MNE_COV)
    write_int(fid, FIFF.FIFF_MNE_COV_KIND, FIFF.FIFFV_MNE_NOISE_COV)
    write_int(fid, FIFF.FIFF_MNE_COV_DIM, 3)
    write_int(fid, FIFF.FIFF_MNE_COV_NFREE, 42)
    write_name_list(fid, FIFF.FIFF_MNE_ROW_NAMES, ["A", "B", "C"])
    write_float(fid, FIFF.FIFF_MNE_COV, full[np.tril_indices(3)])
    write_float_matrix(fid, FIFF.FIFF_MNE_COV_EIGENVECTORS, vec.T.astype(np.float32))
    write_double(fid, FIFF.FIFF_MNE_COV_EIGENVALUES, eig)
    # MNE-C names projection items with FIFF_DESCRIPTION rather than FIFF_NAME.
    start_block(fid, FIFF.FIFFB_PROJ)
    start_block(fid, FIFF.FIFFB_PROJ_ITEM)
    write_string(fid, FIFF.FIFF_DESCRIPTION, "ECG-1")
    write_int(fid, FIFF.FIFF_PROJ_ITEM_KIND, FIFF.FIFFV_PROJ_ITEM_FIELD)
    write_int(fid, FIFF.FIFF_NCHAN, 3)
    write_int(fid, FIFF.FIFF_PROJ_ITEM_NVEC, 1)
    write_name_list(fid, FIFF.FIFF_PROJ_ITEM_CH_NAME_LIST, ["A", "B", "C"])
    write_float_matrix(fid, FIFF.FIFF_PROJ_ITEM_VECTORS, np.array([[0.0, 0.6, 0.8]], np.float32))
    end_block(fid, FIFF.FIFFB_PROJ_ITEM)
    end_block(fid, FIFF.FIFFB_PROJ)
    start_block(fid, FIFF.FIFFB_MNE_BAD_CHANNELS)
    write_name_list(fid, FIFF.FIFF_MNE_CH_NAME_LIST, ["B"])
    end_block(fid, FIFF.FIFFB_MNE_BAD_CHANNELS)
    end_block(fid, FIFF.FIFFB_MNE_COV)
    start_block(fid, FIFF.FIFFB_MNE_COV)
    write_int(fid, FIFF.FIFF_MNE_COV_KIND, FIFF.FIFFV_MNE_SOURCE_COV)
    write_int(fid, FIFF.FIFF_MNE_COV_DIM, 4)
    write_float(fid, FIFF.FIFF_MNE_COV_DIAG, np.array([1.5, 2.5, 3.5, 4.5], np.float32))
    end_block(fid, FIFF.FIFFB_MNE_COV)

np.set_printoptions(precision=9)
fid, tree, _ = fiff_open(out)
with fid:
    for kind in (FIFF.FIFFV_MNE_NOISE_COV, FIFF.FIFFV_MNE_SOURCE_COV):
        c = _read_cov(fid, tree, kind)
        print(kind, c["dim"], c["diag"], c["nfree"], c["names"], c["data"].ravel(), c["eig"], [(p["desc"], p["kind"], p["data"]["data"]) for p in c["projs"]], c["bads"])
