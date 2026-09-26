#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Regenerate cmne_smoke.onnx, the untrained CMNE LSTM used by test_rtcmne_cmne.

The weights are seeded noise: the test checks that InvCMNE loads and runs a
checkpoint with the production tensor layout, not that it improves an estimate.
The model is exported with the same CmneLstm class and export_onnx() as
scripts/ml/training/cmne/train_cmne_lstm.py and saved as one self-contained file.

Requires torch and onnx::

    python3 src/testframes/test_rtcmne_cmne/make_cmne_smoke_model.py
"""

from __future__ import annotations

import importlib.util
import tempfile
from pathlib import Path

import onnx
import torch

N_SOURCES = 16
LOOK_BACK = 4
HIDDEN = 8
SEED = 0

HERE = Path(__file__).resolve().parent
TRAINER = HERE.parents[2] / "scripts" / "ml" / "training" / "cmne" / "train_cmne_lstm.py"
OUTPUT = HERE / "cmne_smoke.onnx"


def main() -> None:
    spec = importlib.util.spec_from_file_location("train_cmne_lstm", TRAINER)
    trainer = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(trainer)

    torch.manual_seed(SEED)
    model = trainer.CmneLstm(n_sources=N_SOURCES, hidden_size=HIDDEN)

    with tempfile.TemporaryDirectory() as tmp:
        exported = Path(tmp) / "model.onnx"
        trainer.export_onnx(model, look_back=LOOK_BACK, n_sources=N_SOURCES, out_path=exported)
        proto = onnx.load(str(exported))

    onnx.save_model(proto, str(OUTPUT), save_as_external_data=False)
    onnx.checker.check_model(str(OUTPUT))
    print(f"wrote {OUTPUT} ({OUTPUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
