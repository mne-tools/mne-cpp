#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Run the documented onboarding commands on this machine (v2.4.0 T10.8b).

The commands come from ``setup_commands.json`` (extracted from the build guide
by ``extract_setup_commands.py``). Each step runs in one shell session, so
environment changes such as ``set PATH=...`` carry over, and stops at the first
failing command. The result (per-step time, total time, disk use of the
checkout) is written as JSON and appended to the GitHub job summary.

    python3 tools/onboarding/run_setup_commands.py --os linux --report onboarding.json
"""

from __future__ import annotations

import argparse
import json
import os
import platform
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
COMMANDS_JSON = Path(__file__).with_name("setup_commands.json")
# Documented blocks a contributor on each OS follows, in order
SECTIONS = {"linux": ["linux", "unix"], "macos": ["unix"], "windows": ["windows"]}
_MARK = "__ONBOARDING_STEP__"


def script_for(os_name: str, commands: list[str]) -> tuple[str, list[str]]:
    """Return a shell script that reports and stops at each failing step, and the command to run it."""
    if os_name == "windows":
        lines = ["@echo off"]
        for k, command in enumerate(commands):
            lines += [f"echo {_MARK} {k}", command, f"if errorlevel 1 exit /b 1"]
        return "\r\n".join(lines) + "\r\n", ["cmd.exe", "/d", "/c"]
    lines = ["set -e"]
    for k, command in enumerate(commands):
        lines += [f"echo {_MARK} {k}", command]
    return "\n".join(lines) + "\n", ["bash"]


def disk_usage(path: Path) -> int:
    """Bytes below @p path, following no symlinks."""
    total = 0
    for root, _dirs, files in os.walk(path):
        for name in files:
            full = os.path.join(root, name)
            if not os.path.islink(full):
                total += os.path.getsize(full)
    return total


def run(os_name: str, commands: list[str], cwd: Path = REPO_ROOT) -> dict:
    """Run @p commands in one shell session in @p cwd and time each of them."""
    script, launcher = script_for(os_name, commands)
    suffix = ".bat" if os_name == "windows" else ".sh"
    with tempfile.NamedTemporaryFile("w", suffix=suffix, delete=False, newline="") as handle:
        handle.write(script)
        script_path = handle.name
    steps = [{"command": c, "seconds": None, "ok": False} for c in commands]
    started = time.monotonic()
    current, step_start = -1, started
    process = subprocess.Popen(launcher + [script_path], cwd=cwd, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, errors="replace")
    assert process.stdout is not None
    for line in process.stdout:
        if line.startswith(_MARK):
            now = time.monotonic()
            if current >= 0:
                steps[current].update(seconds=round(now - step_start, 1), ok=True)
            current, step_start = int(line.split()[1]), now
            print(f"::group::{commands[current]}" if os.environ.get("GITHUB_ACTIONS") else f"$ {commands[current]}",
                  flush=True)
            continue
        print(line, end="", flush=True)
    code = process.wait()
    if os.environ.get("GITHUB_ACTIONS") and current >= 0:
        print("::endgroup::", flush=True)
    os.unlink(script_path)
    if current >= 0:
        steps[current].update(seconds=round(time.monotonic() - step_start, 1), ok=code == 0)
    failed = next((s["command"] for s in steps if not s["ok"]), None)
    return {
        "os": os_name,
        "platform": platform.platform(),
        "ok": code == 0,
        "failed_step": failed,
        "total_seconds": round(time.monotonic() - started, 1),
        "checkout_bytes": disk_usage(cwd),
        "steps": steps,
    }


def summary_markdown(result: dict) -> str:
    """Markdown table of the steps for the job summary."""
    rows = [f"### Onboarding on {result['os']}: {'passed' if result['ok'] else 'FAILED'}", "",
            f"Total {result['total_seconds']} s, checkout {result['checkout_bytes'] / 2**30:.2f} GiB "
            f"({result['platform']})", "", "| Step | Time (s) | Result |", "|---|---:|---|"]
    for step in result["steps"]:
        state = "ok" if step["ok"] else ("failed" if step["seconds"] is not None else "not run")
        rows.append(f"| `{step['command']}` | {step['seconds'] if step['seconds'] is not None else ''} | {state} |")
    return "\n".join(rows) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--os", required=True, choices=sorted(SECTIONS), help="documented path to follow")
    parser.add_argument("--report", type=Path, help="write the result as JSON")
    parser.add_argument("--dry-run", action="store_true", help="print the commands without running them")
    args = parser.parse_args(argv)

    documented = json.loads(COMMANDS_JSON.read_text(encoding="utf-8"))
    commands = [c for section in SECTIONS[args.os] for c in documented.get(section, [])]
    if not commands:
        print(f"ERROR: no documented onboarding commands for {args.os}", file=sys.stderr)
        return 1
    if args.dry_run:
        print("\n".join(commands))
        return 0
    if shutil.which("cmake") is None:
        print("WARNING: cmake is not on PATH before the documented steps run", file=sys.stderr)
    result = run(args.os, commands)
    if args.report:
        args.report.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as handle:
            handle.write(summary_markdown(result))
    print(summary_markdown(result))
    return 0 if result["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
