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
import hashlib
import os
import sys
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
import torch.onnx
from torch.utils.data import DataLoader, Dataset

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


class CmneDataset(Dataset):
    """Lazy sliding-window dataset — avoids materializing all pairs in RAM.

    Parameters
    ----------
    dspm_epochs : list of (n_sources, n_times) float32 arrays
    gt_epochs   : list of (n_sources, n_times) float32 arrays
    look_back   : int
    """

    def __init__(
        self,
        dspm_epochs: list[np.ndarray],
        gt_epochs: list[np.ndarray],
        look_back: int,
    ):
        self.dspm = dspm_epochs
        self.gt = gt_epochs
        self.look_back = look_back
        # Pre-compute a flat index → (epoch_idx, time_step) mapping.
        self._index: list[tuple[int, int]] = []
        for ei, d in enumerate(dspm_epochs):
            n_times = d.shape[1]
            for t in range(look_back, n_times):
                self._index.append((ei, t))
        if not self._index:
            sys.exit("No valid training samples (check look_back vs epoch length).")

    # ------------------------------------------------------------------
    def __len__(self) -> int:
        return len(self._index)

    def __getitem__(self, idx: int):
        ei, t = self._index[idx]
        x = self.dspm[ei][:, t - self.look_back : t].T   # (look_back, n_sources)
        y = np.abs(self.gt[ei][:, t])                     # (n_sources,)
        return (
            torch.from_numpy(np.ascontiguousarray(x)),
            torch.from_numpy(np.ascontiguousarray(y)),
        )


def _cache_key(
    fwd_path: str, cov_path: str, epochs_path: str,
    gt_stc_prefix: str | None, snr: float, method: str,
) -> str:
    """Deterministic hash from input files (content) + parameters."""
    h = hashlib.sha256()
    for path in (fwd_path, cov_path, epochs_path):
        h.update(path.encode())
        h.update(str(os.path.getmtime(path)).encode())
        h.update(str(os.path.getsize(path)).encode())
    h.update(f"snr={snr},method={method},gt={gt_stc_prefix}".encode())
    return h.hexdigest()[:16]


def load_fiff_data(
    fwd_path: str,
    cov_path: str,
    epochs_path: str,
    gt_stc_prefix: str | None,
    snr: float,
    method: str,
    cache_dir: str | None = None,
) -> tuple[list[np.ndarray], list[np.ndarray]]:
    """Load FIFF files via MNE-Python and return per-epoch source data.

    If *cache_dir* is given the z-scored dSPM arrays (and ground-truth
    arrays) are saved as a single ``.npz`` file so that subsequent runs
    skip the expensive ``apply_inverse_epochs`` step entirely.

    Returns
    -------
    dspm_data : list of (n_sources, n_times) float32 arrays
    gt_data   : list of (n_sources, n_times) float32 arrays
    """
    # ── Try loading from cache ──────────────────────────────────────────
    cache_path: Path | None = None
    if cache_dir is not None:
        key = _cache_key(fwd_path, cov_path, epochs_path,
                         gt_stc_prefix, snr, method)
        cache_path = Path(cache_dir) / f"dspm_cache_{key}.npz"
        if cache_path.exists():
            print(f"  Loading cached source estimates: {cache_path}")
            with np.load(cache_path) as f:
                n = int(f["n_epochs"])
                dspm_data = [f[f"dspm_{i}"] for i in range(n)]
                gt_data   = [f[f"gt_{i}"] for i in range(n)]
            print(f"  {n} epochs, {dspm_data[0].shape[0]} sources (from cache)")
            return dspm_data, gt_data

    # ── Compute from scratch ────────────────────────────────────────────
    print(f"  Forward solution : {fwd_path}")
    fwd = mne.read_forward_solution(fwd_path, verbose=False)

    print(f"  Noise covariance : {cov_path}")
    cov = mne.read_cov(cov_path, verbose=False)

    print(f"  Epochs           : {epochs_path}")
    epochs = mne.read_epochs(epochs_path, verbose=False, preload=True)

    # Ensure EEG average reference projection is set (required by MNE inverse)
    if not any(p["desc"] == "Average EEG reference" for p in epochs.info["projs"]):
        epochs.set_eeg_reference(projection=True, verbose=False)

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

    dspm_data = [z_score_rectify(stc.data).astype(np.float32) for stc in stcs_dspm]
    n_epochs = len(dspm_data)
    n_sources = dspm_data[0].shape[0]
    print(f"  {n_epochs} epochs, {n_sources} sources")

    # Load or simulate ground truth
    if gt_stc_prefix is not None:
        print(f"  Loading ground-truth STCs from {gt_stc_prefix}-<idx> …")
        gt_data = []
        for i in range(n_epochs):
            stc_gt = mne.read_source_estimate(f"{gt_stc_prefix}-{i}", verbose=False)
            gt_data.append(stc_gt.data.astype(np.float32))
    else:
        print("  No --gt-stc provided → using dSPM as pseudo ground truth (test mode).")
        gt_data = [stc.data.astype(np.float32) for stc in stcs_dspm]

    # ── Save to cache ───────────────────────────────────────────────────
    if cache_path is not None:
        arrays: dict[str, np.ndarray] = {"n_epochs": np.array(n_epochs)}
        for i, (d, g) in enumerate(zip(dspm_data, gt_data)):
            arrays[f"dspm_{i}"] = d
            arrays[f"gt_{i}"] = g
        np.savez(cache_path, **arrays)
        size_mb = cache_path.stat().st_size / (1024 * 1024)
        print(f"  Cached source estimates: {cache_path} ({size_mb:.1f} MB)")

    return dspm_data, gt_data


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

    n_batches = len(loader)
    # Print batch progress every ~10% of batches (at least every 50 batches)
    batch_log_interval = max(1, min(50, n_batches // 10))

    model.train()
    for epoch in range(1, epochs + 1):
        total_loss = 0.0
        for bi, (X_batch, y_batch) in enumerate(loader, 1):
            X_batch = X_batch.to(device)
            y_batch = y_batch.to(device)

            pred = model(X_batch)
            loss = criterion(pred, y_batch)

            optimizer.zero_grad()
            loss.backward()
            optimizer.step()

            total_loss += loss.item() * X_batch.size(0)

            if bi % batch_log_interval == 0 or bi == n_batches:
                batch_pct = 100.0 * bi / n_batches
                epoch_pct = 100.0 * ((epoch - 1) + bi / n_batches) / epochs
                running_avg = total_loss / (bi * loader.batch_size)
                print(
                    f"[progress] {epoch_pct:.1f}% "
                    f"Epoch {epoch}/{epochs}  "
                    f"batch {bi}/{n_batches} ({batch_pct:.0f}%)  "
                    f"loss = {running_avg:.6f}"
                )


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

    # Fine-tuning
    parser.add_argument("--finetune", type=str, default=None,
                        help="Path to existing ONNX model for fine-tuning. "
                             "Loads the corresponding .pt checkpoint.")

    # Output
    parser.add_argument("--out", type=Path, default=Path("cmne_lstm.onnx"),
                        help="Output ONNX path. Default: cmne_lstm.onnx")
    args = parser.parse_args()

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"Device: {device}")

    # Load data (cached in output directory to skip recomputation)
    cache_dir = str(args.out.parent)
    print("Loading FIFF data …")
    dspm_data, gt_data = load_fiff_data(
        fwd_path=args.fwd,
        cov_path=args.cov,
        epochs_path=args.epochs,
        gt_stc_prefix=args.gt_stc,
        snr=args.snr,
        method=args.method,
        cache_dir=cache_dir,
    )
    n_sources = dspm_data[0].shape[0]

    dataset = CmneDataset(dspm_data, gt_data, args.look_back)
    print(f"  samples={len(dataset)}  look_back={args.look_back}  n_sources={n_sources}")

    loader = DataLoader(dataset, batch_size=args.batch, shuffle=True,
                        num_workers=0, pin_memory=(device.type != "cpu"))

    # Build model
    model = CmneLstm(n_sources, args.hidden, args.layers).to(device)

    # Fine-tune: load checkpoint from a previous training run
    if args.finetune is not None:
        pt_path = Path(args.finetune).with_suffix(".pt")
        if not pt_path.exists():
            sys.exit(
                f"Fine-tune checkpoint not found: {pt_path}\n"
                f"The .pt checkpoint is saved alongside the ONNX model during training."
            )
        ckpt = torch.load(pt_path, map_location=device, weights_only=False)
        # Rebuild model with checkpoint architecture if dimensions differ
        ckpt_n_src = ckpt.get("n_sources", n_sources)
        ckpt_hidden = ckpt.get("hidden_size", args.hidden)
        ckpt_layers = ckpt.get("num_layers", args.layers)
        if ckpt_n_src != n_sources:
            sys.exit(
                f"Source dimension mismatch: checkpoint has {ckpt_n_src} sources, "
                f"but current data has {n_sources} sources."
            )
        model = CmneLstm(ckpt_n_src, ckpt_hidden, ckpt_layers).to(device)
        model.load_state_dict(ckpt["state_dict"])
        print(f"Loaded checkpoint for fine-tuning: {pt_path}")
        print(f"  architecture: hidden={ckpt_hidden}  layers={ckpt_layers}")

    print(f"Model: {sum(p.numel() for p in model.parameters())} parameters")

    # Train
    print("Training …")
    train(model, loader, args.train_epochs, args.lr, device)

    # Save PyTorch checkpoint (for future fine-tuning)
    pt_out = args.out.with_suffix(".pt")
    torch.save({
        "state_dict": model.state_dict(),
        "n_sources": n_sources,
        "hidden_size": args.hidden,
        "num_layers": args.layers,
    }, pt_out)
    print(f"PyTorch checkpoint saved to {pt_out}")

    # Export ONNX
    model = model.cpu()
    export_onnx(model, args.look_back, n_sources, args.out)


if __name__ == "__main__":
    main()
