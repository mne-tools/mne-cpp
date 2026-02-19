#!/usr/bin/env python3
"""Generate reference field-map data replicating the MNE helmet example.

Replicates https://mne.tools/stable/auto_examples/visualization/mne_helmet.html
and exports intermediate + final data for cross-validation with mne-cpp.

The example uses:
  - mne.make_field_map(evoked, ch_type="meg", origin="auto", upsampling=2)
  - evoked.plot_field(maps, time=0.083)

Outputs (written to --outdir):
    helmet_verts.npy        (n_verts, 3)  helmet surface vertices (head coords)
    helmet_norms.npy        (n_verts, 3)  helmet surface normals
    origin.npy              (3,) sphere origin (head coords)
    meg_ch_names.txt        channel names, one per line
    meg_mapping.npy         (n_verts, n_meg_ch) mapping matrix
    meg_mapped_t083.npy     (n_verts,) mapped field at t=0.083
    meg_data_t083.npy       (n_meg_ch,) channel data at t=0.083
    meg_mapped_all.npy      (n_verts, n_times) mapped at all times
    meg_data_all.npy        (n_meg_ch, n_times) channel data for all times
    info.txt                summary information

Copyright (C) 2026, Christoph Dinh. All rights reserved.
BSD-3-Clause license.
"""

import argparse
import os
import sys

import numpy as np
import mne
from mne.cov import make_ad_hoc_cov
from mne.forward._field_interpolation import (
    _compute_mapping_matrix,
    _setup_dots,
)
from mne.forward._lead_dots import _do_self_dots, _do_surface_dots
from mne.forward._make_forward import _create_meg_coils
from mne.surface import get_meg_helmet_surf


def main():
    parser = argparse.ArgumentParser(
        description="Generate helmet field-map reference (MNE example)."
    )
    parser.add_argument("--outdir", required=True, help="Output directory")
    parser.add_argument("--sample-dir", default=None,
                        help="MNE sample data directory (auto-detected if omitted)")
    parser.add_argument("--mode", default="accurate",
                        choices=["fast", "accurate"],
                        help="Legendre accuracy mode (default: accurate)")
    args = parser.parse_args()

    os.makedirs(args.outdir, exist_ok=True)

    # ── Locate sample data ──────────────────────────────────────────────
    if args.sample_dir:
        sample_path = args.sample_dir
    else:
        sample_path = str(mne.datasets.sample.data_path())

    subjects_dir = os.path.join(sample_path, "subjects")
    fname_evoked = os.path.join(sample_path, "MEG", "sample",
                                "sample_audvis-ave.fif")
    fname_trans = os.path.join(sample_path, "MEG", "sample",
                               "sample_audvis_raw-trans.fif")

    print(f"[helmet-ref] Sample data: {sample_path}")
    print(f"[helmet-ref] Evoked: {fname_evoked}")
    print(f"[helmet-ref] Trans:  {fname_trans}")

    # ── Load evoked (matching the example exactly) ──────────────────────
    evoked = mne.read_evokeds(
        fname_evoked, baseline=(None, 0), proj=True,
        verbose=False, condition="Left Auditory"
    )
    print(f"[helmet-ref] Evoked: {evoked.comment}, "
          f"{evoked.data.shape[0]} ch x {evoked.data.shape[1]} times, "
          f"nave={evoked.nave}")

    # ── Create field map using raw primitives (matching C++ pipeline) ───
    # Use get_meg_helmet_surf to get the upsampled helmet just like
    # make_field_map does, but then compute the mapping ourselves with
    # the 'accurate' mode so it matches the C++ Legendre evaluation.
    trans = mne.read_trans(fname_trans, verbose=False)
    try:
        print("[helmet-ref] Getting MEG helmet surface (upsampling=2)...")
        surf = get_meg_helmet_surf(evoked.info, trans=trans, upsampling=2,
                                   verbose=True)
    except RuntimeError as e:
        if "pyvista" in str(e).lower():
            print(f"[helmet-ref] pyvista not available, using upsampling=1")
            surf = get_meg_helmet_surf(evoked.info, trans=trans,
                                       verbose=True)
        else:
            raise
    print(f"[helmet-ref] Helmet surface: {surf['np']} vertices")

    # Auto-fit sphere origin (matching make_field_map(origin='auto'))
    from mne.bem import fit_sphere_to_headshape
    R, origin, _ = fit_sphere_to_headshape(evoked.info, verbose=False)
    origin = np.array(origin, dtype=np.float64)
    print(f"[helmet-ref] Auto-fit origin: {origin} (radius={R*1000:.1f} mm)")

    # Pick good MEG channels
    info = evoked.info
    meg_picks = mne.pick_types(info, meg=True, eeg=False, exclude='bads')
    meg_info = mne.pick_info(info, meg_picks, copy=True)
    ch_names = list(meg_info["ch_names"])
    print(f"[helmet-ref] MEG channels: {len(ch_names)}")

    # Create MEG coils and compute dots
    meg_coils = _create_meg_coils(meg_info["chs"], "normal")
    int_rad, noise, lut_fun, n_fact = _setup_dots(
        args.mode, meg_info, meg_coils, "meg"
    )
    print(f"[helmet-ref] Mode: {args.mode}, int_rad: {int_rad}")

    # Self-dot matrix
    self_dots = _do_self_dots(
        int_rad, False, meg_coils, origin, "meg",
        lut_fun, n_fact, n_jobs=1
    )
    print(f"[helmet-ref] Self-dots: {self_dots.shape}")

    # Surface-dot matrix
    sel = np.arange(surf["np"])
    surface_dots = _do_surface_dots(
        int_rad, False, meg_coils, surf, sel, origin, "meg",
        lut_fun, n_fact, n_jobs=1
    )
    print(f"[helmet-ref] Surface-dots: {surface_dots.shape}")

    # Compute mapping matrix
    miss_meg = 1e-4
    fmd = dict(
        kind="meg",
        ch_names=ch_names,
        origin=origin,
        noise=noise,
        self_dots=self_dots.copy(),
        surface_dots=surface_dots.copy(),
        int_rad=int_rad,
        miss=miss_meg,
    )
    mapping = _compute_mapping_matrix(fmd, meg_info)
    print(f"[helmet-ref] Mapping: {mapping.shape}")

    # Get surface data
    verts = surf["rr"].astype(np.float64)
    norms = surf["nn"].astype(np.float64)
    np.save(os.path.join(args.outdir, "helmet_verts.npy"), verts)
    np.save(os.path.join(args.outdir, "helmet_norms.npy"), norms)
    print(f"[helmet-ref] Helmet surface: {verts.shape[0]} vertices")

    # ── Export origin ───────────────────────────────────────────────────
    np.save(os.path.join(args.outdir, "origin.npy"), origin)
    print(f"[helmet-ref] Origin: {origin}")

    # ── Export channel names ────────────────────────────────────────────
    with open(os.path.join(args.outdir, "meg_ch_names.txt"), "w") as f:
        f.write("\n".join(ch_names) + "\n")

    # ── Export mapping matrix ───────────────────────────────────────────
    np.save(os.path.join(args.outdir, "meg_mapping.npy"), mapping)
    print(f"[helmet-ref] Mapping: {mapping.shape}")

    # ── Apply mapping at t = 0.083 s ───────────────────────────────────
    time = 0.083
    tidx = int(np.argmin(np.abs(evoked.times - time)))
    actual_t = evoked.times[tidx]
    print(f"[helmet-ref] Target time: {time}s -> index {tidx} "
          f"(actual {actual_t:.6f}s)")

    # Build channel index mapping
    ev_ch_names = list(evoked.info["ch_names"])
    ch_picks = [ev_ch_names.index(n) for n in ch_names]

    # Channel data at t=0.083
    meas = evoked.data[ch_picks, tidx]
    np.save(os.path.join(args.outdir, "meg_data_t083.npy"), meas)

    # Mapped field at t=0.083
    mapped = mapping @ meas
    np.save(os.path.join(args.outdir, "meg_mapped_t083.npy"), mapped)
    print(f"[helmet-ref] Data at t083: range=[{meas.min():.4e}, {meas.max():.4e}]")
    print(f"[helmet-ref] Mapped at t083: range=[{mapped.min():.4e}, {mapped.max():.4e}]")
    print(f"[helmet-ref] Max|mapped|: {np.abs(mapped).max():.4e}")

    # ── All time points ─────────────────────────────────────────────────
    data_all = evoked.data[ch_picks, :]
    mapped_all = mapping @ data_all
    np.save(os.path.join(args.outdir, "meg_data_all.npy"), data_all)
    np.save(os.path.join(args.outdir, "meg_mapped_all.npy"), mapped_all)
    print(f"[helmet-ref] Data all: {data_all.shape}")
    print(f"[helmet-ref] Mapped all: {mapped_all.shape}")

    # ── Save time index ─────────────────────────────────────────────────
    np.save(os.path.join(args.outdir, "time_index.npy"),
            np.array([tidx], dtype=np.int64))

    # ── Write info summary ──────────────────────────────────────────────
    with open(os.path.join(args.outdir, "info.txt"), "w") as f:
        f.write(f"source: mne_helmet example\n")
        f.write(f"condition: Left Auditory\n")
        f.write(f"ch_type: meg\n")
        f.write(f"origin: {origin[0]:.6f} {origin[1]:.6f} {origin[2]:.6f}\n")
        f.write(f"upsampling: 2\n")
        f.write(f"n_vertices: {verts.shape[0]}\n")
        f.write(f"n_channels: {len(ch_names)}\n")
        f.write(f"n_times: {evoked.data.shape[1]}\n")
        f.write(f"time_index_083: {tidx}\n")
        f.write(f"actual_time: {actual_t:.6f}\n")
        f.write(f"mapping_shape: {mapping.shape[0]} {mapping.shape[1]}\n")
        f.write(f"max_abs_mapped_083: {np.abs(mapped).max():.6e}\n")

    # ── Global range across all time points ─────────────────────────────
    global_max = np.abs(mapped_all).max()
    print(f"\n[helmet-ref] === Global max|mapped| across all times: {global_max:.4e} ===")

    # Per-time-point max values for a few representative times
    for t_check in [0.0, 0.050, 0.083, 0.100, 0.150]:
        ti = int(np.argmin(np.abs(evoked.times - t_check)))
        m = mapped_all[:, ti]
        print(f"  t={evoked.times[ti]:.4f}s: max|mapped|={np.abs(m).max():.4e}")

    print(f"\n[helmet-ref] Done – reference data in {args.outdir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
