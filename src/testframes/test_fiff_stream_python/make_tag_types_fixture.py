#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the tag-type fixture for test_fiff_stream_python.

One tag per payload type that read_tag has to byte-swap, written by mne-python:
double / short / ushort / julian / dau_pack16 / complex arrays, dense double and
complex-float matrices, and CSR / CSC sparse float matrices. Tag kinds are
arbitrary small numbers; the test looks them up by kind.
"""

import datetime
from pathlib import Path

import numpy as np
from scipy.sparse import csc_array, csr_array

from mne._fiff.constants import FIFF
from mne._fiff.write import _write, start_and_end_file, write_complex64, write_complex_float_matrix, write_dau_pack16, write_double, write_double_matrix, write_float_sparse, write_julian

out = Path(__file__).parent / "data" / "tag_types.fif"
sparse = np.array([[1.0, 0, 2.0, 0], [0, 0, 3.0, 4.0], [5.0, 0, 0, 6.0], [0, 7.0, 0, 0]], np.float32)
with start_and_end_file(out) as fid:
    write_double(fid, 901, np.array([1.25, -2.5e-12, 3.0e200]))
    _write(fid, np.array([-3, 300, -32000], np.int16), 902, 2, FIFF.FIFFT_SHORT, ">i2")
    _write(fid, np.array([1, 40000, 65535], np.uint16), 903, 2, FIFF.FIFFT_USHORT, ">u2")
    write_julian(fid, 904, datetime.date(2026, 10, 4))
    write_dau_pack16(fid, 905, np.array([-7, 12345], np.int16))
    write_complex64(fid, 906, np.array([1.5 - 2.0j, -0.25 + 4.0j], np.complex64))
    write_double_matrix(fid, 907, np.array([[1.0, 2.0, 3.0], [4.0, 5.0, 6.5]]))
    write_complex_float_matrix(fid, 908, np.array([[1 + 2j, 3 - 4j]], np.complex64))
    write_float_sparse(fid, 909, csr_array(sparse), fmt="csr")
    write_float_sparse(fid, 910, csc_array(sparse), fmt="csc")
print(out.stat().st_size, datetime.date(2026, 10, 4).toordinal() + 1721425)
