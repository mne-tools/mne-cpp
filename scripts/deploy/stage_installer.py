#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# Stage the Qt Installer Framework config and packages for one platform from a
# dynamic release archive, ready for binarycreator.
"""Stage the MNE-CPP installer from a dynamic release archive.

The archive is split by path into the installer components: desktop applications,
command-line tools, the C++ SDK and the shared runtime. Without --archive the
archive of --release-tag is downloaded with the GitHub CLI.

Usage
-----
    python3 scripts/deploy/stage_installer.py --platform macos --archive mne-cpp-macos-dynamic-arm64.tar.gz
    python3 scripts/deploy/stage_installer.py --platform linux --release-tag dev_build
    binarycreator -c build_installer/staging/config/config.xml -p build_installer/staging/packages MNE-CPP-Installer
"""

from __future__ import annotations

import argparse
import datetime
import re
import shutil
import subprocess
import sys
import tarfile
import tempfile
import zipfile
from pathlib import Path, PurePosixPath

REPO_ROOT = Path(__file__).resolve().parents[2]
TEMPLATE = REPO_ROOT / "scripts" / "packaging" / "installer"
ICONS = REPO_ROOT / "doc" / "website" / "static" / "img"

GUI_APPS = {"mne_scan", "mne_analyze", "mne_browse", "mne_inspect", "mne_align", "mne_anonymize"}
APP_PLUGINS = {"mne_scan_plugins", "mne_analyze_plugins"}
# Developer utilities that are built and deployed but are of no use to installer users
EXCLUDED = {"mne_screenshot_regression"}
LINUX_ICONS = {
    "mne_scan": "icon_mne_scan_256x256.png",
    "mne_analyze": "icon_mne-analyze_256x256.png",
    "mne_browse": "icon_mne-browse_256x256.png",
    "mne_inspect": "icon_mne_inspect_256x256.png",
}
ARCHIVES = {
    "linux": "{prefix}-linux-dynamic-x86_64.tar.gz",
    "macos": "{prefix}-macos-dynamic-arm64.tar.gz",
    "windows": "{prefix}-windows-dynamic-x86_64.zip",
}


def project_version() -> str:
    text = (REPO_ROOT / "src" / "CMakeLists.txt").read_text(encoding="utf-8")
    parts = [re.search(rf"^set\(MNE_CPP_VERSION_{p} (\d+)\)", text, re.M).group(1) for p in ("MAJOR", "MINOR", "PATCH")]
    return ".".join(parts)


def asset_prefix(tag: str) -> str:
    return {"dev_build": "mne-cpp-dev", "latest": "mne-cpp-latest"}.get(tag, f"mne-cpp-{tag.removeprefix('v')}")


def component_for(rel: PurePosixPath) -> str | None:
    """Installer component of a top-level entry, or of a direct child of bin/ or lib/."""
    top = rel.parts[0]
    if top in {"include", "share"}:
        return "sdk"
    if top == "lib":
        return "sdk" if len(rel.parts) > 1 and rel.parts[1] == "cmake" else "runtime"
    if top != "bin" or len(rel.parts) < 2:
        return "runtime"
    name = PurePosixPath(rel.parts[1])
    if name.suffix not in {"", ".app", ".exe"}:
        return "runtime"
    stem = name.stem if name.suffix else name.name
    if stem in EXCLUDED:
        return None
    if stem in GUI_APPS or stem in APP_PLUGINS or stem.startswith("mne_analyze_studio"):
        return "apps"
    return "tools" if stem.startswith("mne_") else "runtime"


def payload_root(extracted: Path) -> Path:
    """Directory holding bin/, which the Linux archive nests one level down."""
    if (extracted / "bin").is_dir():
        return extracted
    nested = [p.parent for p in extracted.glob("*/bin") if p.is_dir()]
    if len(nested) != 1:
        raise SystemExit(f"error: no single bin/ folder in the archive ({len(nested)} found)")
    return nested[0]


def extract(archive: Path, into: Path) -> Path:
    if archive.suffix == ".zip":
        with zipfile.ZipFile(archive) as z:
            z.extractall(into)
    else:
        with tarfile.open(archive) as t:
            t.extractall(into, filter="tar")
    return payload_root(into)


def split_payload(root: Path, packages: Path) -> dict[str, int]:
    """Move every entry of *root* into packages/org.mnecpp.<component>/data; return files per component."""
    entries = []
    for top in sorted(root.iterdir()):
        if top.name in {"bin", "lib"} and top.is_dir() and not top.is_symlink():
            entries += sorted(top.iterdir())
        else:
            entries.append(top)
    counts: dict[str, int] = {}
    for entry in entries:
        rel = PurePosixPath(entry.relative_to(root).as_posix())
        component = component_for(rel)
        if component is None:
            continue
        target = packages / f"org.mnecpp.{component}" / "data" / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(entry), str(target))
        counts[component] = counts.get(component, 0) + (sum(1 for _ in target.rglob("*")) if target.is_dir() else 1)
    return counts


def render(staging: Path, version: str, date: str) -> None:
    for path in staging.rglob("*"):
        if path.suffix in {".xml", ".qs", ".ui"} and "data" not in path.relative_to(staging).parts:
            text = path.read_text(encoding="utf-8")
            path.write_text(text.replace("@MNE_CPP_VERSION@", version).replace("@MNE_CPP_RELEASE_DATE@", date),
                            encoding="utf-8")


def stage(archive: Path, platform: str, output: Path, date: str) -> dict[str, int]:
    if output.exists():
        shutil.rmtree(output)
    shutil.copytree(TEMPLATE / "config", output / "config")
    shutil.copytree(TEMPLATE / "packages", output / "packages", ignore=shutil.ignore_patterns("data"))
    packages = output / "packages"

    with tempfile.TemporaryDirectory(dir=output) as tmp:
        counts = split_payload(extract(archive, Path(tmp)), packages)
    missing = {"apps", "tools", "sdk", "runtime"} - counts.keys()
    if missing:
        raise SystemExit(f"error: the archive has nothing for {', '.join(sorted(missing))}")

    scripts = packages / "org.mnecpp.pathconfig" / "data" / "scripts"
    scripts.mkdir(parents=True)
    script = "configure_environment.ps1" if platform == "windows" else "configure_environment.sh"
    shutil.copy2(TEMPLATE / "scripts" / script, scripts / script)
    if platform == "linux":
        icons = packages / "org.mnecpp.apps" / "data" / "resources" / "icons"
        icons.mkdir(parents=True)
        for app in GUI_APPS:
            source = ICONS / LINUX_ICONS[app] if app in LINUX_ICONS else TEMPLATE / "config" / "installer.png"
            shutil.copy2(source, icons / f"{app}.png")
    for empty in ("org.mnecpp.pathconfig", "org.mnecpp.downloads", "org.mnecpp.sampledata", "org.mnecpp.mnepython"):
        (packages / empty / "data").mkdir(exist_ok=True)

    render(output, project_version(), date)
    for junk in output.rglob(".DS_Store"):
        junk.unlink()
    return counts


def download(tag: str, platform: str, into: Path) -> Path:
    name = ARCHIVES[platform].format(prefix=asset_prefix(tag))
    subprocess.run(["gh", "release", "download", tag, "-p", name, "-D", str(into), "--clobber"], check=True)
    return into / name


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--platform", required=True, choices=sorted(ARCHIVES))
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument("--archive", type=Path, help="dynamic release archive (.tar.gz or .zip)")
    source.add_argument("--release-tag", help="download the dynamic archive of this GitHub release")
    parser.add_argument("--output-dir", type=Path, default=REPO_ROOT / "build_installer" / "staging")
    parser.add_argument("--release-date", default=datetime.date.today().isoformat())
    args = parser.parse_args(argv)

    output = args.output_dir.resolve()
    with tempfile.TemporaryDirectory() as tmp:
        archive = args.archive.resolve() if args.archive else download(args.release_tag, args.platform, Path(tmp))
        counts = stage(archive, args.platform, output, args.release_date)
    print(f"Staged MNE-CPP {project_version()} for {args.platform} in {output}")
    for component in sorted(counts):
        print(f"  org.mnecpp.{component}: {counts[component]} files")
    return 0


if __name__ == "__main__":
    sys.exit(main())
