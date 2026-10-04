#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write the measurement-info fixture for test_fiff_stream_python.

A two-channel FIFFB_MEAS_INFO with the optional parts read_meas_info handles:
a parent block id, UTC offset, float gantry angle, acquisition parameters and
stimulus strings, an Isotrak block in MRI coordinates with its own transform,
and the CTF head transform only inside the HPI result block. The printed
values are what mne-python reads back.
"""

from pathlib import Path

import numpy as np

import mne
from mne._fiff.constants import FIFF
from mne._fiff.write import end_block, start_and_end_file, start_block, write_ch_info, write_coord_trans, write_dig_points, write_float, write_id, write_int, write_string
from mne.transforms import Transform

mne.set_log_level("ERROR")
out = Path(__file__).parent / "data" / "meas_info.fif"
parent_id = dict(version=65540, machid=np.array([11, 22], np.int32), secs=1700000000, usecs=250000)
dev_head = Transform("meg", "head", np.array([[1, 0, 0, 0.01], [0, 0, -1, 0.02], [0, 1, 0, 0.03], [0, 0, 0, 1.0]]))
ctf_head = Transform("ctf_head", "head", np.array([[0, -1, 0, 0.004], [1, 0, 0, -0.002], [0, 0, 1, 0.0], [0, 0, 0, 1.0]]))
dig_trans = Transform("mri", "head", np.array([[1, 0, 0, 0.0], [0, 1, 0, 0.0], [0, 0, 1, 0.04], [0, 0, 0, 1.0]]))
dig = [dict(kind=FIFF.FIFFV_POINT_CARDINAL, ident=k + 1, r=np.array(r, float)) for k, r in enumerate([[-0.07, 0, 0], [0, 0.1, 0], [0.07, 0, 0]])]

with start_and_end_file(out) as fid:
    start_block(fid, FIFF.FIFFB_MEAS)
    start_block(fid, FIFF.FIFFB_MEAS_INFO)
    write_id(fid, FIFF.FIFF_PARENT_BLOCK_ID, parent_id)
    write_int(fid, FIFF.FIFF_NCHAN, 2)
    write_float(fid, FIFF.FIFF_SFREQ, 600.0)
    write_string(fid, FIFF.FIFF_UTC_OFFSET, "+0100")
    write_float(fid, FIFF.FIFF_GANTRY_ANGLE, 68.0)
    write_coord_trans(fid, dev_head)
    for k, name in enumerate(["EEG 001", "STI 014"]):
        loc = np.zeros(12)
        write_ch_info(fid, dict(scanno=k + 1, logno=k + 1, kind=(FIFF.FIFFV_EEG_CH, FIFF.FIFFV_STIM_CH)[k], range=1.0, cal=1.0, coil_type=FIFF.FIFFV_COIL_EEG if k == 0 else FIFF.FIFFV_COIL_NONE, loc=loc, unit=FIFF.FIFF_UNIT_V, unit_mul=0, ch_name=name))
    start_block(fid, FIFF.FIFFB_HPI_RESULT)
    write_coord_trans(fid, ctf_head)
    end_block(fid, FIFF.FIFFB_HPI_RESULT)
    start_block(fid, FIFF.FIFFB_ISOTRAK)
    write_int(fid, FIFF.FIFF_MNE_COORD_FRAME, FIFF.FIFFV_COORD_MRI)
    write_coord_trans(fid, dig_trans)
    write_dig_points(fid, dig)
    end_block(fid, FIFF.FIFFB_ISOTRAK)
    start_block(fid, FIFF.FIFFB_DACQ_PARS)
    write_string(fid, FIFF.FIFF_DACQ_PARS, "pars")
    write_string(fid, FIFF.FIFF_DACQ_STIM, "stim")
    end_block(fid, FIFF.FIFFB_DACQ_PARS)
    end_block(fid, FIFF.FIFFB_MEAS_INFO)
    end_block(fid, FIFF.FIFFB_MEAS)

info = mne.io.read_info(out)
np.set_printoptions(precision=9)
print(info["meas_id"], info["meas_date"], info["utc_offset"], info["gantry_angle"], info["acq_pars"], info["acq_stim"])
print(info["ctf_head_t"], info["dev_ctf_t"]["trans"])
print([(d["coord_frame"], d["r"]) for d in info["dig"]])
