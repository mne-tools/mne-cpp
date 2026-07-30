#!/usr/bin/env python3
# =============================================================================================================
#
# @file     audit_dependencies.py
# @author   Christoph Dinh <christoph.dinh@mne-cpp.org>
# @since    2.4.0
# @date     July, 2026
#
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#
# @brief    Generate and police the library dependency graph.
#
# =============================================================================================================
"""Audit the dependency architecture of ``src/libraries``.

Layering is the kind of property that is obvious while it holds and expensive
once it does not.  A single edge from a low-level library back up into a
high-level one turns an ordered stack into a knot, and it is almost never added
deliberately: it arrives as a convenient include in a hurry and is noticed years
later when someone tries to reuse a component on its own.

This tool reads the dependency graph out of the CMake files, checks it against a
declared layering, and fails on anything that breaks it:

* a library that no layer claims, or that two layers claim,
* an edge into the same or a higher layer,
* a cycle,
* an explicitly forbidden edge,
* a third-party dependency nobody signed off on.

Escape hatches exist, because architecture rules that cannot be bent get
deleted instead.  An exception must name an owner, a rationale, and an expiry
date, and it is deleted automatically once the edge it excuses is gone.

The graph is derived by reading CMake as text rather than by configuring a build,
so the audit runs on a bare checkout with no Qt, no Eigen, and no compiler.  The
variable expansion below is only as clever as it needs to be for the files in
this repository: ``set()`` and ``list(APPEND)`` are honoured, conditionals are
ignored, and every branch's value is treated as reachable.  For a dependency
audit that is the conservative direction, because it over-reports edges rather
than missing them.

Usage
-----
    # human-readable layer report
    python3 tools/quality/audit_dependencies.py

    # fail the build on any violation (this is what CI runs)
    python3 tools/quality/audit_dependencies.py --check

    # the graph itself, for diffing or for the quality dashboard
    python3 tools/quality/audit_dependencies.py --format json
    python3 tools/quality/audit_dependencies.py --format dot | dot -Tsvg -o deps.svg

Exit codes
----------
    0   no violations (or ``--check`` not requested)
    1   at least one violation
    2   the tree or the policy file could not be read
"""

from __future__ import annotations

import argparse
import datetime as _datetime
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable

REPO_ROOT = Path(__file__).resolve().parents[2]
LIBRARIES_DIR = REPO_ROOT / "src" / "libraries"
POLICY_FILE = Path(__file__).resolve().parent / "dependency_policy.json"

INTERNAL_PREFIX = "mne_"
LINK_KEYWORDS = {"PRIVATE", "PUBLIC", "INTERFACE", "LINK_PRIVATE", "LINK_PUBLIC", "optimized", "debug", "general"}


# --------------------------------------------------------------------------------------------------------
# CMake reading
# --------------------------------------------------------------------------------------------------------

_SET_RE = re.compile(r"\bset\s*\(\s*([A-Za-z_][A-Za-z0-9_]*)\s+(.*?)\)", re.DOTALL | re.IGNORECASE)
_LIST_APPEND_RE = re.compile(
    r"\blist\s*\(\s*APPEND\s+([A-Za-z_][A-Za-z0-9_]*)\s+(.*?)\)", re.DOTALL | re.IGNORECASE
)
_LINK_RE = re.compile(r"\btarget_link_libraries\s*\(\s*(.*?)\)", re.DOTALL | re.IGNORECASE)
_PROJECT_RE = re.compile(r"^[ \t]*project\s*\(\s*([A-Za-z0-9_.+-]+)", re.MULTILINE | re.IGNORECASE)
_VARIABLE_RE = re.compile(r"\$\{([A-Za-z_][A-Za-z0-9_]*)\}")

_SET_MODIFIERS = {"CACHE", "PARENT_SCOPE", "FORCE", "INTERNAL", "STRING", "BOOL", "PATH", "FILEPATH"}

# Qt targets carry the major version in the name, and the version is itself a
# variable, so ``Qt6::Core`` and ``Qt${QT_VERSION_MAJOR}::Core`` are one
# dependency written two ways. Collapse both to ``Qt::Core`` so the policy can
# name a Qt module once instead of once per spelling.
_QT_TARGET_RE = re.compile(r"^Qt(?:\d+|\$\{[A-Za-z_][A-Za-z0-9_]*\})?::")


def normalise_target(token: str) -> str:
    return _QT_TARGET_RE.sub("Qt::", token)


def strip_cmake_comments(text: str) -> str:
    """Remove ``#`` comments while leaving ``#`` inside quoted strings alone."""
    out: list[str] = []
    for line in text.splitlines():
        in_quotes = False
        cut = len(line)
        index = 0
        while index < len(line):
            char = line[index]
            if char == "\\" and in_quotes:
                index += 2
                continue
            if char == '"':
                in_quotes = not in_quotes
            elif char == "#" and not in_quotes:
                cut = index
                break
            index += 1
        out.append(line[:cut])
    return "\n".join(out)


def tokenize(chunk: str) -> list[str]:
    return [token.strip('"') for token in chunk.split() if token.strip('"')]


def collect_variables(text: str) -> dict[str, list[str]]:
    """Union every assignment of every variable in the file.

    Conditionals are not evaluated, so a variable set differently in two
    branches ends up holding both values.  That is intentional: an audit should
    see an edge that exists on any platform.
    """
    variables: dict[str, list[str]] = {}
    for pattern in (_SET_RE, _LIST_APPEND_RE):
        for name, body in pattern.findall(text):
            values = [t for t in tokenize(body) if t not in _SET_MODIFIERS]
            variables.setdefault(name, [])
            for value in values:
                if value not in variables[name]:
                    variables[name].append(value)
    return variables


def expand(
    tokens: Iterable[str],
    variables: dict[str, list[str]],
    prefixes: dict[str, str] | None = None,
    depth: int = 0,
) -> tuple[list[str], set[str]]:
    """Expand ``${VAR}`` references into resolved tokens plus the names that stayed unresolved.

    ``prefixes`` handles the repository's habit of keeping a list of bare Qt
    module names in one variable and prepending the namespace with
    ``list(TRANSFORM)`` at the point of use. Modelling ``TRANSFORM`` properly is
    not worth it; naming the variables whose contents are Qt modules is.
    """
    prefixes = prefixes or {}
    resolved: list[str] = []
    unresolved: set[str] = set()

    for token in tokens:
        match = _VARIABLE_RE.fullmatch(token)
        if match is None:
            # ``Qt${QT_VERSION_MAJOR}::ZlibPrivate`` is a variable reference only
            # in the version, which does not change which dependency it names.
            candidate = normalise_target(token)
            if "${" in candidate:
                unresolved.update(_VARIABLE_RE.findall(candidate))
            else:
                resolved.append(candidate)
            continue

        name = match.group(1)
        if name not in variables or depth >= 8:
            unresolved.add(name)
            continue

        inner, inner_unresolved = expand(variables[name], variables, prefixes, depth + 1)
        prefix = prefixes.get(name)
        if prefix:
            inner = [value if value.startswith(prefix) else prefix + value for value in inner]
        resolved.extend(inner)
        unresolved.update(inner_unresolved)

    return resolved, unresolved


@dataclass
class Library:
    name: str
    directory: str
    depends_on: list[str]
    external: list[str]
    unresolved: list[str]

    @property
    def cmake_path(self) -> str:
        return f"src/libraries/{self.directory}/CMakeLists.txt"


def read_library(directory: Path, prefixes: dict[str, str] | None = None) -> Library | None:
    cmake = directory / "CMakeLists.txt"
    if not cmake.is_file():
        return None

    text = strip_cmake_comments(cmake.read_text(encoding="utf-8"))
    project_match = _PROJECT_RE.search(text)
    if project_match is None:
        return None

    name = project_match.group(1)
    variables = collect_variables(text)
    variables.setdefault("PROJECT_NAME", [name])

    linked: list[str] = []
    for block in _LINK_RE.findall(text):
        tokens = tokenize(block)
        linked.extend(tokens[1:] if tokens else [])

    linked = [t for t in linked if t not in LINK_KEYWORDS]
    resolved, unresolved = expand(linked, variables, prefixes)

    internal: list[str] = []
    external: list[str] = []
    for token in resolved:
        if token == name:
            continue
        if token.startswith(INTERNAL_PREFIX):
            if token not in internal:
                internal.append(token)
            continue
        normalised = normalise_target(token)
        if normalised not in external:
            external.append(normalised)

    return Library(
        name=name,
        directory=directory.name,
        depends_on=sorted(internal),
        external=sorted(external),
        unresolved=sorted(unresolved),
    )


def build_graph(libraries_dir: Path, prefixes: dict[str, str] | None = None) -> dict[str, Library]:
    graph: dict[str, Library] = {}
    for child in sorted(libraries_dir.iterdir()):
        if not child.is_dir():
            continue
        library = read_library(child, prefixes)
        if library is not None:
            graph[library.name] = library
    return graph


# --------------------------------------------------------------------------------------------------------
# Findings
# --------------------------------------------------------------------------------------------------------


@dataclass
class Finding:
    code: str
    severity: str
    location: str
    message: str

    def render(self) -> str:
        return f"{self.severity.upper():7} {self.code}  {self.location}\n         {self.message}"

    def as_dict(self) -> dict[str, str]:
        return {
            "code": self.code,
            "severity": self.severity,
            "location": self.location,
            "message": self.message,
        }


def error(code: str, location: str, message: str) -> Finding:
    return Finding(code, "error", location, message)


def warning(code: str, location: str, message: str) -> Finding:
    return Finding(code, "warning", location, message)


def policy_location() -> str:
    try:
        return POLICY_FILE.resolve().relative_to(REPO_ROOT).as_posix()
    except ValueError:
        return POLICY_FILE.as_posix()


# --------------------------------------------------------------------------------------------------------
# Layer model
# --------------------------------------------------------------------------------------------------------


def layer_index(policy: dict[str, Any]) -> tuple[dict[str, int], list[str], list[Finding]]:
    """Map each library to its layer number, complaining about double or missing assignment."""
    findings: list[Finding] = []
    index: dict[str, int] = {}
    names: list[str] = []

    for position, layer in enumerate(policy.get("layers", [])):
        name = layer.get("name", f"layer{position}")
        names.append(name)
        for library in layer.get("libraries", []):
            if library in index:
                findings.append(
                    error(
                        "DEP003",
                        policy_location(),
                        f"'{library}' is assigned to both '{names[index[library]]}' and '{name}'.",
                    )
                )
                continue
            index[library] = position

    return index, names, findings


def check_layer_assignment(
    graph: dict[str, Library], index: dict[str, int], policy: dict[str, Any]
) -> list[Finding]:
    findings: list[Finding] = []
    for name, library in sorted(graph.items()):
        if name not in index:
            findings.append(
                error(
                    "DEP001",
                    library.cmake_path,
                    f"'{name}' is not assigned to any layer. Place it in the layering before it "
                    "acquires dependencies nobody agreed to.",
                )
            )
    for library in sorted(index):
        if library not in graph:
            findings.append(
                error(
                    "DEP002",
                    policy_location(),
                    f"the layering names '{library}', which is not a library under src/libraries.",
                )
            )
    return findings


def find_cycles(graph: dict[str, Library]) -> list[list[str]]:
    """Every elementary cycle we can reach, reported once each."""
    cycles: list[list[str]] = []
    seen: set[tuple[str, ...]] = set()
    stack: list[str] = []
    on_stack: set[str] = set()
    visited: set[str] = set()

    def visit(node: str) -> None:
        visited.add(node)
        stack.append(node)
        on_stack.add(node)
        for neighbour in graph[node].depends_on if node in graph else []:
            if neighbour not in graph:
                continue
            if neighbour in on_stack:
                cycle = stack[stack.index(neighbour):] + [neighbour]
                rotation = tuple(cycle[:-1])
                pivot = rotation.index(min(rotation))
                key = rotation[pivot:] + rotation[:pivot]
                if key not in seen:
                    seen.add(key)
                    cycles.append(list(key) + [key[0]])
            elif neighbour not in visited:
                visit(neighbour)
        stack.pop()
        on_stack.discard(node)

    for node in sorted(graph):
        if node not in visited:
            visit(node)

    return cycles


def check_edges(
    graph: dict[str, Library],
    index: dict[str, int],
    layer_names: list[str],
    policy: dict[str, Any],
    exempt: set[tuple[str, str]],
) -> list[Finding]:
    findings: list[Finding] = []

    forbidden = {
        (entry.get("from"), entry.get("to")): entry.get("reason", "")
        for entry in policy.get("forbidden_edges", [])
    }

    for name, library in sorted(graph.items()):
        for dependency in library.depends_on:
            if dependency not in graph:
                findings.append(
                    error(
                        "DEP004",
                        library.cmake_path,
                        f"'{name}' links '{dependency}', which is not a library under src/libraries.",
                    )
                )
                continue

            reason = forbidden.get((name, dependency))
            if reason is not None:
                findings.append(
                    error(
                        "DEP012",
                        library.cmake_path,
                        f"'{name}' -> '{dependency}' is explicitly forbidden: {reason}",
                    )
                )

            if (name, dependency) in exempt:
                continue
            if name not in index or dependency not in index:
                continue

            here, there = index[name], index[dependency]
            if there < here:
                continue

            relation = "its own layer" if there == here else "a higher layer"
            findings.append(
                error(
                    "DEP010",
                    library.cmake_path,
                    f"'{name}' ({layer_names[here]}) depends on '{dependency}' "
                    f"({layer_names[there]}), which is in {relation}. Dependencies must point "
                    "strictly downwards.",
                )
            )

    for cycle in find_cycles(graph):
        head = cycle[0]
        findings.append(
            error(
                "DEP011",
                graph[head].cmake_path,
                "dependency cycle: " + " -> ".join(cycle),
            )
        )

    return findings


def check_external(graph: dict[str, Library], policy: dict[str, Any]) -> list[Finding]:
    """A new third-party dependency is an architectural decision, not a detail."""
    findings: list[Finding] = []
    allowed = set(policy.get("allowed_external", []))
    ignored_variables = set(policy.get("ignored_variables", []))

    for name, library in sorted(graph.items()):
        for dependency in library.external:
            if dependency in allowed:
                continue
            findings.append(
                error(
                    "DEP020",
                    library.cmake_path,
                    f"'{name}' links third-party target '{dependency}', which is not in "
                    "'allowed_external'. Add it to the policy with the rest of the review, or "
                    "drop the dependency.",
                )
            )
        for variable in library.unresolved:
            if variable in ignored_variables:
                continue
            findings.append(
                warning(
                    "DEP021",
                    library.cmake_path,
                    f"'{name}' links through '${{{variable}}}', which this audit cannot resolve, so "
                    "any dependency it carries is invisible here. Record it in "
                    "'ignored_variables' once reviewed.",
                )
            )

    return findings


def check_exceptions(
    graph: dict[str, Library], policy: dict[str, Any], today: _datetime.date
) -> tuple[list[Finding], set[tuple[str, str]]]:
    """An exception is a dated promise, not a permanent grant."""
    findings: list[Finding] = []
    exempt: set[tuple[str, str]] = set()
    location = policy_location()
    required = ("from", "to", "owner", "rationale", "expires")

    entries = policy.get("exceptions", [])
    if not isinstance(entries, list):
        return [error("DEP030", location, "'exceptions' must be a list.")], exempt

    for position, entry in enumerate(entries):
        label = f"exceptions[{position}]"
        if not isinstance(entry, dict):
            findings.append(error("DEP030", location, f"{label} must be an object."))
            continue

        missing = [key for key in required if not entry.get(key)]
        if missing:
            findings.append(
                error(
                    "DEP030",
                    location,
                    f"{label} is missing required field(s): {', '.join(missing)}. Every exception "
                    "needs an owner, a rationale, and an expiry date.",
                )
            )
            continue

        source, target = str(entry["from"]), str(entry["to"])
        label = f"exception '{source}' -> '{target}'"

        if source not in graph:
            findings.append(error("DEP033", location, f"{label} names unknown library '{source}'."))
            continue
        if target not in graph:
            findings.append(error("DEP033", location, f"{label} names unknown library '{target}'."))
            continue

        try:
            expires = _datetime.date.fromisoformat(str(entry["expires"]))
        except ValueError:
            findings.append(
                error("DEP034", location, f"{label} has expiry '{entry['expires']}'; expected YYYY-MM-DD.")
            )
            continue

        if target not in graph[source].depends_on:
            findings.append(
                warning(
                    "DEP032",
                    location,
                    f"{label} is no longer needed; that edge does not exist. Delete the exception.",
                )
            )
            continue

        exempt.add((source, target))

        if expires < today:
            findings.append(
                error(
                    "DEP031",
                    location,
                    f"{label} expired on {expires.isoformat()}. Remove the edge or renegotiate with "
                    f"{entry['owner']}.",
                )
            )
        elif (expires - today).days <= 30:
            findings.append(
                warning(
                    "DEP035",
                    location,
                    f"{label} expires on {expires.isoformat()}, in {(expires - today).days} day(s).",
                )
            )

    return findings, exempt


# --------------------------------------------------------------------------------------------------------
# Reporting
# --------------------------------------------------------------------------------------------------------


def graph_as_dict(graph: dict[str, Library], index: dict[str, int], layer_names: list[str]) -> dict[str, Any]:
    return {
        "layers": layer_names,
        "libraries": {
            name: {
                "directory": library.directory,
                "layer": layer_names[index[name]] if name in index else None,
                "depends_on": library.depends_on,
                "external": library.external,
                "unresolved_variables": library.unresolved,
            }
            for name, library in sorted(graph.items())
        },
    }


def render_dot(graph: dict[str, Library], index: dict[str, int], layer_names: list[str]) -> str:
    lines = ["digraph mne_cpp_libraries {", '  rankdir=BT;', '  node [shape=box, fontname="sans"];']
    for position, layer in enumerate(layer_names):
        members = sorted(name for name in graph if index.get(name) == position)
        if not members:
            continue
        lines.append(f"  subgraph cluster_{position} {{")
        lines.append(f'    label="{layer}";')
        for member in members:
            lines.append(f'    "{member}";')
        lines.append("  }")
    for name, library in sorted(graph.items()):
        for dependency in library.depends_on:
            lines.append(f'  "{name}" -> "{dependency}";')
    lines.append("}")
    return "\n".join(lines)


def render_text(graph: dict[str, Library], index: dict[str, int], layer_names: list[str]) -> str:
    lines = ["Library dependency architecture", ""]
    for position, layer in enumerate(layer_names):
        members = sorted(name for name in graph if index.get(name) == position)
        lines.append(f"  {position}. {layer}")
        for member in members:
            deps = graph[member].depends_on
            suffix = f"  <- {', '.join(deps)}" if deps else ""
            lines.append(f"       {member}{suffix}")
        lines.append("")
    unassigned = sorted(name for name in graph if name not in index)
    if unassigned:
        lines.append("  unassigned: " + ", ".join(unassigned))
        lines.append("")
    edges = sum(len(library.depends_on) for library in graph.values())
    lines.append(f"  {len(graph)} libraries, {edges} internal edges")
    return "\n".join(lines)


# --------------------------------------------------------------------------------------------------------
# Entry point
# --------------------------------------------------------------------------------------------------------


def load_policy(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        print(f"error: policy file not found: {path}", file=sys.stderr)
        raise SystemExit(2)
    except json.JSONDecodeError as exc:
        print(f"error: policy file is not valid JSON: {path}: {exc}", file=sys.stderr)
        raise SystemExit(2)


def run(args: argparse.Namespace) -> int:
    libraries_dir = Path(args.libraries).resolve()
    if not libraries_dir.is_dir():
        print(f"error: no such directory: {libraries_dir}", file=sys.stderr)
        return 2

    policy = load_policy(Path(args.policy).resolve())
    graph = build_graph(libraries_dir, policy.get("variable_prefixes", {}))

    index, layer_names, findings = layer_index(policy)
    findings += check_layer_assignment(graph, index, policy)

    exception_findings, exempt = check_exceptions(graph, policy, _datetime.date.today())
    findings += exception_findings
    findings += check_edges(graph, index, layer_names, policy, exempt)
    findings += check_external(graph, policy)

    errors = [f for f in findings if f.severity == "error"]

    if args.format == "json":
        payload = graph_as_dict(graph, index, layer_names)
        payload["findings"] = [f.as_dict() for f in findings]
        print(json.dumps(payload, indent=2, sort_keys=True))
        return 1 if (errors and args.check) else 0

    if args.format == "dot":
        print(render_dot(graph, index, layer_names))
        return 1 if (errors and args.check) else 0

    print(render_text(graph, index, layer_names))
    print()

    if findings:
        for finding in sorted(findings, key=lambda f: (f.severity != "error", f.code, f.location)):
            print(finding.render())
            print()
        warnings = len(findings) - len(errors)
        print(f"{len(errors)} violation(s), {warnings} warning(s)")
    else:
        print("OK: the graph matches the declared architecture.")

    return 1 if (errors and args.check) else 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Audit the MNE-CPP library dependency architecture.")
    parser.add_argument("--libraries", default=str(LIBRARIES_DIR), help="path to src/libraries")
    parser.add_argument("--policy", default=str(POLICY_FILE), help="path to the architecture policy")
    parser.add_argument(
        "--format", choices=("text", "json", "dot"), default="text", help="output format (default: %(default)s)"
    )
    parser.add_argument(
        "--check", action="store_true", help="exit non-zero when the graph violates the policy"
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    return run(build_parser().parse_args(argv))


if __name__ == "__main__":
    raise SystemExit(main())
