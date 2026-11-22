#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Check a recording of test_acq_recorder with mne-python (and mne-bids, the BIDS validator).

Usage: check_recording.py spec.json

spec.json names the FIFF file, a little-endian float64 file with the stored raw values (samples x channels)
and what mne.io.read_raw_fif must report. Every warning is an error. The data must equal the raw values
times the channel calibrations exactly, which is how mne-python reads every stored format. Prints OK, or
the first mismatch and exits 1. Exits 2 when a needed package is missing, so the test can skip.
"""

import json
import shutil
import subprocess
import sys
import warnings
from pathlib import Path

try:
    import numpy as np

    import mne
except ImportError as exc:  # pragma: no cover - environment dependent
    print(f"SKIP {exc}")
    sys.exit(2)


def fail(message):
    print(f"FAIL {message}")
    sys.exit(1)


def main():
    spec = json.loads(Path(sys.argv[1]).read_text())
    warnings.simplefilter("error")
    mne.set_log_level("ERROR")
    raw = mne.io.read_raw_fif(spec["fif"], verbose="error")

    if raw.first_samp != spec["first_samp"]:
        fail(f"first_samp {raw.first_samp} != {spec['first_samp']}")
    if raw.n_times != spec["n_times"]:
        fail(f"n_times {raw.n_times} != {spec['n_times']}")
    if raw.orig_format != spec["orig_format"]:
        fail(f"orig_format {raw.orig_format} != {spec['orig_format']}")
    if len(raw.filenames) != spec.get("files", 1):
        fail(f"{len(raw.filenames)} files != {spec.get('files', 1)}")
    if raw.ch_names != spec["ch_names"]:
        fail(f"ch_names {raw.ch_names} != {spec['ch_names']}")

    stored = np.fromfile(spec["expected"], dtype="<f8").reshape(spec["n_times"], len(raw.ch_names)).T
    cals = np.array([ch["cal"] * ch["range"] for ch in raw.info["chs"]], dtype=np.float64)
    expected = stored * cals[:, np.newaxis]
    data = raw.get_data()
    if not np.array_equal(data, expected):
        c, s = np.argwhere(data != expected)[0]
        fail(f"data[{c}, {s}] = {data[c, s]!r} != {expected[c, s]!r}")

    meas = raw.info["meas_date"]
    stamp = [int(meas.timestamp() // 1), meas.microsecond]
    if stamp != spec["meas_date"]:
        fail(f"meas_date {stamp} != {spec['meas_date']}")
    device = raw.info["device_info"] or {}
    for key, value in spec["device_info"].items():
        if device.get(key) != value:
            fail(f"device_info[{key}] {device.get(key)!r} != {value!r}")
    if spec["description_contains"] not in (raw.info["description"] or ""):
        fail(f"description {raw.info['description']!r}")

    annotations = raw.annotations
    want = spec["annotations"]
    if len(annotations) != len(want):
        fail(f"{len(annotations)} annotations != {len(want)}")
    sfreq = raw.info["sfreq"]
    for got, (onset, duration, description) in zip(annotations, want):
        if got["description"] != description:
            fail(f"annotation {got['description']!r} != {description!r}")
        # Onset and end are stored as float32 seconds since meas_date.
        tolerance = 1e-9 + 2 * np.finfo(np.float32).eps * (onset + duration) / sfreq
        if abs(got["onset"] - onset / sfreq) > tolerance:
            fail(f"onset {got['onset']!r} != {onset / sfreq!r}")
        if abs(got["duration"] - duration / sfreq) > tolerance:
            fail(f"duration {got['duration']!r} != {duration / sfreq!r}")

    bids = spec.get("bids")
    if bids:
        try:
            import mne_bids
        except ImportError as exc:  # pragma: no cover - environment dependent
            print(f"SKIP {exc}")
            sys.exit(2)
        path = mne_bids.BIDSPath(
            root=bids["root"],
            subject=bids["subject"],
            session=bids.get("session") or None,
            task=bids["task"],
            run=bids.get("run") or None,
            datatype="meg",
            suffix="meg",
            extension=".fif",
        )
        warnings.simplefilter("default")
        with warnings.catch_warnings(record=True) as caught:
            warnings.simplefilter("always")
            back = mne_bids.read_raw_bids(path, verbose="error")
        if not np.array_equal(back.get_data(), data):
            fail("mne_bids.read_raw_bids returns other data")
        if len(back.annotations) != len(want):
            fail(f"read_raw_bids: {len(back.annotations)} annotations != {len(want)}")
        if caught:
            fail(f"read_raw_bids warns: {caught[0].message}")
        validator = shutil.which("bids-validator-deno")
        if validator is None:
            print("SKIP bids-validator-deno")
            sys.exit(2)
        run = subprocess.run(
            [validator, "--ignoreNiftiHeaders", bids["root"]],
            capture_output=True,
            text=True,
            check=False,
        )
        if run.returncode != 0:
            fail(f"BIDS validator:\n{run.stdout}\n{run.stderr}")
    print("OK")


if __name__ == "__main__":
    main()
