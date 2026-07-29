#!/usr/bin/env python3
"""Report line coverage using the same scope Codecov does.

Codecov's number comes from the Linux CI job, which runs gcov via fastcov with
``--process-gcno`` and then filters to ``src/libraries``, ``src/applications``
and ``src/tools``.  The ``--process-gcno`` part matters: it pulls in objects
that were compiled but never executed, so a source file no test ever reaches is
counted at 0 percent instead of being left out of the denominator entirely.

Measuring locally with llvm-cov and only passing the library dylibs leaves those
files out and reports a substantially higher number than Codecov for the same
tree.  This script closes that gap by collecting every instrumented binary in
the build output, not just the ones under lib/, which is the local equivalent of
``--process-gcno``.

Usage
-----
    # after running the instrumented suite and merging the profile
    python3 tools/quality/coverage_report.py /tmp/cov.profdata

    # show the biggest gaps rather than the module summary
    python3 tools/quality/coverage_report.py /tmp/cov.profdata --top 30
    python3 tools/quality/coverage_report.py /tmp/cov.profdata --zero-only

To produce the profile in the first place::

    cmake -S . -B build/cov -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON \\
        -DCMAKE_PREFIX_PATH="$PWD/src/external/qt/dynamic;$PWD/src/external/eigen" \\
        -DCMAKE_CXX_FLAGS="-fprofile-instr-generate -fcoverage-mapping -O1 -g" \\
        -DCMAKE_EXE_LINKER_FLAGS="-fprofile-instr-generate" \\
        -DCMAKE_SHARED_LINKER_FLAGS="-fprofile-instr-generate"
    cmake --build build/cov -j 8
    LLVM_PROFILE_FILE='/tmp/prof/%p.profraw' ctest --test-dir build/cov -j 4
    xcrun llvm-profdata merge -sparse /tmp/prof/*.profraw -o /tmp/cov.profdata

Note that the local figure will still not match Codecov exactly.  gcov and
llvm-cov disagree about what counts as a line, and the CI build uses GCC on
Linux while this uses clang on the developer machine.  The point is to make the
local number track the same denominator so that a change measured here moves
Codecov in the same direction and by a comparable amount.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from collections import defaultdict
from pathlib import Path

# The three trees Codecov keeps, straight from the --include list in
# .github/workflows/_reusable-tests.yml.
INCLUDED_ROOTS = ("src/libraries/", "src/applications/", "src/tools/")

# Generated and third-party code is excluded from the report the same way the CI
# job excludes it, so the denominator reflects code someone actually maintains.
EXCLUDED_PATTERN = re.compile(r"(/external/|autogen|_autogen|/usr/|Xcode|/testframes/|\.moc$|moc_)")


def find_binaries(build_root: Path) -> list[Path]:
    """Collect every instrumented binary, which is the local --process-gcno.

    Passing only the dylibs under lib/ silently drops the application bundles
    and the plugin libraries, and with them every source file that only those
    binaries contain.  Those files then vanish from the denominator instead of
    counting as uncovered, which is exactly the discrepancy this script exists
    to avoid.
    """
    binaries: list[Path] = []

    for path in sorted(build_root.rglob("*.dylib")):
        binaries.append(path)

    for path in sorted(build_root.rglob("*.so")):
        binaries.append(path)

    bin_dir = build_root / "bin"
    if bin_dir.is_dir():
        for path in sorted(bin_dir.iterdir()):
            if path.is_file() and path.stat().st_mode & 0o111:
                binaries.append(path)

        # Executables inside .app bundles are not reachable by the loop above.
        for app in sorted(bin_dir.glob("*.app")):
            macos_dir = app / "Contents" / "MacOS"
            if macos_dir.is_dir():
                for path in sorted(macos_dir.iterdir()):
                    if path.is_file() and path.stat().st_mode & 0o111:
                        binaries.append(path)

    return binaries


def export_summary(binaries: list[Path], profdata: Path) -> dict:
    """Run llvm-cov export and return the parsed per-file summary."""
    cmd = ["xcrun", "llvm-cov", "export", "-instr-profile", str(profdata),
           "-format=text", "-summary-only"]
    for binary in binaries:
        cmd += ["-object", str(binary)]

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0 or not result.stdout.strip():
        sys.exit(f"llvm-cov export failed:\n{result.stderr.strip()[:2000]}")

    return json.loads(result.stdout)


def collect(data: dict) -> list[tuple[str, int, int]]:
    """Reduce the export to (path, total_lines, covered_lines) in Codecov scope."""
    seen: dict[str, tuple[int, int]] = {}

    for entry in data["data"][0]["files"]:
        name = entry["filename"]
        if EXCLUDED_PATTERN.search(name):
            continue

        index = -1
        for root in INCLUDED_ROOTS:
            index = name.find(root)
            if index != -1:
                break
        if index == -1:
            continue

        relative = name[index:]
        lines = entry["summary"]["lines"]

        # A file compiled into several binaries appears more than once. Keep the
        # best result, since a line covered anywhere is covered.
        previous = seen.get(relative)
        if previous is None or lines["covered"] > previous[1]:
            seen[relative] = (lines["count"], lines["covered"])

    return [(path, total, covered) for path, (total, covered) in seen.items()]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("profdata", type=Path, help="merged .profdata file")
    parser.add_argument("--build-root", type=Path, default=Path("out/Debug"),
                        help="directory holding the instrumented binaries")
    parser.add_argument("--top", type=int, default=0,
                        help="list the N files with the most uncovered lines")
    parser.add_argument("--zero-only", action="store_true",
                        help="restrict the file listing to files at 0 percent")
    parser.add_argument("--target", type=float, default=50.0,
                        help="coverage target used for the shortfall line")
    args = parser.parse_args()

    if not args.profdata.is_file():
        return f"no such profile: {args.profdata}"

    binaries = find_binaries(args.build_root)
    if not binaries:
        return f"no instrumented binaries under {args.build_root}"

    files = collect(export_summary(binaries, args.profdata))
    if not files:
        return "no files matched the Codecov scope; is this the right build root?"

    total = sum(f[1] for f in files)
    covered = sum(f[2] for f in files)
    percent = 100.0 * covered / total

    print(f"objects scanned      {len(binaries)}")
    print(f"files in scope       {len(files)}")
    print(f"lines                {covered} / {total}")
    print(f"line coverage        {percent:.2f}%")

    shortfall = int(args.target / 100.0 * total) - covered
    if shortfall > 0:
        print(f"to reach {args.target:g}%          {shortfall} more lines")
    else:
        print(f"target {args.target:g}% met       {-shortfall} lines clear")

    modules: dict[str, list[int]] = defaultdict(lambda: [0, 0])
    for path, file_total, file_covered in files:
        parts = path.split("/")
        key = "/".join(parts[:3]) if len(parts) >= 3 else path
        modules[key][0] += file_total
        modules[key][1] += file_covered

    print("\nby module, most uncovered first")
    for key, (mod_total, mod_covered) in sorted(
            modules.items(), key=lambda kv: kv[1][1] - kv[1][0]):
        gap = mod_total - mod_covered
        if gap:
            print(f"  {gap:7d} uncovered  {100.0 * mod_covered / mod_total:5.1f}%  {key}")

    if args.top or args.zero_only:
        listing = [f for f in files if f[2] == 0] if args.zero_only else files
        listing = [f for f in listing if f[1] - f[2] > 0]
        listing.sort(key=lambda f: f[2] - f[1])
        heading = "0% files" if args.zero_only else "files"
        print(f"\n{heading}, most uncovered first")
        for path, file_total, file_covered in listing[:args.top or 20]:
            pct = 100.0 * file_covered / file_total if file_total else 0.0
            print(f"  {file_total - file_covered:6d} uncovered  {pct:5.1f}%  {path}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
