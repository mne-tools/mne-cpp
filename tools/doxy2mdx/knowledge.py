#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
"""Resolve the ``topics`` and ``examples`` sections of ``doc/api_registry.json``.

A topic ties a scientific method to the manual sections that explain it, the
command-line tools that run it, the classes and methods that implement it and
the examples that demonstrate it. Every reference is resolved against the
files it names: manual anchors against the headings of the page, classes
against the generated API pages, ``Class::method`` against the method headings
of that page, examples against ``src/examples``. An unresolvable reference is
an error, so the links rendered on the website cannot drift from the code.
"""

from __future__ import annotations

import json
import re
from dataclasses import dataclass, field
from pathlib import Path

DOCS = Path("doc/website/docs")
REGISTRY = Path("doc/api_registry.json")
HEADING = re.compile(r"^(#{1,6})\s+(.+?)\s*$")
EXPLICIT_ID = re.compile(r"\s*\{#([\w-]+)\}\s*$")
FENCE = re.compile(r"^\s*(```|~~~)")


def slugify(text: str) -> str:
    """Heading id as Docusaurus (github-slugger) derives it from the heading text."""
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", text)
    text = text.replace("`", "").replace("\\", "")
    return re.sub(r"[^\w\- ]", "", text.strip().lower()).replace(" ", "-")


def headings(path: Path) -> list[tuple[str, str]]:
    """(text, id) of every Markdown heading of *path* outside code fences, duplicates numbered."""
    seen: dict[str, int] = {}
    result = []
    fenced = False
    for line in path.read_text(encoding="utf-8").splitlines():
        if FENCE.match(line):
            fenced = not fenced
            continue
        match = None if fenced else HEADING.match(line)
        if not match:
            continue
        text = match.group(2)
        explicit = EXPLICIT_ID.search(text)
        if explicit:
            text = text[:explicit.start()]
            anchor = explicit.group(1)
        else:
            base = slugify(text)
            anchor = base if base not in seen else f"{base}-{seen[base]}"
            seen[base] = seen.get(base, 0) + 1
        result.append((re.sub(r"[`\\]", "", text).strip(), anchor))
    return result


def front_matter(path: Path) -> dict[str, str]:
    text = path.read_text(encoding="utf-8")
    if not text.startswith("---\n"):
        return {}
    block = text[4:text.index("\n---", 4)]
    return {k.strip(): v.strip().strip('"') for k, v in
            (line.split(":", 1) for line in block.splitlines() if ":" in line and not line.startswith(" "))}


def doc_url(rel: str, meta: dict[str, str] | None = None) -> str:
    """Site URL (without baseUrl) of the doc ``rel`` under ``docs/``; trailingSlash is on."""
    if meta and meta.get("slug", "").startswith("/"):
        return "/docs" + meta["slug"].rstrip("/") + "/"
    stem = rel.removesuffix(".mdx").removesuffix(".md")
    stem = stem.removesuffix("/index") if stem.endswith("/index") else ("" if stem == "index" else stem)
    return f"/docs/{stem}/" if stem else "/docs/"


def page_title(path: Path) -> str:
    meta = front_matter(path)
    if meta.get("title"):
        return meta["title"]
    first = next((text for text, _ in headings(path)), path.stem)
    return first


@dataclass
class Knowledge:
    """Resolved topics and example metadata, ready to be written as JSON for the website."""

    facets: dict[str, dict[str, str]] = field(default_factory=dict)
    examples: dict[str, dict] = field(default_factory=dict)
    topics: list[dict] = field(default_factory=list)
    api_pages: dict[str, tuple[str, Path]] = field(default_factory=dict)


def api_pages(root: Path) -> dict[str, tuple[str, Path]]:
    """Short class name -> (URL, page) of every generated API class page."""
    pages = {}
    api = root / DOCS / "api"
    for page in sorted(api.rglob("*.mdx")) if api.is_dir() else []:
        title = front_matter(page).get("title", "")
        if "::" in title:
            short = title.rsplit("::", 1)[1]
            pages.setdefault(short, (doc_url(page.relative_to(root / DOCS).as_posix(), front_matter(page)), page))
    return pages


def load(root: Path, example_names: list[str]) -> Knowledge:
    """Validate and resolve the registry sections; raise ValueError listing every unresolved reference."""
    path = root / REGISTRY
    registry = json.loads(path.read_text(encoding="utf-8")) if path.is_file() else {}
    knowledge = Knowledge(api_pages=api_pages(root))
    errors: list[str] = []
    examples = registry.get("examples")
    if examples:
        knowledge.facets = examples["facets"]
        entries = examples["entries"]
        for name in sorted(set(example_names) - entries.keys()):
            errors.append(f"examples: src/examples/{name} has no entry")
        for name in sorted(entries.keys() - set(example_names)):
            errors.append(f"examples: '{name}' is not a directory in src/examples")
        for name, entry in entries.items():
            for facet, key in (("tasks", "tasks"), ("modalities", "modalities")):
                if not entry.get(key):
                    errors.append(f"examples.{name}: no {key}")
                for value in entry.get(key, []):
                    if value not in knowledge.facets[facet]:
                        errors.append(f"examples.{name}: unknown {facet[:-1] if facet != 'modalities' else 'modality'} "
                                      f"'{value}'")
            if entry.get("level") not in knowledge.facets["levels"]:
                errors.append(f"examples.{name}: unknown level '{entry.get('level')}'")
        knowledge.examples = entries
    registered = {c["name"] for c in registry.get("classes", [])}
    ids = set()
    for topic in registry.get("topics", {}).get("entries", []):
        where = f"topics.{topic.get('id')}"
        if topic["id"] in ids:
            errors.append(f"{where}: duplicate id")
        ids.add(topic["id"])
        resolved = {"id": topic["id"], "title": topic["title"], "summary": topic["summary"],
                    "manual": [], "tools": [], "api": [], "examples": []}
        for kind in ("manual", "tools"):
            for ref in topic.get(kind, []):
                link = _doc_link(root, ref)
                if isinstance(link, str):
                    errors.append(f"{where}: {link}")
                else:
                    resolved[kind].append(link)
        methods: dict[str, list[str]] = {}
        for ref in topic.get("methods", []):
            cls, _, method = ref.partition("::")
            methods.setdefault(cls, []).append(method)
            if cls not in topic["classes"]:
                errors.append(f"{where}: method {ref} of a class the topic does not list")
        for cls in topic["classes"]:
            if cls not in registered:
                errors.append(f"{where}: class {cls} is not in the registry")
                continue
            if cls not in knowledge.api_pages:
                errors.append(f"{where}: class {cls} has no API page")
                continue
            url, page = knowledge.api_pages[cls]
            entry = {"name": cls, "url": url, "methods": []}
            anchors = headings(page)
            for method in methods.get(cls, []):
                hit = next((anchor for text, anchor in anchors if text.startswith(method + "(")), None)
                if hit is None:
                    errors.append(f"{where}: {cls}::{method} has no heading on {page.name}")
                else:
                    entry["methods"].append({"name": method, "url": f"{url}#{hit}"})
            resolved["api"].append(entry)
        if topic.get("start") not in topic["examples"]:
            errors.append(f"{where}: start example '{topic.get('start')}' is not one of its examples")
        for name in topic["examples"]:
            if name not in example_names:
                errors.append(f"{where}: example {name} does not exist")
            resolved["examples"].append({"name": name, "start": name == topic.get("start")})
        knowledge.topics.append(resolved)
    if errors:
        raise ValueError("\n".join(errors))
    return knowledge


def _doc_link(root: Path, ref: str) -> dict | str:
    rel, _, anchor = ref.partition("#")
    page = root / DOCS / rel
    if not page.is_file():
        return f"{ref}: no such page under {DOCS}"
    title = page_title(page)
    url = doc_url(rel, front_matter(page))
    if not anchor:
        return {"label": title, "url": url}
    match = next((text for text, hid in headings(page) if hid == anchor), None)
    if match is None:
        return f"{ref}: no heading with id '{anchor}'"
    return {"label": f"{title} › {match}", "url": f"{url}#{anchor}"}


def topics_of_example(knowledge: Knowledge, name: str) -> list[str]:
    return [t["id"] for t in knowledge.topics if any(e["name"] == name for e in t["examples"])]


def topics_of_class(knowledge: Knowledge, name: str) -> list[str]:
    return [t["id"] for t in knowledge.topics if any(a["name"] == name for a in t["api"])]
