#!/usr/bin/env python3
"""
Generate reference STC files using mne-python for cross-validation
against mne-cpp's inverse computation.

Usage:
    python generate_reference_stc.py <output_dir> [<data_dir>]

Parameters match the C++ test: SNR=3, lambda2=1/9, dSPM, set_no=0, no baseline.
Writes: <output_dir>/ref-lh.stc and <output_dir>/ref-rh.stc
Also writes a summary to stdout for the C++ test to parse.
"""

import sys
import os
import numpy as np
import mne


def main():
    if len(sys.argv) < 2:
        print("ERROR: Usage: generate_reference_stc.py <output_dir> [<data_dir>]",
              file=sys.stderr)
        sys.exit(1)

    output_dir = sys.argv[1]
    if len(sys.argv) >= 3:
        data_dir = sys.argv[2]
    else:
        data_dir = os.path.expanduser("~/mne_data/MNE-sample-data")

    evoked_file = os.path.join(data_dir, "MEG", "sample", "sample_audvis-ave.fif")
    inv_file = os.path.join(data_dir, "MEG", "sample",
                            "sample_audvis-meg-eeg-oct-6-meg-eeg-inv.fif")

    if not os.path.exists(evoked_file):
        print(f"ERROR: Evoked file not found: {evoked_file}", file=sys.stderr)
        sys.exit(1)
    if not os.path.exists(inv_file):
        print(f"ERROR: Inverse file not found: {inv_file}", file=sys.stderr)
        sys.exit(1)

    # Parameters matching the C++ test
    snr = 3.0
    lambda2 = 1.0 / snr ** 2
    method = "dSPM"
    set_no = 0  # first evoked dataset (Left Auditory)

    # Read evoked data WITHOUT baseline correction
    evokeds = mne.read_evokeds(evoked_file, baseline=None)
    evoked = evokeds[set_no]
    print(f"Evoked: {evoked.comment}, {evoked.nave} averages, "
          f"{len(evoked.times)} time points")
    print(f"  tmin={evoked.times[0]:.6f}, tmax={evoked.times[-1]:.6f}, "
          f"sfreq={evoked.info['sfreq']:.1f}")

    # Read inverse operator
    inv = mne.minimum_norm.read_inverse_operator(inv_file)
    print(f"Inverse: {inv['nsource']} sources, {inv['nchan']} channels")

    # Compute inverse
    stc = mne.minimum_norm.apply_inverse(evoked, inv, lambda2, method,
                                          pick_ori=None)

    print(f"STC: {stc.data.shape[0]} sources, {stc.data.shape[1]} time points")
    print(f"  tmin={stc.tmin:.6f}, tstep={stc.tstep:.8f}")
    print(f"  lh vertices: {len(stc.lh_vertno)}")
    print(f"  rh vertices: {len(stc.rh_vertno)}")
    print(f"  data range: [{stc.data.min():.6e}, {stc.data.max():.6e}]")

    # Save split lh/rh STCs
    stc_path = os.path.join(output_dir, "ref")
    stc.save(stc_path, overwrite=True)

    # Verify files were created
    lh_path = os.path.join(output_dir, "ref-lh.stc")
    rh_path = os.path.join(output_dir, "ref-rh.stc")

    if not os.path.exists(lh_path):
        print(f"ERROR: LH STC not created: {lh_path}", file=sys.stderr)
        sys.exit(1)
    if not os.path.exists(rh_path):
        print(f"ERROR: RH STC not created: {rh_path}", file=sys.stderr)
        sys.exit(1)

    print(f"OK: {lh_path} ({os.path.getsize(lh_path)} bytes)")
    print(f"OK: {rh_path} ({os.path.getsize(rh_path)} bytes)")

    # Print machine-readable summary for C++ test to parse
    print(f"SUMMARY:tmin={stc.tmin:.8f}")
    print(f"SUMMARY:tstep={stc.tstep:.10f}")
    print(f"SUMMARY:n_lh={len(stc.lh_vertno)}")
    print(f"SUMMARY:n_rh={len(stc.rh_vertno)}")
    print(f"SUMMARY:n_times={stc.data.shape[1]}")
    print(f"SUMMARY:max_val={stc.data.max():.8e}")


if __name__ == "__main__":
    main()
