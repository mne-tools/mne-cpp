# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Tests for scripts/deploy/stage_installer.py.
"""The installer components get the right parts of a release archive."""

from __future__ import annotations

import importlib.util
import re
import tarfile
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile
from pathlib import Path, PurePosixPath

REPO_ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location("stage_installer", REPO_ROOT / "scripts/deploy/stage_installer.py")
stage = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(stage)
TEMPLATE = REPO_ROOT / "scripts/packaging/installer"

MACOS = ["bin/mne_scan.app/Contents/MacOS/mne_scan", "bin/mne_analyze_studio_workbench", "bin/mne_show_fiff",
         "bin/mne_screenshot_regression", "bin/resources/a.txt", "lib/libmne_fiff.dylib",
         "lib/QtCore.framework/QtCore", "lib/cmake/MNE-CPP/MNECPPConfig.cmake", "include/fiff/fiff.h",
         "resources/mne_scan/x.json"]
WINDOWS = ["bin/mne_scan.exe", "bin/mne_fiff.dll", "bin/platforms/qwindows.dll", "bin/mne_rt_server_plugins/a.dll",
           "bin/mne_show_fiff.exe", "include/fiff/fiff.h", "lib/mne_fiff.lib", "lib/cmake/MNE-CPP/x.cmake"]


def _components(packages: Path) -> dict[str, set[str]]:
    return {d.name.removeprefix("org.mnecpp."): {p.relative_to(d / "data").as_posix() for p in (d / "data").rglob("*")
                                                 if p.is_file()} for d in packages.iterdir() if (d / "data").is_dir()}


class StageInstallerTest(unittest.TestCase):
    def test_component_for_splits_by_path(self) -> None:
        cases = {
            "bin/mne_inspect.app": "apps", "bin/mne_scan_plugins": "apps", "bin/mne_analyze_studio_workbench": "apps",
            "bin/mne_show_fiff": "tools", "bin/mne_show_fiff.exe": "tools", "bin/mne_rt_server_plugins": "tools",
            "bin/mne_fiff.dll": "runtime", "bin/platforms": "runtime", "bin/qt.conf": "runtime",
            "lib/libmne_fiff.so": "runtime", "lib/cmake": "sdk", "include": "sdk", "share": "sdk",
            "plugins": "runtime", "resources": "runtime", "bin/mne_screenshot_regression": None,
        }
        for path, expected in cases.items():
            self.assertEqual(stage.component_for(PurePosixPath(path)), expected, path)

    def test_stage_macos_tarball_with_nested_root(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            archive = tmp / "mac.tar.gz"
            with tarfile.open(archive, "w:gz") as tar:
                for rel in MACOS:
                    src = tmp / "src" / "mne-cpp" / rel
                    src.parent.mkdir(parents=True, exist_ok=True)
                    src.write_text(rel)
                tar.add(tmp / "src" / "mne-cpp", arcname="mne-cpp")
            stage.stage(archive, "macos", tmp / "out", "2026-10-20")
            got = _components(tmp / "out" / "packages")
            xml = ET.parse(tmp / "out" / "packages" / "org.mnecpp.apps" / "meta" / "package.xml").getroot()
            config = (tmp / "out" / "config" / "controller.qs").read_text()
        self.assertEqual(got["apps"], {"bin/mne_scan.app/Contents/MacOS/mne_scan", "bin/mne_analyze_studio_workbench"})
        self.assertEqual(got["tools"], {"bin/mne_show_fiff"})
        self.assertEqual(got["sdk"], {"lib/cmake/MNE-CPP/MNECPPConfig.cmake", "include/fiff/fiff.h"})
        self.assertEqual(got["runtime"], {"bin/resources/a.txt", "lib/libmne_fiff.dylib", "lib/QtCore.framework/QtCore",
                                          "resources/mne_scan/x.json"})
        self.assertEqual(got["pathconfig"], {"scripts/configure_environment.sh"})
        self.assertEqual(xml.findtext("Version"), stage.project_version())
        self.assertEqual(xml.findtext("ReleaseDate"), "2026-10-20")
        self.assertNotIn("@MNE_CPP_", config)

    def test_stage_windows_zip(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp = Path(tmp)
            archive = tmp / "win.zip"
            with zipfile.ZipFile(archive, "w") as z:
                for rel in WINDOWS:
                    z.writestr(rel, rel)
            stage.stage(archive, "windows", tmp / "out", "2026-10-20")
            got = _components(tmp / "out" / "packages")
        self.assertEqual(got["apps"], {"bin/mne_scan.exe"})
        self.assertEqual(got["tools"], {"bin/mne_show_fiff.exe", "bin/mne_rt_server_plugins/a.dll"})
        self.assertEqual(got["runtime"], {"bin/mne_fiff.dll", "bin/platforms/qwindows.dll", "lib/mne_fiff.lib"})
        self.assertEqual(got["pathconfig"], {"scripts/configure_environment.ps1"})

    def test_package_metadata_is_consistent(self) -> None:
        packages = {d.name: d / "meta" for d in (TEMPLATE / "packages").iterdir()}
        profile_script = (packages["org.mnecpp.runtime"] / "installscript.qs").read_text()
        for name, meta in packages.items():
            xml = ET.parse(meta / "package.xml").getroot()
            for dependency in filter(None, (xml.findtext("Dependencies") or "").split(",")):
                self.assertIn(dependency.strip(), packages, f"{name} depends on an unknown component")
            for tag, folder in (("Script", None), ("UserInterfaces/UserInterface", None), ("Licenses/License", "file")):
                for element in xml.findall(tag):
                    self.assertTrue((meta / (element.get(folder) if folder else element.text)).is_file(), name)
            if name != "org.mnecpp.runtime":
                self.assertIn(f'"{name}"', profile_script, f"{name} is missing from the setup types")
        config = ET.parse(TEMPLATE / "config" / "config.xml").getroot()
        referenced = [config.findtext(t) for t in ("ControlScript", "StyleSheet", "PageListPixmap", "InstallerWindowIcon")]
        referenced += [i.text for i in config.iter("Image")]
        for name in referenced + ["installer.icns", "installer.ico", "sidebar@2x.png"]:
            self.assertTrue((TEMPLATE / "config" / name).is_file(), name)
        # binarycreator -r config/hero.qrc compiles the step panel traces and the check mark
        qrc = [f.text for f in ET.parse(TEMPLATE / "config" / "hero.qrc").getroot().iter("file")]
        self.assertIn("hero/side@2x.png", qrc)
        for name in qrc:
            self.assertTrue((TEMPLATE / "config" / name).is_file(), name)
        scripts = (TEMPLATE / "config" / "controller.qs").read_text() + profile_script
        scripts += (TEMPLATE / "config" / "style.qss").read_text()
        for used in re.findall(r":/mnecpp/(hero/[\w.]+)", scripts):
            self.assertIn(used, qrc)


if __name__ == "__main__":
    unittest.main()
