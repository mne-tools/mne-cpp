#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Write a small cmne model and cmne's outputs as the oracle for test_inv_cmne.

Needs ``pip install "cmne[onnx]==0.2.1"``. An untrained 12-source network
(look_back 6, 16 units, seed 0) is exported with ``cmne.export_onnx``, which
stores the configuration in the ``cmne_config`` metadata. A signed 12 x 30
source estimate (seed 1) is passed through ``cmne.apply_cmne`` with
``cmne.OnnxPredictor`` and ``cmne.control_estimate``; all inputs and outputs are
saved as text so the C++ test needs neither Python nor NumPy.
"""

from pathlib import Path

import numpy as np

import cmne

here = Path(__file__).parent
model = cmne.CMNEModel.create(n_sources=12, look_back=6, num_units=16, seed=0)
onnx_path = cmne.export_onnx(model, here / "cmne_ref.onnx")
predictor = cmne.OnnxPredictor(onnx_path)

source = np.random.default_rng(1).standard_normal((12, 30))
result = cmne.apply_cmne(source, predictor)
for name, data in (
    ("source", source),
    ("sensing", result.sensing),
    ("prediction", result.prediction),
    ("cmne", result.cmne),
    ("control", cmne.control_estimate(source, look_back=6)),
):
    np.savetxt(here / f"cmne_ref_{name}.txt", data, fmt="%.9g")
print(f"cmne {cmne.__version__}: wrote {onnx_path}")
