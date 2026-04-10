#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Train the CMNE LSTM correction model and export to ONNX.

This script trains a unidirectional LSTM that learns to predict
source-space activation from z-scored rectified dSPM time series
(Dinh et al., 2021).  The trained model is exported as an ONNX file
that can be loaded by MNE-CPP's ``InvCmne::applyLstmCorrection()``.

Usage
-----
    python train_cmne_lstm.py \
        --fwd       sample_audvis-meg-eeg-oct-6-fwd.fif \
        --cov       sample_audvis-cov.fif \
        --epochs    sample_audvis-epo.fif \
        --gt-stc    sample_audvis-gt \
        --look-back 40 \
        --hidden    256 \
        --layers    1 \
        --train-epochs 50 \
        --lr        1e-3 \
        --batch     64 \
        --out       cmne_lstm.onnx

Input data
----------
* ``--fwd``     : Forward solution (``.fif``), used to build the inverse operator.
* ``--cov``     : Noise covariance (``.fif``).
* ``--epochs``  : MNE ``Epochs`` object (``.fif``), the raw sensor-space epochs.
* ``--gt-stc``  : Ground-truth source estimate prefix (read via
                  ``mne.read_source_estimate``).  One STC per epoch, named
                  ``<prefix>-<idx>`` (e.g. ``gt-0-lh.stc``, ``gt-1-lh.stc``…).
                  If omitted, a simulation mode is used where the dSPM
                  estimate itself serves as a pseudo ground truth (useful
                  for testing the pipeline).
* ``--snr``     : SNR for the inverse operator (default 3.0, i.e. lambda2 = 1/9).
* ``--method``  : Inverse method passed to ``apply_inverse_epochs``
                  (default ``dSPM``).

Output
------
An ONNX file with:
    * **input**:  ``X``  — float32 ``[batch, look_back, n_sources]``
    * **output**: ``Y``  — float32 ``[batch, n_sources]``

Copyright (C) 2026, Christoph Dinh.  BSD-3-Clause license.
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
import torch.onnx
from torch.utils.data import DataLoader, TensorDataset

import mne


# ── Model ────────────────────────────────────────────────────────────────────

class CmneLstm(nn.Module):
    """Single-layer unidirectional LSTM for CMNE temporal correction."""

    def __init__(self, n_sources: int, hidden_size: int, num_layers: int = 1):
        super().__init__()
        self.lstm = nn.LSTM(
            input_size=n_sources,
            hidden_size=hidden_size,
            num_layers=num_layers,
            batch_first=True,
        )
        self.fc = nn.Linear(hidden_size, n_sources)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        """
        Parameters
        ----------
        x : Tensor of shape (batch, look_back, n_sources)

        Returns
        -------
        Tensor of shape (batch, n_sources)
        """
        lstm_out, _ = self.lstm(x)
        return self.fc(lstm_out[:, -1, :])


# ── Data loading ─────────────────────────────────────────────────────────────

def z_score_rectify(data: np.ndarray) -> np.ndarray:
    """Z-score rectified source time courses (n_sources x n_times)."""
    abs_data = np.abs(data)
    mu = abs_data.mean(axis=1, keepdims=True)
    sigma = abs_data.std(axis=1, keepdims=True)
    sigma = np.maximum(sigma, 1e-10)
    return (abs_data - mu) / sigma


def build_training_pairs(
    dspm_epochs: list[np.ndarray],
    gt_epochs: list[np.ndarray],
    look_back: int,
) -> tuple[np.ndarray, np.ndarray]:
    """Build (input, target) pairs from dSPM and ground-truth epochs.

    Parameters
    ----------
    dspm_epochs : list of (n_sources, n_times) arrays
        Z-scored rectified dSPM time courses.
    gt_epochs : list of (n_sources, n_times) arrays
        Ground-truth source activations.
    look_back : int
        Number of past time steps.

    Returns
    -------
    inputs  : (N, look_back, n_sources) float32
    targets : (N, n_sources) float32
    """
    inputs, targets = [], []
    for dspm, gt in zip(dspm_epochs, gt_epochs):
        n_sources, n_times = dspm.shape
        for t in range(look_back, n_times):
            inputs.append(dspm[:, t - look_back:t].T)
            targets.append(np.abs(gt[:, t]))

    if not inputs:
        sys.exit("No valid training samples (check look_back vs epoch length).")

    return np.stack(inputs).astype(np.float32), np.stack(targets).astype(np.float32)


def load_fiff_data(
    fwd_path: str,
    cov_path: str,
    epochs_path: str,
    gt_stc_prefix: str | None,
    snr: float,
    method: str,
    look_back: int,
) -> tuple[np.ndarray, np.ndarray]:
    """Load FIFF files via MNE-Python and return training pairs.

    Returns
    -------
    inputs  : (N, look_back, n_sources) float32
    targets : (N, n_sources) float32
    """
    # Read forward, covariance, epochs
    print(f"  Forward solution : {fwd_path}")
    fwd = mne.read_forward_solution(fwd_path, verbose=False)

    print(f"  Noise covariance : {cov_path}")
    cov = mne.read_cov(cov_path, verbose=False)

    print(f"  Epochs           : {epochs_path}")
    epochs = mne.read_epochs(epochs_path, verbose=False, preload=True)

    # Build inverse operator
    lambda2 = 1.0 / snr ** 2
    print(f"  SNR={snr}  lambda2={lambda2}  method={method}")
    inv = mne.minimum_norm.make_inverse_operator(
        epochs.info, fwd, cov, verbose=False
    )

    # Compute dSPM for every epoch
    print("  Computing dSPM source estimates …")
    stcs_dspm = mne.minimum_norm.apply_inverse_epochs(
        epochs, inv, lambda2=lambda2, method=method, verbose=False
    )

    dspm_data = [z_score_rectify(stc.data) for stc in stcs_dspm]
    n_epochs = len(dspm_data)
    n_sources = dspm_data[0].shape[0]
    print(f"  {n_epochs} epochs, {n_sources} sources")

    # Load or simulate ground truth
    if gt_stc_prefix is not None:
        print(f"  Loading ground-truth STCs from {gt_stc_prefix}-<idx> …")
        gt_data = []
        for i in range(n_epochs):
            stc_gt = mne.read_source_estimate(f"{gt_stc_prefix}-{i}", verbose=False)
            gt_data.append(stc_gt.data)
    else:
        print("  No --gt-stc provided → using dSPM as pseudo ground truth (test mode).")
        gt_data = [stc.data for stc in stcs_dspm]

    return build_training_pairs(dspm_data, gt_data, look_back)


# ── Training loop ────────────────────────────────────────────────────────────

def train(
    model: CmneLstm,
    loader: DataLoader,
    epochs: int,
    lr: float,
    device: torch.device,
) -> None:
    optimizer = torch.optim.Adam(model.parameters(), lr=lr)
    criterion = nn.MSELoss()

    model.train()
    for epoch in range(1, epochs + 1):
        total_loss = 0.0
        for X_batch, y_batch in loader:
            X_batch = X_batch.to(device)
            y_batch = y_batch.to(device)

            pred = model(X_batch)
            loss = criterion(pred, y_batch)

            optimizer.zero_grad()
            loss.backward()
            optimizer.step()

            total_loss += loss.item() * X_batch.size(0)

        avg = total_loss / len(loader.dataset)
        if epoch == 1 or epoch % max(1, epochs // 10) == 0:
            print(f"  epoch {epoch:4d}/{epochs}  loss = {avg:.6f}")


# ── ONNX export ─────────────────────────────────────────────────────────────

def export_onnx(
    model: CmneLstm,
    look_back: int,
    n_sources: int,
    out_path: Path,
) -> None:
    model.eval()
    dummy = torch.randn(1, look_back, n_sources)
    torch.onnx.export(
        model,
        dummy,
        str(out_path),
        input_names=["X"],
        output_names=["Y"],
        dynamic_axes={
            "X": {0: "batch"},
            "Y": {0: "batch"},
        },
        opset_version=17,
    )
    print(f"ONNX model exported to {out_path}")


# ── CLI ──────────────────────────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(
        description="Train CMNE LSTM from FIFF files and export to ONNX."
    )
    # Data inputs (FIFF)
    parser.add_argument("--fwd", type=str, required=True,
                        help="Forward solution .fif file.")
    parser.add_argument("--cov", type=str, required=True,
                        help="Noise covariance .fif file.")
    parser.add_argument("--epochs", type=str, required=True,
                        help="MNE Epochs .fif file.")
    parser.add_argument("--gt-stc", type=str, default=None,
                        help="Ground-truth STC prefix (e.g. 'gt' reads "
                             "gt-0, gt-1, …). If omitted, dSPM is used as "
                             "pseudo ground truth (test mode).")
    parser.add_argument("--snr", type=float, default=3.0,
                        help="SNR for inverse operator. Default: 3.0")
    parser.add_argument("--method", type=str, default="dSPM",
                        choices=["dSPM", "MNE", "sLORETA", "eLORETA"],
                        help="Inverse method. Default: dSPM")

    # LSTM hyper-parameters
    parser.add_argument("--look-back", type=int, default=40,
                        help="Number of past time steps (k). Default: 40")
    parser.add_argument("--hidden", type=int, default=256,
                        help="LSTM hidden size. Default: 256")
    parser.add_argument("--layers", type=int, default=1,
                        help="Number of LSTM layers. Default: 1")
    parser.add_argument("--train-epochs", type=int, default=50,
                        help="Training epochs. Default: 50")
    parser.add_argument("--lr", type=float, default=1e-3,
                        help="Learning rate. Default: 1e-3")
    parser.add_argument("--batch", type=int, default=64,
                        help="Batch size. Default: 64")

    # Output
    parser.add_argument("--out", type=Path, default=Path("cmne_lstm.onnx"),
                        help="Output ONNX path. Default: cmne_lstm.onnx")
    args = parser.parse_args()

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Device: {device}")

    # Load data
    print("Loading FIFF data …")
    X_np, y_np = load_fiff_data(
        fwd_path=args.fwd,
        cov_path=args.cov,
        epochs_path=args.epochs,
        gt_stc_prefix=args.gt_stc,
        snr=args.snr,
        method=args.method,
        look_back=args.look_back,
    )
    n_sources = X_np.shape[2]
    print(f"  samples={X_np.shape[0]}  look_back={args.look_back}  n_sources={n_sources}")

    dataset = TensorDataset(torch.from_numpy(X_np), torch.from_numpy(y_np))
    loader = DataLoader(dataset, batch_size=args.batch, shuffle=True)

    # Build model
    model = CmneLstm(n_sources, args.hidden, args.layers).to(device)
    print(f"Model: {sum(p.numel() for p in model.parameters())} parameters")

    # Train
    print("Training …")
    train(model, loader, args.train_epochs, args.lr, device)

    # Export
    model = model.cpu()
    export_onnx(model, args.look_back, n_sources, args.out)


if __name__ == "__main__":
    main()
