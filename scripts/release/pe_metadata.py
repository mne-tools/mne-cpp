#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Windows product metadata of the files MNE-CPP submits to SignPath.

The SignPath Foundation signs a file only when its ``ProductName`` and
``ProductVersion`` match the restrictions of the artifact configuration in
``.signpath/artifact-configurations``. This tool applies the same
configuration before a signing request is submitted, so a violation fails
the build with the offending file named instead of a rejected request::

    python scripts/release/pe_metadata.py version
    python scripts/release/pe_metadata.py check \\
        --artifact-configuration .signpath/artifact-configurations/windows-archive.xml \\
        mne-cpp-windows-dynamic-x86_64.zip
    python scripts/release/pe_metadata.py stamp --description "MNE-CPP Installer" \\
        installerbase.exe installerbase-mne-cpp.exe

``stamp`` gives the Qt Installer Framework template the version resource
of ``cmake/mne_version_info.rc.in`` (compiled with ``rc.exe``) before
``binarycreator`` appends the payload. ``check`` and ``stamp`` use the
Windows version API and run on Windows only.
"""

from __future__ import annotations

import argparse
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
import zipfile
from dataclasses import dataclass
from pathlib import Path
from typing import Callable

REPO_ROOT = Path(__file__).resolve().parents[2]
RC_TEMPLATE = REPO_ROOT / "cmake" / "mne_version_info.rc.in"
NS = "{http://signpath.io/artifact-configuration/v1}"
RT_VERSION = 16
PRODUCT_NAME = "MNE-CPP"

Reader = Callable[[Path], dict[str, str]]


def project_version(root: Path = REPO_ROOT) -> str:
    """MAJOR.MINOR.PATCH declared in ``src/CMakeLists.txt``."""
    text = (root / "src" / "CMakeLists.txt").read_text(encoding="utf-8")
    parts = [re.search(rf"^set\(MNE_CPP_VERSION_{p} (\d+)\)", text, re.M) for p in ("MAJOR", "MINOR", "PATCH")]
    if not all(parts):
        raise SystemExit("src/CMakeLists.txt does not declare MNE_CPP_VERSION_MAJOR/MINOR/PATCH")
    return ".".join(m.group(1) for m in parts)


@dataclass
class Rule:
    """One ``<pe-file>`` element or ``<include>`` of a ``<pe-file-set>``."""

    path: str | None
    min_matches: int
    max_matches: int | None
    product_name: str | None
    product_version: str | None


def load_configuration(xml_path: Path, parameters: dict[str, str]) -> tuple[str, list[Rule]]:
    """Root element name and the PE rules of a SignPath artifact configuration, parameters resolved."""
    root = ET.parse(xml_path).getroot()
    values = {}
    for param in root.iter(f"{NS}parameter"):
        name = param.get("name")
        value = parameters.get(name, param.get("default-value"))
        if value is None and param.get("required") == "true":
            raise ValueError(f"{xml_path.name}: required parameter '{name}' not given")
        values[name] = value

    def resolve(text: str | None) -> str | None:
        if text is None:
            return None
        return re.sub(r"\$\{([\w.-]+)\}", lambda m: values.get(m.group(1)) or m.group(0), text)

    def count(value: str | None, default: int) -> int | None:
        return None if value == "unbounded" else int(value) if value is not None else default

    files = [child for child in root if child.tag != f"{NS}parameters"]
    if len(files) != 1:
        raise ValueError(f"{xml_path.name}: expected one root file element")
    top = files[0]
    rules = []
    for element in top.iter():
        tag = element.tag.removeprefix(NS)
        if tag == "pe-file":
            rules.append(Rule(None if element is top else element.get("path"),
                              count(element.get("min-matches"), 1), count(element.get("max-matches"), 1),
                              resolve(element.get("product-name")), resolve(element.get("product-version"))))
        elif tag == "pe-file-set":
            for include in element.findall(f"{NS}include"):
                rules.append(Rule(include.get("path"),
                                  count(include.get("min-matches"), 1), count(include.get("max-matches"), 1),
                                  resolve(include.get("product-name", element.get("product-name"))),
                                  resolve(include.get("product-version", element.get("product-version")))))
    for rule in rules:
        if rule.product_name is None or rule.product_version is None:
            raise ValueError(f"{xml_path.name}: {rule.path or top.tag.removeprefix(NS)} "
                             "has no product-name/product-version restriction")
    return top.tag.removeprefix(NS), rules


def glob_regex(pattern: str) -> re.Pattern[str]:
    """SignPath path wildcards: ``**`` spans directories, ``*`` and ``?`` do not; case-insensitive."""
    out = []
    segments = pattern.replace("\\", "/").split("/")
    for i, segment in enumerate(segments):
        last = i == len(segments) - 1
        if segment == "**":
            out.append("(?:[^/]+/)*")
            continue
        piece = ""
        j = 0
        while j < len(segment):
            c = segment[j]
            if c == "*":
                piece += "[^/]*"
            elif c == "?":
                piece += "[^/]"
            elif c == "[":
                end = segment.index("]", j)
                body = segment[j + 1:end]
                piece += "[^" + body[1:] + "]" if body.startswith("!") else "[" + body + "]"
                j = end
            else:
                piece += re.escape(c)
            j += 1
        out.append(piece + ("" if last else "/"))
    return re.compile("".join(out) + r"\Z", re.I)


def violations(kind: str, rules: list[Rule], names: list[str], read: Callable[[str], dict[str, str]]) -> list[str]:
    """Every way the files *names* (paths inside the artifact) break *rules*."""
    errors = []
    for rule in rules:
        matched = names[:1] if kind == "pe-file" and rule.path is None else \
            [n for n in names if glob_regex(rule.path).match(n)]
        label = rule.path or "<root>"
        if len(matched) < rule.min_matches:
            errors.append(f"{label}: matches {len(matched)} file(s), needs at least {rule.min_matches}")
        if rule.max_matches is not None and len(matched) > rule.max_matches:
            errors.append(f"{label}: matches {len(matched)} file(s), allows at most {rule.max_matches}")
        for name in matched:
            info = read(name)
            for key, expected in (("ProductName", rule.product_name), ("ProductVersion", rule.product_version)):
                actual = info.get(key)
                if actual != expected:
                    shown = "missing" if actual is None else f"'{actual}'"
                    errors.append(f"{name}: {key} is {shown}, expected '{expected}'")
    return errors


def windows_version_strings(path: Path) -> dict[str, str]:
    """StringFileInfo values of *path* through the Windows version API."""
    import ctypes
    from ctypes import wintypes

    api = ctypes.WinDLL("version")
    api.GetFileVersionInfoSizeW.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(wintypes.DWORD)]
    api.GetFileVersionInfoW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD, ctypes.c_void_p]
    api.VerQueryValueW.argtypes = [ctypes.c_void_p, wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_void_p),
                                   ctypes.POINTER(wintypes.UINT)]
    size = api.GetFileVersionInfoSizeW(str(path), None)
    if not size:
        return {}
    buffer = ctypes.create_string_buffer(size)
    if not api.GetFileVersionInfoW(str(path), 0, size, buffer):
        return {}
    pointer, length = ctypes.c_void_p(), wintypes.UINT()
    languages = []
    if api.VerQueryValueW(buffer, "\\VarFileInfo\\Translation", ctypes.byref(pointer), ctypes.byref(length)):
        words = ctypes.cast(pointer, ctypes.POINTER(wintypes.WORD))
        languages = [(words[i], words[i + 1]) for i in range(0, length.value // 2, 2)]
    result = {}
    for key in ("CompanyName", "FileDescription", "FileVersion", "ProductName", "ProductVersion"):
        for language, codepage in languages:
            query = f"\\StringFileInfo\\{language:04x}{codepage:04x}\\{key}"
            if api.VerQueryValueW(buffer, query, ctypes.byref(pointer), ctypes.byref(length)) and length.value:
                result[key] = ctypes.wstring_at(pointer, length.value).rstrip("\0")
                break
    return result


def check(artifact: Path, configuration: Path, version: str, reader: Reader = windows_version_strings) -> list[str]:
    """Violations of *configuration* by *artifact* (a ``.zip`` or a single PE file)."""
    kind, rules = load_configuration(configuration, {"version": version})
    if kind == "pe-file":
        return violations(kind, rules, [artifact.name], lambda _name: reader(artifact))
    if kind != "zip-file":
        raise ValueError(f"{configuration.name}: unsupported root element <{kind}>")
    with zipfile.ZipFile(artifact) as archive, tempfile.TemporaryDirectory() as tmp:
        names = [info.filename for info in archive.infolist() if not info.is_dir()]
        return violations(kind, rules, names, lambda name: reader(Path(archive.extract(name, tmp))))


def configure_rc(version: str, name: str, description: str, filetype: str) -> str:
    """``cmake/mne_version_info.rc.in`` with its ``@VAR@`` placeholders filled like CMake does."""
    major, minor, patch = version.split(".")
    values = {"MNE_RC_VERSION": version, "MNE_RC_VERSION_COMMA": f"{major},{minor},{patch},0",
              "MNE_RC_FILETYPE": filetype, "MNE_RC_INTERNAL_NAME": name, "MNE_RC_DESCRIPTION": description}
    text = RC_TEMPLATE.read_text(encoding="utf-8")
    missing = set(re.findall(r"@(\w+)@", text)) - values.keys()
    if missing:
        raise ValueError(f"unfilled placeholders in {RC_TEMPLATE.name}: {sorted(missing)}")
    return re.sub(r"@(\w+)@", lambda m: values[m.group(1)], text)


def res_resources(data: bytes) -> list[tuple[int | str, int | str, int, bytes]]:
    """(type, name, language, payload) of every entry of a compiled ``.res`` file."""
    entries = []
    offset = 0
    while offset + 8 <= len(data):
        size, header = struct.unpack_from("<II", data, offset)
        if header < 16:
            raise ValueError(f"malformed .res entry at byte {offset}")
        position = offset + 8
        ids: list[int | str] = []
        for _ in range(2):
            if struct.unpack_from("<H", data, position)[0] == 0xFFFF:
                ids.append(struct.unpack_from("<H", data, position + 2)[0])
                position += 4
            else:
                end = position
                while struct.unpack_from("<H", data, end)[0]:
                    end += 2
                ids.append(data[position:end].decode("utf-16-le"))
                position = end + 2
        position += -(position - offset) % 4
        language = struct.unpack_from("<H", data, position + 6)[0]
        if size:
            entries.append((ids[0], ids[1], language, data[offset + header:offset + header + size]))
        offset += header + size
        offset += -offset % 4
    return entries


def _rc_exe() -> str:
    found = shutil.which("rc")
    if found:
        return found
    kits = Path(r"C:\Program Files (x86)\Windows Kits\10\bin")
    candidates = sorted(kits.glob("*/x64/rc.exe"), reverse=True)
    if not candidates:
        raise SystemExit("rc.exe not found")
    return str(candidates[0])


def stamp(source: Path, target: Path, version: str, description: str) -> None:
    """Copy *source* to *target* with the MNE-CPP version resource replacing any existing one."""
    import ctypes
    from ctypes import wintypes

    with tempfile.TemporaryDirectory() as tmp:
        rc, res = Path(tmp) / "version.rc", Path(tmp) / "version.res"
        rc.write_text(configure_rc(version, target.stem, description, "0x1L"), encoding="utf-8")
        subprocess.run([_rc_exe(), "/nologo", "/fo", str(res), str(rc)], check=True)
        payload = next(p for t, _n, _l, p in res_resources(res.read_bytes()) if t == RT_VERSION)
    shutil.copyfile(source, target)

    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.LoadLibraryExW.restype = wintypes.HMODULE
    kernel.LoadLibraryExW.argtypes = [wintypes.LPCWSTR, wintypes.HANDLE, wintypes.DWORD]
    kernel.FreeLibrary.argtypes = [wintypes.HMODULE]
    kernel.BeginUpdateResourceW.restype = wintypes.HANDLE
    kernel.BeginUpdateResourceW.argtypes = [wintypes.LPCWSTR, wintypes.BOOL]
    kernel.UpdateResourceW.argtypes = [wintypes.HANDLE, ctypes.c_void_p, ctypes.c_void_p, wintypes.WORD,
                                       ctypes.c_void_p, wintypes.DWORD]
    kernel.EndUpdateResourceW.argtypes = [wintypes.HANDLE, wintypes.BOOL]
    existing: list[tuple[int, int]] = []
    language_callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE, ctypes.c_void_p, ctypes.c_void_p,
                                           wintypes.WORD, ctypes.c_void_p)
    name_callback = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HMODULE, ctypes.c_void_p, ctypes.c_void_p,
                                       ctypes.c_void_p)
    kernel.EnumResourceNamesW.argtypes = [wintypes.HMODULE, ctypes.c_void_p, name_callback, ctypes.c_void_p]
    kernel.EnumResourceLanguagesW.argtypes = [wintypes.HMODULE, ctypes.c_void_p, ctypes.c_void_p,
                                              language_callback, ctypes.c_void_p]

    @language_callback
    def on_language(_module, _type, name, language, _param):
        existing.append((name, language))
        return True

    @name_callback
    def on_name(module, type_, name, _param):
        kernel.EnumResourceLanguagesW(module, type_, name, on_language, None)
        return True

    module = kernel.LoadLibraryExW(str(target), None, 0x2)  # LOAD_LIBRARY_AS_DATAFILE
    if not module:
        raise OSError(ctypes.get_last_error(), f"cannot load {target}")
    kernel.EnumResourceNamesW(module, RT_VERSION, on_name, None)
    kernel.FreeLibrary(module)
    if any(name > 0xFFFF for name, _language in existing):
        raise ValueError(f"{source.name}: named RT_VERSION resources are not supported")

    handle = kernel.BeginUpdateResourceW(str(target), False)
    if not handle:
        raise OSError(ctypes.get_last_error(), f"BeginUpdateResource failed for {target}")
    for name, language in existing:
        kernel.UpdateResourceW(handle, RT_VERSION, name, language, None, 0)
    buffer = ctypes.create_string_buffer(payload, len(payload))
    if not kernel.UpdateResourceW(handle, RT_VERSION, 1, 0x0409, buffer, len(payload)):
        raise OSError(ctypes.get_last_error(), "UpdateResource failed")
    if not kernel.EndUpdateResourceW(handle, False):
        raise OSError(ctypes.get_last_error(), f"EndUpdateResource failed for {target}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("version", help="print the release version declared in src/CMakeLists.txt")
    check_parser = commands.add_parser("check", help="check an artifact against a SignPath artifact configuration")
    check_parser.add_argument("--artifact-configuration", type=Path, required=True)
    check_parser.add_argument("--version", default=None, help="expected product version (default: project version)")
    check_parser.add_argument("artifact", type=Path)
    stamp_parser = commands.add_parser("stamp", help="copy a PE file and give it the MNE-CPP version resource")
    stamp_parser.add_argument("--version", default=None)
    stamp_parser.add_argument("--description", required=True)
    stamp_parser.add_argument("source", type=Path)
    stamp_parser.add_argument("target", type=Path)
    args = parser.parse_args(argv)

    if args.command == "version":
        print(project_version())
        return 0
    if sys.platform != "win32":
        raise SystemExit(f"'{args.command}' reads Windows version resources and runs on Windows only")
    version = args.version or project_version()
    if args.command == "stamp":
        stamp(args.source, args.target, version, args.description)
        print(f"{args.target}: ProductName {PRODUCT_NAME}, ProductVersion {version}")
        return 0
    errors = check(args.artifact, args.artifact_configuration, version)
    for error in errors:
        print(f"::error::{args.artifact.name}: {error}")
    if not errors:
        print(f"PASS: {args.artifact.name} satisfies {args.artifact_configuration.name} (ProductVersion {version})")
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
