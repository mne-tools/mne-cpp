#!/usr/bin/env python3
"""Generate reference *applied* field maps for ALL evoked conditions.

This script complements generate_fieldmap_reference.py by computing
the full end-to-end result: for every evoked condition in the input
file it applies the mapping matrix to the channel data, producing the
mapped field values that mne-cpp should reproduce exactly.

Outputs per condition *i* (written to --outdir):
    meg_data_cond<i>.npy        (n_meg_ch,)  channel data at peak time
    meg_mapped_cond<i>.npy      (n_verts,)   mapped MEG field at peak
    eeg_data_cond<i>.npy        (n_eeg_ch,)  channel data at peak time
    eeg_mapped_cond<i>.npy      (n_verts,)   mapped EEG field at peak
    meg_data_cond<i>_all.npy    (n_meg_ch, n_times) all time points
    meg_mapped_cond<i>_all.npy  (n_verts, n_times)  all mapped
    eeg_data_cond<i>_all.npy    (n_eeg_ch, n_times) all time points
    eeg_mapped_cond<i>_all.npy  (n_verts, n_times)  all mapped

Global outputs:
    meg_mapping.npy         (n_verts, n_meg_ch)   mapping matrix
    eeg_mapping.npy         (n_verts, n_eeg_ch)   mapping matrix
    conditions.txt          one condition name per line
    peak_times.txt          peak time index per condition

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
from mne.forward._make_forward import _create_meg_coils, _create_eeg_els


def _read_head_surface(surf_path):
    """Read a BEM head surface from a .fif file."""
    surfs = mne.read_bem_surfaces(surf_path, patch_stats=False, verbose=False)
    for s in surfs:
        if s.get("id", 0) == 4:
            return s
    return surfs[0]


def _pick_good_channels(info, ch_type):
    """Return pick indices for good channels of the given type."""
    picks = mne.pick_types(
        info, meg=(ch_type == "meg"), eeg=(ch_type == "eeg"), exclude="bads"
    )
    return picks


def main():
    parser = argparse.ArgumentParser(
        description="Generate applied field-map references for all conditions."
    )
    parser.add_argument("--evoked", required=True,
                        help="Path to evoked .fif")
    parser.add_argument("--trans", required=True,
                        help="Path to head→MRI transform .fif")
    parser.add_argument("--surf", required=True,
                        help="Path to BEM head surface .fif")
    parser.add_argument("--outdir", required=True,
                        help="Output directory")
    parser.add_argument("--mode", default="accurate",
                        choices=["fast", "accurate"])
    parser.add_argument("--max-verts", default=642, type=int)
    args = parser.parse_args()

    os.makedirs(args.outdir, exist_ok=True)

    # ── Load all conditions ─────────────────────────────────────────────
    print(f"[multi-ref] Reading all evoked conditions from: {args.evoked}")
    all_evoked = mne.read_evokeds(args.evoked, verbose=False)
    n_cond = len(all_evoked)
    print(f"[multi-ref] Found {n_cond} conditions:")
    for i, ev in enumerate(all_evoked):
        print(f"  [{i}] '{ev.comment}' "
              f"({ev.data.shape[0]} ch × {ev.data.shape[1]} times, "
              f"nave={ev.nave})")

    # Save condition names
    with open(os.path.join(args.outdir, "conditions.txt"), "w") as f:
        for ev in all_evoked:
            f.write(ev.comment + "\n")

    # Use first evoked's info for geometry (should be same for all)
    info0 = all_evoked[0].info

    # ── Transform & surface ─────────────────────────────────────────────
    print(f"[multi-ref] Reading transform: {args.trans}")
    trans = mne.read_trans(args.trans, verbose=False)

    print(f"[multi-ref] Reading head surface: {args.surf}")
    surf = _read_head_surface(args.surf)

    origin = np.array([0.0, 0.0, 0.04])
    np.save(os.path.join(args.outdir, "origin.npy"), origin)

    mri_head_t = mne.transforms.invert_transform(trans)
    surf_head = mne.transform_surface_to(surf, "head", mri_head_t, copy=True)

    n_verts_orig = surf_head["rr"].shape[0]
    if n_verts_orig > args.max_verts:
        print(f"[multi-ref] Subsampling: {n_verts_orig} → {args.max_verts}")
        rng = np.random.RandomState(42)
        sel_verts = np.sort(rng.choice(n_verts_orig, args.max_verts,
                                        replace=False))
        surf_head["rr"] = surf_head["rr"][sel_verts]
        surf_head["nn"] = surf_head["nn"][sel_verts]
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
    print(f"[multi-ref] Surface: {verts.shape[0]} vertices")

    # ── Compute MEG mapping matrix ──────────────────────────────────────
    meg_mapping = None
    meg_picks = _pick_good_channels(info0, "meg")
    meg_ch_names = []

    if len(meg_picks) > 0:
        print(f"[multi-ref] MEG: {len(meg_picks)} channels")
        meg_info = mne.pick_info(info0, meg_picks, copy=True)
        meg_ch_names = list(meg_info["ch_names"])

        with open(os.path.join(args.outdir, "meg_ch_names.txt"), "w") as f:
            f.write("\n".join(meg_ch_names) + "\n")

        meg_coils = _create_meg_coils(meg_info["chs"], "normal")
        int_rad, noise, lut_fun, n_fact = _setup_dots(
            args.mode, meg_info, meg_coils, "meg"
        )
        self_dots = _do_self_dots(
            int_rad, False, meg_coils, origin, "meg",
            lut_fun, n_fact, n_jobs=1
        )
        sel = np.arange(verts.shape[0])
        surface_dots = _do_surface_dots(
            int_rad, False, meg_coils, surf_head, sel, origin, "meg",
            lut_fun, n_fact, n_jobs=1
        )
        fmd = dict(
            kind="meg", ch_names=meg_ch_names, origin=origin, noise=noise,
            self_dots=self_dots.copy(), surface_dots=surface_dots.copy(),
            int_rad=int_rad, miss=1e-4,
        )
        meg_mapping = _compute_mapping_matrix(fmd, meg_info)
        np.save(os.path.join(args.outdir, "meg_mapping.npy"), meg_mapping)
        print(f"[multi-ref] MEG mapping: {meg_mapping.shape}")

    # ── Compute EEG mapping matrix ──────────────────────────────────────
    eeg_mapping = None
    eeg_picks = _pick_good_channels(info0, "eeg")
    eeg_ch_names = []

    if len(eeg_picks) > 0:
        print(f"[multi-ref] EEG: {len(eeg_picks)} channels")
        eeg_info = mne.pick_info(info0, eeg_picks, copy=True)
        eeg_ch_names = list(eeg_info["ch_names"])

        with open(os.path.join(args.outdir, "eeg_ch_names.txt"), "w") as f:
            f.write("\n".join(eeg_ch_names) + "\n")

        eeg_coils = _create_eeg_els(eeg_info["chs"])
        int_rad, noise, lut_fun, n_fact = _setup_dots(
            args.mode, eeg_info, eeg_coils, "eeg"
        )
        self_dots = _do_self_dots(
            int_rad, False, eeg_coils, origin, "eeg",
            lut_fun, n_fact, n_jobs=1
        )
        sel = np.arange(verts.shape[0])
        surface_dots = _do_surface_dots(
            int_rad, False, eeg_coils, surf_head, sel, origin, "eeg",
            lut_fun, n_fact, n_jobs=1
        )
        fmd = dict(
            kind="eeg", ch_names=eeg_ch_names, origin=origin, noise=noise,
            self_dots=self_dots.copy(), surface_dots=surface_dots.copy(),
            int_rad=int_rad, miss=1e-3,
        )
        eeg_mapping = _compute_mapping_matrix(fmd, eeg_info)
        np.save(os.path.join(args.outdir, "eeg_mapping.npy"), eeg_mapping)
        print(f"[multi-ref] EEG mapping: {eeg_mapping.shape}")

    # ── Apply mapping to each condition ─────────────────────────────────
    peak_times = []

    for ci, ev in enumerate(all_evoked):
        print(f"\n[multi-ref] === Condition {ci}: '{ev.comment}' ===")

        # Find peak time (max absolute value across all channels)
        abs_max_per_time = np.abs(ev.data).max(axis=0)
        peak_idx = int(np.argmax(abs_max_per_time))
        peak_times.append(peak_idx)
        print(f"  Peak time index: {peak_idx} "
              f"({ev.times[peak_idx]:.4f} s)")

        # ── MEG ─────────────────────────────────────────────
        if meg_mapping is not None and len(meg_ch_names) > 0:
            # Extract MEG channel data in the same order as the mapping
            ev_info = ev.info
            ev_ch_names = list(ev_info["ch_names"])

            # Build index mapping: for each meg_ch_name, find its
            # position in ev.data rows
            meg_indices = []
            for name in meg_ch_names:
                idx = ev_ch_names.index(name)
                meg_indices.append(idx)
            meg_indices = np.array(meg_indices)

            # All time points
            meg_data_all = ev.data[meg_indices, :]
            meg_mapped_all = meg_mapping @ meg_data_all

            # Peak time point
            meg_data_peak = meg_data_all[:, peak_idx]
            meg_mapped_peak = meg_mapped_all[:, peak_idx]

            np.save(os.path.join(args.outdir,
                                  f"meg_data_cond{ci}.npy"),
                    meg_data_peak)
            np.save(os.path.join(args.outdir,
                                  f"meg_mapped_cond{ci}.npy"),
                    meg_mapped_peak)
            np.save(os.path.join(args.outdir,
                                  f"meg_data_cond{ci}_all.npy"),
                    meg_data_all)
            np.save(os.path.join(args.outdir,
                                  f"meg_mapped_cond{ci}_all.npy"),
                    meg_mapped_all)

            print(f"  MEG data:   {meg_data_all.shape}")
            print(f"  MEG mapped: {meg_mapped_all.shape}")
            print(f"  MEG peak mapped range: "
                  f"[{meg_mapped_peak.min():.6e}, {meg_mapped_peak.max():.6e}]")

        # ── EEG ─────────────────────────────────────────────
        if eeg_mapping is not None and len(eeg_ch_names) > 0:
            ev_ch_names = list(ev.info["ch_names"])
            eeg_indices = []
            for name in eeg_ch_names:
                idx = ev_ch_names.index(name)
                eeg_indices.append(idx)
            eeg_indices = np.array(eeg_indices)

            eeg_data_all = ev.data[eeg_indices, :]
            eeg_mapped_all = eeg_mapping @ eeg_data_all

            eeg_data_peak = eeg_data_all[:, peak_idx]
            eeg_mapped_peak = eeg_mapped_all[:, peak_idx]

            np.save(os.path.join(args.outdir,
                                  f"eeg_data_cond{ci}.npy"),
                    eeg_data_peak)
            np.save(os.path.join(args.outdir,
                                  f"eeg_mapped_cond{ci}.npy"),
                    eeg_mapped_peak)
            np.save(os.path.join(args.outdir,
                                  f"eeg_data_cond{ci}_all.npy"),
                    eeg_data_all)
            np.save(os.path.join(args.outdir,
                                  f"eeg_mapped_cond{ci}_all.npy"),
                    eeg_mapped_all)

            print(f"  EEG data:   {eeg_data_all.shape}")
            print(f"  EEG mapped: {eeg_mapped_all.shape}")
            print(f"  EEG peak mapped range: "
                  f"[{eeg_mapped_peak.min():.6e}, {eeg_mapped_peak.max():.6e}]")

    # Save peak times
    with open(os.path.join(args.outdir, "peak_times.txt"), "w") as f:
        for pt in peak_times:
            f.write(f"{pt}\n")

    # ── Print summary statistics ────────────────────────────────────────
    print("\n[multi-ref] === Summary ===")
    for ci, ev in enumerate(all_evoked):
        print(f"  Condition {ci} ('{ev.comment}'):")
        if meg_mapping is not None:
            mapped = np.load(os.path.join(args.outdir,
                                           f"meg_mapped_cond{ci}.npy"))
            print(f"    MEG peak max|mapped| = {np.abs(mapped).max():.6e}")
        if eeg_mapping is not None:
            mapped = np.load(os.path.join(args.outdir,
                                           f"eeg_mapped_cond{ci}.npy"))
            print(f"    EEG peak max|mapped| = {np.abs(mapped).max():.6e}")

    print(f"\n[multi-ref] Done – {n_cond} conditions written to {args.outdir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
