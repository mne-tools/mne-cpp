#!/usr/bin/env python3
"""Generate reference field-map matrices using MNE-Python for cross-validation.

This script computes the intermediate and final field-map matrices using
MNE-Python's internal sphere-model routines (_lead_dots.py /
_field_interpolation.py) and saves them as .npy files so the mne-cpp C++
test can load and compare its own results.

Outputs (written to the directory given by --outdir):
    meg_self_dots.npy       (n_meg_ch, n_meg_ch) float64
    meg_surface_dots.npy    (n_surf_verts, n_meg_ch) float64
    meg_mapping.npy         (n_surf_verts, n_meg_ch) float64
    eeg_self_dots.npy       (n_eeg_ch, n_eeg_ch) float64
    eeg_surface_dots.npy    (n_surf_verts, n_eeg_ch) float64
    eeg_mapping.npy         (n_surf_verts, n_eeg_ch) float64
    meg_ch_names.txt        channel names, one per line
    eeg_ch_names.txt        channel names, one per line
    surface_verts.npy       (n_surf_verts, 3) float64  – vertex positions
    surface_norms.npy       (n_surf_verts, 3) float64  – vertex normals
    origin.npy              (3,) float64  – sphere origin in head coords
    meg_noise_stds.npy      (n_meg_ch,) float64  – ad-hoc noise stds
    eeg_noise_stds.npy      (n_eeg_ch,) float64
    meg_coil_rmag.npy       coil integration point positions (flat)
    meg_coil_cosmag.npy     coil direction vectors (flat)
    meg_coil_w.npy          coil integration weights (flat)
    meg_coil_slices.npy     (n_coils, 2) start/end indices into the flat arrays
    eeg_coil_rmag.npy       electrode positions (flat)
    eeg_coil_slices.npy     (n_coils, 2)

Usage:
    python generate_fieldmap_reference.py \\
        --evoked /path/to/sample_audvis-ave.fif \\
        --trans  /path/to/sample_audvis_raw-trans.fif \\
        --surf   /path/to/subjects/sample/bem/sample-head.fif \\
        --outdir /path/to/output_dir \\
        [--mode accurate]

Copyright (C) 2026, Christoph Dinh. All rights reserved.
BSD-3-Clause license.
"""

import argparse
import os
import sys

import numpy as np

# ─── MNE imports ────────────────────────────────────────────────────────────
import mne
from mne.cov import make_ad_hoc_cov
from mne.forward._field_interpolation import (
    _compute_mapping_matrix,
    _setup_dots,
)
from mne.forward._lead_dots import _do_self_dots, _do_surface_dots
from mne.forward._make_forward import _create_meg_coils, _create_eeg_els
from mne.surface import read_surface


def _read_head_surface(surf_path):
    """Read a BEM head surface from a .fif file and return an MNE surface dict."""
    surfs = mne.read_bem_surfaces(surf_path, patch_stats=False, verbose=False)
    # Pick the outermost surface (head/scalp) – id == 4
    for s in surfs:
        if s.get("id", 0) == 4:
            return s
    # Fall back to the first surface
    return surfs[0]


def _pick_good_channels(info, ch_type):
    """Return pick indices for good channels of the given type."""
    picks = mne.pick_types(
        info, meg=(ch_type == "meg"), eeg=(ch_type == "eeg"), exclude="bads"
    )
    return picks


def _export_coil_data(coils, outdir, prefix):
    """Export coil integration-point data to .npy files."""
    all_rmag = []
    all_cosmag = []
    all_w = []
    slices = []
    offset = 0
    for coil in coils:
        n = len(coil["rmag"])
        all_rmag.append(coil["rmag"])
        all_cosmag.append(coil["cosmag"])
        all_w.append(coil["w"])
        slices.append([offset, offset + n])
        offset += n

    np.save(os.path.join(outdir, f"{prefix}_coil_rmag.npy"),
            np.vstack(all_rmag))
    if all_cosmag[0].ndim > 0 and all_cosmag[0].shape[-1] == 3:
        np.save(os.path.join(outdir, f"{prefix}_coil_cosmag.npy"),
                np.vstack(all_cosmag))
    np.save(os.path.join(outdir, f"{prefix}_coil_w.npy"),
            np.concatenate(all_w))
    np.save(os.path.join(outdir, f"{prefix}_coil_slices.npy"),
            np.array(slices, dtype=np.int64))


def main():
    parser = argparse.ArgumentParser(
        description="Generate MNE-Python field-map reference data."
    )
    parser.add_argument("--evoked", required=True,
                        help="Path to evoked .fif (e.g. sample_audvis-ave.fif)")
    parser.add_argument("--trans", required=True,
                        help="Path to head→MRI transform .fif")
    parser.add_argument("--surf", required=True,
                        help="Path to BEM head surface .fif "
                             "(e.g. sample-head.fif)")
    parser.add_argument("--outdir", required=True,
                        help="Output directory for .npy files")
    parser.add_argument("--mode", default="accurate",
                        choices=["fast", "accurate"],
                        help="Legendre accuracy mode (default: accurate)")
    parser.add_argument("--condition", default=0, type=int,
                        help="Evoked condition index (default: 0)")
    parser.add_argument("--max-verts", default=642, type=int,
                        help="Maximum surface vertices (default: 642, "
                             "i.e. ico-3 subdivision)")
    args = parser.parse_args()

    os.makedirs(args.outdir, exist_ok=True)

    # ── Load data ───────────────────────────────────────────────────────
    print(f"[fieldmap-ref] Reading evoked:  {args.evoked}")
    evoked = mne.read_evokeds(args.evoked, condition=args.condition,
                              verbose=False)
    info = evoked.info

    print(f"[fieldmap-ref] Reading transform: {args.trans}")
    trans = mne.read_trans(args.trans, verbose=False)

    print(f"[fieldmap-ref] Reading head surface: {args.surf}")
    surf = _read_head_surface(args.surf)

    # ── Sphere origin ───────────────────────────────────────────────────
    # MNE-Python default when origin=="auto": fitted sphere
    # For reproducibility we use the MNE-C default: (0, 0, 0.04)
    origin = np.array([0.0, 0.0, 0.04])
    np.save(os.path.join(args.outdir, "origin.npy"), origin)

    # ── Head surface in head coordinates ────────────────────────────────
    # The BEM surface is in MRI coords → transform to head coords.
    # trans is head → MRI, so we need its inverse.
    mri_head_t = mne.transforms.invert_transform(trans)
    surf_head = mne.transform_surface_to(surf, "head", mri_head_t, copy=True)

    # Subsample if surface is too large (matching MNE-Python's mapping_grade=3
    # which uses ico-3 = 642 vertices)
    n_verts_orig = surf_head["rr"].shape[0]
    if n_verts_orig > args.max_verts:
        print(f"[fieldmap-ref] Subsampling surface: {n_verts_orig} -> {args.max_verts} vertices")
        # Use uniform random selection with fixed seed for reproducibility
        rng = np.random.RandomState(42)
        sel_verts = np.sort(rng.choice(n_verts_orig, args.max_verts, replace=False))
        surf_head["rr"] = surf_head["rr"][sel_verts]
        surf_head["nn"] = surf_head["nn"][sel_verts]
        # Update tris to only include triangles where all 3 vertices are selected
        vert_map = -np.ones(n_verts_orig, dtype=np.int64)
        vert_map[sel_verts] = np.arange(len(sel_verts))
        old_tris = surf_head["tris"]
        mask = np.all(np.isin(old_tris, sel_verts), axis=1)
        new_tris = vert_map[old_tris[mask]]
        surf_head["tris"] = new_tris.astype(np.int32)
        surf_head["ntri"] = len(new_tris)
        surf_head["np"] = len(sel_verts)

    verts = surf_head["rr"].astype(np.float64)
    norms = surf_head["nn"].astype(np.float64)
    np.save(os.path.join(args.outdir, "surface_verts.npy"), verts)
    np.save(os.path.join(args.outdir, "surface_norms.npy"), norms)
    print(f"[fieldmap-ref] Surface: {verts.shape[0]} vertices")

    # ── MEG field map ───────────────────────────────────────────────────
    meg_picks = _pick_good_channels(info, "meg")
    if len(meg_picks) > 0:
        print(f"[fieldmap-ref] MEG: {len(meg_picks)} channels")
        meg_info = mne.pick_info(info, meg_picks, copy=True)
        meg_ch_names = [meg_info["ch_names"][i] for i in range(len(meg_info["ch_names"]))]

        # Write channel names
        with open(os.path.join(args.outdir, "meg_ch_names.txt"), "w") as f:
            f.write("\n".join(meg_ch_names) + "\n")

        # Create MEG coils in head coordinate frame
        meg_coils = _create_meg_coils(meg_info["chs"], "normal")
        _export_coil_data(meg_coils, args.outdir, "meg")

        # Setup dot parameters
        int_rad, noise, lut_fun, n_fact = _setup_dots(
            args.mode, meg_info, meg_coils, "meg"
        )

        # Self-dot matrix
        self_dots = _do_self_dots(
            int_rad, False, meg_coils, origin, "meg",
            lut_fun, n_fact, n_jobs=1
        )
        np.save(os.path.join(args.outdir, "meg_self_dots.npy"), self_dots)
        print(f"[fieldmap-ref] MEG self_dots: {self_dots.shape}")

        # Surface-dot matrix
        sel = np.arange(verts.shape[0])
        surface_dots = _do_surface_dots(
            int_rad, False, meg_coils, surf_head, sel, origin, "meg",
            lut_fun, n_fact, n_jobs=1
        )
        np.save(os.path.join(args.outdir, "meg_surface_dots.npy"), surface_dots)
        print(f"[fieldmap-ref] MEG surface_dots: {surface_dots.shape}")

        # Noise stds
        noise_data = noise.data
        if hasattr(noise_data, "toarray"):
            noise_data = noise_data.toarray()
        meg_stds = np.sqrt(np.diag(noise_data))
        np.save(os.path.join(args.outdir, "meg_noise_stds.npy"), meg_stds)

        # Full mapping matrix via _compute_mapping_matrix
        miss_meg = 1e-4
        fmd = dict(
            kind="meg",
            ch_names=meg_ch_names,
            origin=origin,
            noise=noise,
            self_dots=self_dots.copy(),
            surface_dots=surface_dots.copy(),
            int_rad=int_rad,
            miss=miss_meg,
        )
        mapping = _compute_mapping_matrix(fmd, meg_info)
        np.save(os.path.join(args.outdir, "meg_mapping.npy"), mapping)
        print(f"[fieldmap-ref] MEG mapping: {mapping.shape}")

    # ── EEG field map ───────────────────────────────────────────────────
    eeg_picks = _pick_good_channels(info, "eeg")
    if len(eeg_picks) > 0:
        print(f"[fieldmap-ref] EEG: {len(eeg_picks)} channels")
        eeg_info = mne.pick_info(info, eeg_picks, copy=True)
        eeg_ch_names = [eeg_info["ch_names"][i] for i in range(len(eeg_info["ch_names"]))]

        with open(os.path.join(args.outdir, "eeg_ch_names.txt"), "w") as f:
            f.write("\n".join(eeg_ch_names) + "\n")

        # Create EEG electrodes in head coordinate frame
        eeg_coils = _create_eeg_els(eeg_info["chs"])
        _export_coil_data(eeg_coils, args.outdir, "eeg")

        # Setup dot parameters
        int_rad, noise, lut_fun, n_fact = _setup_dots(
            args.mode, eeg_info, eeg_coils, "eeg"
        )

        # Self-dot matrix
        self_dots = _do_self_dots(
            int_rad, False, eeg_coils, origin, "eeg",
            lut_fun, n_fact, n_jobs=1
        )
        np.save(os.path.join(args.outdir, "eeg_self_dots.npy"), self_dots)
        print(f"[fieldmap-ref] EEG self_dots: {self_dots.shape}")

        # Surface-dot matrix
        sel = np.arange(verts.shape[0])
        surface_dots = _do_surface_dots(
            int_rad, False, eeg_coils, surf_head, sel, origin, "eeg",
            lut_fun, n_fact, n_jobs=1
        )
        np.save(os.path.join(args.outdir, "eeg_surface_dots.npy"), surface_dots)
        print(f"[fieldmap-ref] EEG surface_dots: {surface_dots.shape}")

        # Noise stds
        noise_data = noise.data
        if hasattr(noise_data, "toarray"):
            noise_data = noise_data.toarray()
        eeg_stds = np.sqrt(np.diag(noise_data))
        np.save(os.path.join(args.outdir, "eeg_noise_stds.npy"), eeg_stds)

        # Full mapping matrix
        miss_eeg = 1e-3
        fmd = dict(
            kind="eeg",
            ch_names=eeg_ch_names,
            origin=origin,
            noise=noise,
            self_dots=self_dots.copy(),
            surface_dots=surface_dots.copy(),
            int_rad=int_rad,
            miss=miss_eeg,
        )
        mapping = _compute_mapping_matrix(fmd, eeg_info)
        np.save(os.path.join(args.outdir, "eeg_mapping.npy"), mapping)
        print(f"[fieldmap-ref] EEG mapping: {mapping.shape}")

    print("[fieldmap-ref] Done – reference data written to", args.outdir)
    return 0


if __name__ == "__main__":
    sys.exit(main())
