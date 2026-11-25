#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    SOUP inventory and CycloneDX SBOM of MNE-CPP.
"""SOUP inventory and SBOM of MNE-CPP.

``soup_catalog.json`` holds the curated facts of every third-party component (license, purpose, linkage, risk
and mitigation). This tool adds the versions, read from where they are pinned (``src/3rdparty/external_deps.env``,
CMake files, vendored headers, CI workflows) or, with ``--build-dir``, from a configured build, and writes:

* ``doc/compliance/soup.json``        the machine-readable inventory
* ``doc/compliance/sbom.cdx.json``    a CycloneDX 1.5 SBOM of the shipped components
* ``doc/website/docs/development/soup.mdx``   the documentation page

``--check`` fails when a committed file is stale, or when a library links a third-party CMake target (from
``dependency_policy.json``'s ``allowed_external``) that no catalog entry claims, so no SOUP enters unlisted.

Usage
-----
    python3 scripts/compliance/generate_soup.py               # rewrite the committed files
    python3 scripts/compliance/generate_soup.py --check
    python3 scripts/compliance/generate_soup.py --build-dir build/developer-dynamic --output-dir /tmp/soup
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any

REPO_ROOT = Path(__file__).resolve().parents[2]
CATALOG = Path(__file__).resolve().parent / "soup_catalog.json"
DEPENDENCY_POLICY = REPO_ROOT / "scripts" / "quality" / "dependency_policy.json"
ENV_FILE = REPO_ROOT / "src" / "3rdparty" / "external_deps.env"
OUTPUTS = {
    "json": Path("doc/compliance/soup.json"),
    "sbom": Path("doc/compliance/sbom.cdx.json"),
    "mdx": Path("doc/website/docs/development/soup.mdx"),
}


def read_env(path: Path = ENV_FILE) -> dict[str, str]:
    values = {}
    for line in path.read_text(encoding="utf-8").splitlines():
        match = re.fullmatch(r"\s*([A-Z_][A-Z0-9_]*)=(.*?)\s*", line)
        if match:
            values[match.group(1)] = match.group(2).strip("\"'")
    return values


def read_cache(build_dir: Path) -> dict[str, str]:
    cache = {}
    for line in (build_dir / "CMakeCache.txt").read_text(encoding="utf-8", errors="replace").splitlines():
        match = re.match(r"([^#/][^:=]*):[A-Z]+=(.*)$", line)
        if match:
            cache[match.group(1)] = match.group(2)
    return cache


def pinned_version(spec: dict[str, Any], root: Path, env: dict[str, str]) -> str:
    """The version where the repository pins it."""
    if "literal" in spec:
        return spec["literal"]
    if "env" in spec:
        return env[spec["env"]]
    if "cmake_set" in spec:
        text = (root / spec["cmake_set"]).read_text(encoding="utf-8")
        return re.search(rf'set\(\s*{spec["variable"]}\s+"([^"]+)"', text).group(1)
    if "cmake_minimum" in spec:
        text = (root / spec["cmake_minimum"]).read_text(encoding="utf-8")
        return re.search(r"cmake_minimum_required\(\s*VERSION\s+([\d.]+)", text).group(1)
    if "header" in spec or "workflow" in spec:
        text = (root / (spec.get("header") or spec["workflow"])).read_text(encoding="utf-8")
        found = re.findall(spec["regex"], text)
        if not found:
            raise ValueError(f"no version matching {spec['regex']} in {spec.get('header') or spec['workflow']}")
        return spec.get("join", ".").join(found) if "join" in spec else found[0]
    raise ValueError(f"unknown version source {spec}")


def build_version(spec: dict[str, Any] | None, cache: dict[str, str]) -> str | None:
    """The version a configured build actually uses, or None when the build does not say."""
    if not spec:
        return None
    if "cmake_config_version" in spec:
        directory = cache.get(spec["cmake_config_version"])
        config = Path(directory) / spec["file"] if directory else None
        if config and config.exists():
            match = re.search(r'set\(PACKAGE_VERSION "([^"]+)"\)', config.read_text(encoding="utf-8"))
            return match.group(1) if match else None
        return None
    if "cmake_cache_header" in spec:
        directory = cache.get(spec["cmake_cache_header"])
        header = Path(directory) / spec["file"] if directory and "NOTFOUND" not in directory else None
        if header and header.exists():
            match = re.search(spec["regex"], header.read_text(encoding="utf-8", errors="replace"))
            return match.group(1) if match else None
        return None
    return None


def compiler(build_dir: Path, cache: dict[str, str]) -> dict[str, str] | None:
    for config in sorted(build_dir.glob("CMakeFiles/*/CMakeCXXCompiler.cmake")):
        text = config.read_text(encoding="utf-8")
        ident = re.search(r'set\(CMAKE_CXX_COMPILER_ID "([^"]*)"\)', text)
        version = re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]*)"\)', text)
        if ident and version:
            return {"name": f"C++ compiler ({ident.group(1)})", "version": version.group(1),
                    "path": cache.get("CMAKE_CXX_COMPILER", "")}
    return None


SHIPPED_SOURCES = ("src/libs", "src/apps", "src/tools")
_COMPONENT_LISTS = re.compile(
    r"(?:set|list)\(\s*(?:APPEND\s+)?\w*QT\w*_COMPONENTS\s+([^)]*)\)"  # set(QT_REQUIRED_COMPONENTS Core Gui)
    r"|find_package\(\s*(?:QT|Qt6|Qt\$\{QT_VERSION_MAJOR\})\b[^)]*?\bCOMPONENTS\s+([^)]*)\)")


def requested_qt_modules(root: Path = REPO_ROOT) -> list[str]:
    """Qt modules the CMakeLists of the shipped libraries, applications and tools request."""
    modules: set[str] = set()
    files: list[Path] = []
    for directory in SHIPPED_SOURCES:
        files += sorted((root / directory).rglob("CMakeLists.txt"))
    for path in files:
        if not path.exists() or "3rdparty" in path.parts:
            continue
        text = re.sub(r"#[^\n]*", "", path.read_text(encoding="utf-8", errors="replace"))
        for match in _COMPONENT_LISTS.finditer(text):
            for word in (match.group(1) or match.group(2)).split():
                if re.fullmatch(r"[A-Z][A-Za-z0-9]*", word) and word not in {"REQUIRED", "OPTIONAL_COMPONENTS"}:
                    modules.add(word)
    return sorted(modules)


def found_qt_modules(cache: dict[str, str]) -> list[str]:
    """Qt modules a configured build found (Qt6<Module>_DIR), without the build-time tool packages."""
    modules = set()
    for key, value in cache.items():
        match = re.fullmatch(r"Qt6([A-Z][A-Za-z0-9]*)_DIR", key)
        if match and value and "NOTFOUND" not in value and not match.group(1).endswith("Tools"):
            modules.add(match.group(1))
    return sorted(modules)


def unclaimed_targets(catalog: dict[str, Any], policy_file: Path = DEPENDENCY_POLICY) -> list[str]:
    """Third-party CMake targets the libraries may link that no catalog entry claims."""
    allowed = json.loads(policy_file.read_text(encoding="utf-8"))["allowed_external"]
    claimed = {target for entry in catalog["entries"] for target in entry["cmake_targets"]}
    return sorted(set(allowed) - claimed)


def inventory(catalog: dict[str, Any], root: Path = REPO_ROOT, build_dir: Path | None = None) -> dict[str, Any]:
    env = read_env(root / "src" / "3rdparty" / "external_deps.env")
    cache = read_cache(build_dir) if build_dir else {}
    components = []
    for entry in catalog["entries"]:
        item = {key: entry[key] for key in ("id", "name", "supplier", "license", "url", "linkage", "shipped",
                                             "used_by", "purpose", "risk", "mitigation", "safety_relevant")}
        item["version"] = pinned_version(entry["version"], root, env)
        if entry.get("modules") == "qt":
            item["modules"] = requested_qt_modules(root)
        if build_dir:
            built = build_version(entry.get("build_version"), cache)
            if built:
                item["build_version"] = built
            if entry.get("modules") == "qt":
                item["build_modules"] = found_qt_modules(cache)
        components.append(item)
    toolchain = [{"name": tool["name"], "version": pinned_version(tool["version"], root, env)}
                 for tool in catalog["toolchain"]]
    result: dict[str, Any] = {"generated_by": "scripts/compliance/generate_soup.py",
                              "catalog": "scripts/compliance/soup_catalog.json",
                              "components": components, "toolchain": toolchain}
    if build_dir:
        result["build"] = {"directory": str(build_dir), "compiler": compiler(build_dir, cache)}
    return result


def cyclonedx(report: dict[str, Any], catalog: dict[str, Any]) -> dict[str, Any]:
    """A CycloneDX 1.5 SBOM of the components that ship with or inside MNE-CPP."""
    purls = {entry["id"]: entry["purl"] for entry in catalog["entries"]}
    components = []
    for item in report["components"]:
        if not item["shipped"]:
            continue
        version = item.get("build_version", item["version"])
        component: dict[str, Any] = {"type": "library", "bom-ref": item["id"], "name": item["name"],
                                     "version": version, "supplier": {"name": item["supplier"]},
                                     "licenses": [{"expression": item["license"]}],
                                     "description": item["purpose"]}
        if purls[item["id"]]:
            component["purl"] = purls[item["id"]].format(version=version)
        if item["url"]:
            component["externalReferences"] = [{"type": "website", "url": item["url"]}]
        modules = item.get("modules")
        if modules:
            component["components"] = [{"type": "library", "bom-ref": f"{item['id']}-{module.lower()}",
                                        "name": f"{item['name']} {module}", "version": version}
                                       for module in modules]
        components.append(component)
    return {"bomFormat": "CycloneDX", "specVersion": "1.5", "version": 1,
            "metadata": {"component": {"type": "application", "bom-ref": "mne-cpp", "name": "MNE-CPP",
                                       "licenses": [{"expression": "BSD-3-Clause"}]},
                         "tools": {"components": [{"type": "application", "name": "generate_soup.py"}]}},
            "components": components,
            "dependencies": [{"ref": "mne-cpp", "dependsOn": [c["bom-ref"] for c in components]}]}


def _cell(text: str) -> str:
    return text.replace("|", "\\|").replace("\n", " ")


def render_mdx(report: dict[str, Any]) -> str:
    lines = ["---", "id: soup", 'title: "Third-Party Software (SOUP)"', "sidebar_label: Third-Party Software", "---",
             "", "{/* Generated by scripts/compliance/generate_soup.py from scripts/compliance/soup_catalog.json. "
             "Do not edit. */}", "",
             "# Third-party software (SOUP)", "",
             "Every third-party component that MNE-CPP builds, ships, loads at run time or is built with, as IEC 62304 "
             "calls for software of unknown provenance: version, license, purpose, what fails if it fails, and how "
             "MNE-CPP detects or contains that. The machine-readable inventory is "
             "[`doc/compliance/soup.json`](https://github.com/mne-tools/mne-cpp/blob/staging/doc/compliance/soup.json), "
             "the SBOM of the shipped components "
             "[`doc/compliance/sbom.cdx.json`](https://github.com/mne-tools/mne-cpp/blob/staging/doc/compliance/sbom.cdx.json) "
             "(CycloneDX 1.5). CI fails when a library links a third-party CMake target that the catalog does not "
             "list.", "",
             "## Components", "",
             "| Component | Version | License | Linkage | Safety-relevant |", "|---|---|---|---|---|"]
    for item in report["components"]:
        lines.append(f"| {_cell(item['name'])} | {_cell(item['version'])} | `{item['license']}` "
                     f"| {_cell(item['linkage'])} | {'yes' if item['safety_relevant'] else 'no'} |")
    lines += ["", "## Purpose, risk and mitigation", ""]
    for item in report["components"]:
        lines += [f"### {item['name']}", "",
                  f"- **Used by:** {item['used_by']}",
                  *([f"- **Modules:** {', '.join(item['modules'])} (requested by the CMake files of the libraries, "
                     "applications and tools)"] if item.get("modules") else []),
                  f"- **Purpose:** {item['purpose']}",
                  f"- **If it fails:** {item['risk']}",
                  f"- **Mitigation:** {item['mitigation']}", ""]
    lines += ["## Pinned toolchain", "", "| Tool | Version |", "|---|---|"]
    lines += [f"| {_cell(t['name'])} | {_cell(t['version'])} |" for t in report["toolchain"]]
    return "\n".join(lines) + "\n"


def render(root: Path = REPO_ROOT, build_dir: Path | None = None) -> dict[Path, str]:
    catalog = json.loads((root / "scripts" / "compliance" / "soup_catalog.json").read_text(encoding="utf-8"))
    report = inventory(catalog, root, build_dir)
    return {OUTPUTS["json"]: json.dumps(report, indent=2, ensure_ascii=False) + "\n",
            OUTPUTS["sbom"]: json.dumps(cyclonedx(report, catalog), indent=2, ensure_ascii=False) + "\n",
            OUTPUTS["mdx"]: render_mdx(report)}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="fail if a committed file is stale or a target unlisted")
    parser.add_argument("--build-dir", type=Path, help="also record the versions a configured build uses")
    parser.add_argument("--output-dir", type=Path, default=REPO_ROOT, help="where to write (default: the repository)")
    args = parser.parse_args(argv)

    catalog = json.loads(CATALOG.read_text(encoding="utf-8"))
    problems = [f"third-party CMake target {t} is not in scripts/compliance/soup_catalog.json"
                for t in unclaimed_targets(catalog)]
    files = render(REPO_ROOT, None if args.check else args.build_dir)
    if args.check:
        problems += [f"{path} is stale; run scripts/compliance/generate_soup.py" for path, text in files.items()
                     if not (REPO_ROOT / path).exists() or (REPO_ROOT / path).read_text(encoding="utf-8") != text]
    else:
        for path, text in files.items():
            target = args.output_dir / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(text, encoding="utf-8")
            print(f"wrote {target}")
    for problem in problems:
        print(f"FAIL: {problem}")
    if args.check and not problems:
        print(f"PASS: {len(catalog['entries'])} SOUP entries cover every third-party target; the files are current.")
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())
