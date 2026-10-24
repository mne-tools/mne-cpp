#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Run the built examples and collect their figures and console output for the website.
"""Run the built examples and collect their figures and console output for the website.

Every example runs with its default options, ``MNE_CPP_FIGURE_DIR`` pointing at
``doc/website/static/img/examples/<name>/`` and ``QT_QPA_PLATFORM=offscreen``.
The PNGs it writes through ``DISPLIB::FigureExporter`` and its console output
(``output.txt``, with the repository path made relative) replace the previous
results, which ``generate_examples.py`` then embeds into the example's page.

An example that fails, or needs data that is not installed, keeps its previous
results and is reported. The examples need the MNE sample dataset in
``resources/data/MNE-sample-data`` and ``resources/data/mne-cpp-test-data``.

Usage
-----
    python3 scripts/doxy2mdx/run_examples.py                       # every example
    python3 scripts/doxy2mdx/run_examples.py ex_averaging ex_tf_plot
    python3 scripts/doxy2mdx/run_examples.py --bin out/Release/examples
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import generate_examples as gen  # noqa: E402

TIMEOUT = 600
# Messages of the offscreen platform plugin, not of the example.
QT_NOISE = re.compile(r"Populating font family aliases|does not support propagateSizeHints")
# Examples that need a network or end in an interactive 3D view that does not close on its own.
SKIP = {"ex_lsl": "needs a Lab Streaming Layer network",
        "ex_connectivity_performance": "benchmark sweep of several hours"} | {name: "interactive 3D view" for name in (
    "ex_clustered_inverse_mne", "ex_clustered_inverse_mne_raw", "ex_clustered_inverse_pwl_rap_music_raw",
    "ex_clustered_inverse_rap_music_raw", "ex_connectivity", "ex_connectivity_comparison", "ex_coreg", "ex_disp_3D",
    "ex_fs_surface", "ex_inverse_mne_raw", "ex_inverse_pwl_rap_music", "ex_inverse_rap_music", "ex_read_fwd_disp_3D",
    "ex_roi_clustered_inverse_pwl_rap_music", "ex_st_clustered_inverse_pwl_rap_music")}


def executable(bin_dir: Path, name: str) -> Path | None:
    """The example binary."""
    for candidate in (bin_dir / name, bin_dir / f"{name}.exe"):
        if candidate.is_file():
            return candidate
    return None


def run(name: str, binary: Path, root: Path) -> tuple[bool, str]:
    """Run one example into a scratch directory; on success move its results into the gallery."""
    with tempfile.TemporaryDirectory() as tmp:
        figures = Path(tmp) / "figures"
        figures.mkdir()
        env = dict(os.environ, MNE_CPP_FIGURE_DIR=str(figures), QT_QPA_PLATFORM="offscreen")
        try:
            done = subprocess.run([str(binary)], cwd=tmp, env=env, capture_output=True, text=True,
                                  errors="replace", timeout=TIMEOUT)
        except subprocess.TimeoutExpired:
            return False, f"timed out after {TIMEOUT} s"
        output = (done.stdout + done.stderr).replace(f"{binary.parent}/../", "").replace(f"{root}/", "")
        output = "\n".join(line for line in output.splitlines() if not QT_NOISE.search(line))
        if done.returncode != 0:
            tail = "\n    ".join(output.strip().splitlines()[-5:])
            return False, f"exit code {done.returncode}\n    {tail}"
        target = root / gen.GALLERY / name
        shutil.rmtree(target, ignore_errors=True)
        target.mkdir(parents=True)
        for png in sorted(figures.glob("*.png")):
            shutil.move(png, target / png.name)
        (target / "output.txt").write_text(output.strip() + "\n", encoding="utf-8")
        return True, f"{len(list(target.glob('*.png')))} figure(s), {len(output.splitlines())} output line(s)"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("names", nargs="*", help="examples to run (default: all)")
    parser.add_argument("--bin", type=Path, default=gen.REPO_ROOT / "out/Release/examples",
                        help="directory of the built examples")
    args = parser.parse_args(argv)
    root = gen.REPO_ROOT
    names = args.names or [p.name for p in sorted((root / gen.EXAMPLES).glob("ex_*/"))]
    failed = []
    skipped = 0
    for name in names:
        if name in SKIP:
            print(f"skip {name}: {SKIP[name]}")
            skipped += 1
            continue
        binary = executable(args.bin.resolve(), name)
        if binary is None:
            failed.append(name)
            print(f"FAIL {name}: not built in {args.bin}")
            continue
        ok, message = run(name, binary, root)
        print(f"{'ok  ' if ok else 'FAIL'} {name}: {message}", flush=True)
        if not ok:
            failed.append(name)
    print(f"\n{len(names) - len(failed) - skipped} of {len(names)} examples ran, {skipped} skipped; regenerate the pages with "
          "python3 scripts/doxy2mdx/generate_examples.py")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
