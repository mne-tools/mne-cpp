#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    Reproducible benchmarks of MNE-CPP for the website's Performance page.
"""Reproducible benchmarks of MNE-CPP, with MNE-Python as the reference where the comparison is fair.

Every suite runs built MNE-CPP binaries (and, for the comparisons, MNE-Python on the same files with the same
parameters) and records the environment, so every number can be traced to hardware, operating system, compiler
and commit. The results file feeds ``render_charts.py``.

Suites
------
acq        mne_acq_bench: ring throughput over a channel sweep, wake-up latency percentiles, loss-free paced
           rates, decode throughput (no MNE-Python counterpart: MNE-Python does not acquire)
forward    mne_forward_solution vs mne.make_forward_solution: MNE sample subject, oct-6 source space,
           3-layer BEM, MEG+EEG, mindist 5 mm; the timed span of both reads the inputs and writes the result
inverse    mne_inverse_operator vs mne.minimum_norm.make_inverse_operator from that forward solution and the
           sample noise covariance, loose 0.2, depth weighting; same timed span
startup    wall time and peak memory of a command-line tool's start (--version), against
           ``python -c "import mne"``

    python3 scripts/bench/run_benchmarks.py --build build/developer-dynamic --out results.json
    python3 scripts/bench/run_benchmarks.py --suites acq,startup --quick

MNE-Python runs with the interpreter given by --python (default: the first python3 on PATH that imports mne).
Times are the median of --repeats runs; the spread is kept in the results.
"""

from __future__ import annotations

import argparse
import datetime
import json
import os
import platform
import re
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any, Callable

REPO_ROOT = Path(__file__).resolve().parents[2]
SAMPLE = Path(os.environ.get("MNE_DATA", Path.home() / "mne_data")) / "MNE-sample-data"


def run(command: list[str], env: dict[str, str] | None = None, cwd: Path | None = None) -> subprocess.CompletedProcess:
    return subprocess.run(command, capture_output=True, text=True, env=env, cwd=cwd, check=False)


def machine() -> str:
    if sys.platform == "darwin":
        brand = run(["sysctl", "-n", "machdep.cpu.brand_string"]).stdout.strip()
        cores = run(["sysctl", "-n", "hw.ncpu"]).stdout.strip()
        memory = int(run(["sysctl", "-n", "hw.memsize"]).stdout.strip() or 0) / 2**30
    else:
        cpuinfo = Path("/proc/cpuinfo").read_text(encoding="utf-8", errors="replace") if Path("/proc/cpuinfo").exists() else ""
        match = re.search(r"model name\s*:\s*(.+)", cpuinfo)
        brand = match.group(1).strip() if match else platform.processor() or platform.machine()
        cores = str(os.cpu_count())
        meminfo = Path("/proc/meminfo").read_text(encoding="utf-8") if Path("/proc/meminfo").exists() else ""
        match = re.search(r"MemTotal:\s+(\d+)", meminfo)
        memory = int(match.group(1)) / 2**20 if match else 0
    return f"{brand}, {cores} cores, {memory:.0f} GB"


def operating_system() -> str:
    if sys.platform == "darwin":
        return f"macOS {platform.mac_ver()[0]}"
    if sys.platform.startswith("linux"):
        release = Path("/etc/os-release")
        if release.exists():
            match = re.search(r'PRETTY_NAME="([^"]+)"', release.read_text(encoding="utf-8"))
            if match:
                return match.group(1)
    return platform.platform()


def compiler(build: Path) -> str:
    for config in sorted(build.glob("CMakeFiles/*/CMakeCXXCompiler.cmake")):
        text = config.read_text(encoding="utf-8")
        ident = re.search(r'set\(CMAKE_CXX_COMPILER_ID "([^"]*)"\)', text)
        version = re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]*)"\)', text)
        if ident and version:
            build_type = re.search(r"^CMAKE_BUILD_TYPE:\w+=(.*)$",
                                   (build / "CMakeCache.txt").read_text(encoding="utf-8"), re.MULTILINE)
            return f"{ident.group(1)} {version.group(1)} {build_type.group(1) if build_type else ''}".strip()
    return "unknown compiler"


def environment(build: Path, python: str | None) -> dict[str, str]:
    commit = run(["git", "rev-parse", "--short=10", "HEAD"], cwd=REPO_ROOT).stdout.strip()
    dirty = run(["git", "status", "--porcelain", "--untracked-files=no"], cwd=REPO_ROOT).stdout.strip()
    result = {"machine": machine(), "os": operating_system(), "compiler": compiler(build),
              "commit": commit + ("+dirty" if dirty else ""), "date": datetime.date.today().isoformat()}
    if python:
        versions = run([python, "-c", "import mne, numpy, scipy, sys; "
                                      "print(mne.__version__, numpy.__version__, scipy.__version__, sys.version.split()[0])"])
        if versions.returncode == 0:
            mne_version, numpy_version, scipy_version, python_version = versions.stdout.split()
            result["reference"] = (f"MNE-Python {mne_version}, NumPy {numpy_version}, SciPy {scipy_version}, "
                                   f"Python {python_version}")
    return result


def tool_env(build: Path) -> dict[str, str]:
    """Environment for running the built tools (Qt frameworks next to the build on macOS)."""
    env = dict(os.environ, QT_QPA_PLATFORM="offscreen")
    qt_lib = REPO_ROOT / "src" / "external" / "qt" / "dynamic" / "lib"
    if qt_lib.exists():
        env["DYLD_FRAMEWORK_PATH"] = str(qt_lib)
        env["LD_LIBRARY_PATH"] = f"{qt_lib}:{env.get('LD_LIBRARY_PATH', '')}"
    return env


def binary(build: Path, name: str) -> Path:
    for directory in ("out/Release/bin", "out/Release/tests"):
        candidate = REPO_ROOT / directory / name
        if candidate.exists():
            return candidate
    candidate = build / name
    if candidate.exists():
        return candidate
    raise FileNotFoundError(f"{name} not found; build it first")


def measured(command: list[str], env: dict[str, str] | None = None) -> tuple[float, float]:
    """Wall seconds and peak resident memory in MB of one run."""
    time_tool = shutil.which("time") or "/usr/bin/time"
    flag = "-l" if sys.platform == "darwin" else "-v"
    start = time.perf_counter()
    result = run([time_tool, flag, *command], env=env)
    wall = time.perf_counter() - start
    if result.returncode != 0:
        raise RuntimeError(f"{' '.join(command)} failed: {result.stderr[-800:]}")
    if sys.platform == "darwin":
        match = re.search(r"(\d+)\s+maximum resident set size", result.stderr)
        peak = int(match.group(1)) / 2**20 if match else float("nan")
    else:
        match = re.search(r"Maximum resident set size \(kbytes\): (\d+)", result.stderr)
        peak = int(match.group(1)) / 2**10 if match else float("nan")
    return wall, peak


def repeated(times: int, once: Callable[[], float]) -> dict[str, Any]:
    values = [once() for _ in range(times)]
    return {"median": statistics.median(values), "min": min(values), "max": max(values), "runs": values}


def python_timed(python: str, code: str) -> float:
    """Seconds the timed block of a Python snippet reports (it prints the seconds as its last line)."""
    result = run([python, "-c", code])
    if result.returncode != 0:
        raise RuntimeError(f"MNE-Python run failed: {result.stderr[-800:]}")
    return float(result.stdout.strip().splitlines()[-1])


# ── Suites ──────────────────────────────────────────────────────────────────────────────────────────────────


def suite_acq(build: Path, args: argparse.Namespace) -> list[dict[str, Any]]:
    bench = binary(build, "mne_acq_bench")
    with tempfile.TemporaryDirectory() as tmp:
        out = Path(tmp) / "acq.json"
        command = [str(bench), "--repeats", str(args.repeats), "--json", str(out)] + (["--quick"] if args.quick else [])
        result = run(command, env=tool_env(build))
        # mne_acq_bench exits 1 when a run lost samples but still writes its results; a loss is reported, not hidden
        if result.returncode not in (0, 1) or not out.is_file():
            raise RuntimeError(f"mne_acq_bench failed: {result.stderr[-800:]}")
        metrics = {m["name"]: m for m in json.loads(out.read_text(encoding="utf-8"))["metrics"]}
    lost = [describe_paced(m["name"]) for m in metrics.values() if m["value"] < 0]
    reproduce = "mne_acq_bench --repeats 3 --json acq.json"
    value = lambda name: metrics[name]["value"]  # noqa: E731
    charts = [{
        "id": "acq_throughput", "title": "Acquisition ring throughput", "unit": "GB/s", "higher_is_better": True,
        "subtitle": "int16 samples through the lock-free ring, producer and two readers, verified",
        "command": reproduce,
        "series": [{"label": "MNE-CPP", "points": [{"x": f"{n} channels", "y": value(f"ring_int16_{n}_ch")}
                                                  for n in (1, 64, 1024, 4096)]}]}, {
        "id": "acq_latency", "title": "Commit-to-wake latency", "unit": "µs", "higher_is_better": False,
        "subtitle": "time from a producer's commit until the reader runs, per percentile",
        "command": reproduce,
        "series": [{"label": label, "points": [{"x": f"{reader} reader", "y": value(f"wake_{reader}_{suffix}") / 1000}
                                               for reader in ("spin", "output", "block")]}
                   for label, suffix in (("p50", "p50"), ("p99", "p99"), ("p99.9", "p999"))]}, {
        "id": "acq_paced", "title": "Loss-free real-time acquisition", "unit": "MS/s", "higher_is_better": True,
        "subtitle": "paced streams that must not lose a sample (channels × rate)",
        "command": reproduce,
        "series": [{"label": "MNE-CPP", "points": [
            {"x": "64 ch × 1 kHz", "y": 64 * value("paced_float32_64_ch_1_khz") / 1000},
            {"x": "1000 ch × 20 kHz", "y": 1000 * value("paced_float32_1000_ch_20_khz") / 1000},
            {"x": "256 ch × 20 kHz to FIFF", "y": 256 * value("record_float32_256_ch_20_khz") / 1000}]}]}]
    if lost:
        charts[-1]["notes"] = [f"Lost samples on this machine: {', '.join(lost)}."]
    return charts


def describe_paced(name: str) -> str:
    """'paced_int16_4_ch_62500_khz' -> '4 ch × 62500 kHz int16'; other names unchanged."""
    match = re.fullmatch(r"(?:paced|record)_(\w+?)_(\d+)_ch_(\d+)_khz", name)
    if not match:
        return name
    fmt, channels, khz = match.groups()
    rate = f"{int(khz) / 1000:g} MHz" if int(khz) >= 1000 else f"{khz} kHz"
    return f"{channels} ch × {rate} {fmt}"


def suite_forward(build: Path, args: argparse.Namespace) -> list[dict[str, Any]]:
    raw = SAMPLE / "MEG/sample/sample_audvis_raw.fif"
    trans = SAMPLE / "MEG/sample/sample_audvis_raw-trans.fif"
    src = SAMPLE / "subjects/sample/bem/sample-oct-6-src.fif"
    bem = SAMPLE / "subjects/sample/bem/sample-5120-5120-5120-bem-sol.fif"
    with tempfile.TemporaryDirectory() as tmp:
        fwd = Path(tmp) / "cpp-fwd.fif"
        command = [str(binary(build, "mne_forward_solution")), "--meas", str(raw), "--mri", str(trans), "--src", str(src),
                   "--bem", str(bem), "--meg", "--eeg", "--mindist", "5", "--fwd", str(fwd)]
        cpp = repeated(args.repeats, lambda: measured(command, tool_env(build))[0])
        series = [{"label": "MNE-CPP", "points": [{"x": "oct-6, MEG+EEG, 3-layer BEM", "y": cpp["median"]}]}]
        if args.python:
            code = (f"import time, mne\nmne.set_log_level('ERROR')\nt=time.perf_counter()\n"
                    f"f=mne.make_forward_solution(r'{raw}', trans=r'{trans}', src=r'{src}', bem=r'{bem}', meg=True, "
                    f"eeg=True, mindist=5.0, n_jobs={args.jobs})\n"
                    f"mne.write_forward_solution(r'{Path(tmp) / 'py-fwd.fif'}', f, overwrite=True)\n"
                    f"print(time.perf_counter()-t)")
            py = repeated(args.repeats, lambda: python_timed(args.python, code))
            series.append({"label": f"MNE-Python (n_jobs={args.jobs})",
                           "points": [{"x": "oct-6, MEG+EEG, 3-layer BEM", "y": py["median"]}]})
    return [{"id": "forward", "title": "Forward solution", "unit": "s", "higher_is_better": False,
             "subtitle": "MNE sample subject; read inputs, compute, write; median of runs",
             "command": "python3 scripts/bench/run_benchmarks.py --suites forward", "series": series}]


def suite_inverse(build: Path, args: argparse.Namespace) -> list[dict[str, Any]]:
    fwd = SAMPLE / "MEG/sample/sample_audvis-meg-eeg-oct-6-fwd.fif"
    cov = SAMPLE / "MEG/sample/sample_audvis-cov.fif"
    raw = SAMPLE / "MEG/sample/sample_audvis_raw.fif"
    with tempfile.TemporaryDirectory() as tmp:
        command = [str(binary(build, "mne_inverse_operator")), "--fwd", str(fwd), "--noisecov", str(cov), "--meg",
                   "--eeg", "--loose", "0.2", "--depth", "--inv", str(Path(tmp) / "cpp-inv.fif")]
        cpp = repeated(args.repeats, lambda: measured(command, tool_env(build))[0])
        series = [{"label": "MNE-CPP", "points": [{"x": "MEG+EEG, loose 0.2, depth", "y": cpp["median"]}]}]
        if args.python:
            code = (f"import time, mne\nfrom mne.minimum_norm import make_inverse_operator, write_inverse_operator\n"
                    f"mne.set_log_level('ERROR')\nt=time.perf_counter()\n"
                    f"f=mne.read_forward_solution(r'{fwd}')\nc=mne.read_cov(r'{cov}')\n"
                    f"i=make_inverse_operator(mne.io.read_info(r'{raw}'), f, c, loose=0.2, depth=0.8)\n"
                    f"write_inverse_operator(r'{Path(tmp) / 'py-inv.fif'}', i, overwrite=True)\n"
                    f"print(time.perf_counter()-t)")
            py = repeated(args.repeats, lambda: python_timed(args.python, code))
            series.append({"label": "MNE-Python", "points": [{"x": "MEG+EEG, loose 0.2, depth", "y": py["median"]}]})
    return [{"id": "inverse", "title": "Inverse operator", "unit": "s", "higher_is_better": False,
             "subtitle": "MNE sample data; read forward and covariance, assemble, write; median of runs",
             "command": "python3 scripts/bench/run_benchmarks.py --suites inverse", "series": series}]


def suite_startup(build: Path, args: argparse.Namespace) -> list[dict[str, Any]]:
    tool = [str(binary(build, "mne_show_fiff")), "--version"]
    samples = [measured(tool, tool_env(build)) for _ in range(max(args.repeats, 5))]
    wall = [{"x": "CLI tool (mne_show_fiff --version)", "y": 1000 * statistics.median(s[0] for s in samples)}]
    memory = [{"x": "CLI tool (mne_show_fiff --version)", "y": statistics.median(s[1] for s in samples)}]
    series_wall = [{"label": "MNE-CPP", "points": wall}]
    series_memory = [{"label": "MNE-CPP", "points": memory}]
    if args.python:
        py = [measured([args.python, "-c", "import mne"]) for _ in range(max(args.repeats, 5))]
        series_wall.append({"label": "MNE-Python", "points": [{"x": "import mne", "y": 1000 * statistics.median(s[0] for s in py)}]})
        series_memory.append({"label": "MNE-Python", "points": [{"x": "import mne", "y": statistics.median(s[1] for s in py)}]})
    reproduce = "python3 scripts/bench/run_benchmarks.py --suites startup"
    return [{"id": "startup_time", "title": "Start-up time", "unit": "ms", "higher_is_better": False,
             "subtitle": "process start to exit, median of runs", "command": reproduce, "series": series_wall},
            {"id": "startup_memory", "title": "Peak memory at start-up", "unit": "MB", "higher_is_better": False,
             "subtitle": "maximum resident set size", "command": reproduce, "series": series_memory}]


SUITES = {"acq": suite_acq, "forward": suite_forward, "inverse": suite_inverse, "startup": suite_startup}


def find_python() -> str | None:
    for candidate in (shutil.which("python3"), sys.executable,
                      "/Library/Frameworks/Python.framework/Versions/Current/bin/python3"):
        if candidate and Path(candidate).exists() and run([candidate, "-c", "import mne"]).returncode == 0:
            return candidate
    return None


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build", type=Path, default=REPO_ROOT / "build" / "developer-dynamic")
    parser.add_argument("--suites", default=",".join(SUITES), help="comma-separated: " + ", ".join(SUITES))
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--jobs", type=int, default=1, help="MNE-Python n_jobs where it has one")
    parser.add_argument("--python", default=None, help="interpreter with MNE-Python; 'none' skips the comparison")
    parser.add_argument("--quick", action="store_true", help="short runs, for CI")
    parser.add_argument("--out", type=Path, default=REPO_ROOT / "doc" / "website" / "static" / "bench" / "results.json")
    args = parser.parse_args(argv)
    args.python = None if args.python == "none" else (args.python or find_python())

    charts = []
    for name in args.suites.split(","):
        print(f"== {name}", flush=True)
        charts += SUITES[name](args.build, args)
    results = {"generated_by": "scripts/bench/run_benchmarks.py", "environment": environment(args.build, args.python),
               "charts": charts}
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(results, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"wrote {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
