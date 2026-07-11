#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2010-2026 MNE-CPP Authors
#
# @author  Christoph Dinh <christoph.dinh@mne-cpp.org>

"""MNE-CPP vs MNE-Python gap analysis renderer + drift check (v2.4.0 TASK T7.0).

MNE-CPP and MNE-Python have fundamentally different architectures (C++
real-time, class/CLI-tool oriented vs. scriptable, function-heavy), so the
*comparison itself is a qualitative, human judgement*. That judgement lives as
DATA in ``doc/api_registry.json`` — the existing ``classes`` array plus the
hand-curated ``parity`` array — not in this script.

This tool does only what a machine can do reliably:

1. **Inventory** the public MNE-Python API from the *installed* pinned ``mne``
   package so no symbol is silently forgotten (deterministic, version-locked).
2. **Reconcile** that inventory with the registry: a ``classes`` entry whose
   ``mne_python`` / ``python_equiv`` names a symbol counts it *implemented*; the
   ``parity`` array supplies the remaining qualitative verdicts (CLI-tool /
   method equivalents, ``partial``, ``missing``, ``not-applicable``).
3. **Detect drift** and fail loudly when the human data and the pinned Python
   reference disagree — Python APIs classified nowhere, or parity records that
   name a symbol the reference no longer exports.
4. **Render** ``doc/release/v2.4.0/mne-python-gap.{json,md}`` and refresh the
   header of ``doc/dev-notes/gap-analysis.md`` from that same data.

So the maintainer edits the registry (data); the script keeps the reports and
the narrative honest and current. It never encodes the mapping itself.

Usage::

    python3 tools/parity/gap_analysis.py                 # regenerate reports
    python3 tools/parity/gap_analysis.py --check         # CI: stale/drift => fail
    python3 tools/parity/gap_analysis.py --refresh-gap-doc
"""

from __future__ import annotations

import argparse
import datetime as _dt
import inspect
import json
import sys
import types
from collections import OrderedDict
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

# ---------------------------------------------------------------------------
# Paths.
# ---------------------------------------------------------------------------
REPO_ROOT = Path(__file__).resolve().parents[2]
REGISTRY_PATH = REPO_ROOT / "doc" / "api_registry.json"
OUT_DIR = REPO_ROOT / "doc" / "release" / "v2.4.0"
OUT_JSON = OUT_DIR / "mne-python-gap.json"
OUT_MD = OUT_DIR / "mne-python-gap.md"
GAP_DOC = REPO_ROOT / "doc" / "dev-notes" / "gap-analysis.md"

# Status vocabulary for a Python symbol as seen from MNE-CPP.
STATUS_IMPLEMENTED = "implemented"
STATUS_PARTIAL = "partial"
STATUS_MISSING = "missing"
STATUS_NOT_APPLICABLE = "not-applicable"
STATUS_ORDER = [
    STATUS_IMPLEMENTED,
    STATUS_PARTIAL,
    STATUS_MISSING,
    STATUS_NOT_APPLICABLE,
]

# ---------------------------------------------------------------------------
# Domain taxonomy: MNE-Python submodule -> MNE-CPP library-oriented domain.
# Order defines report section order.
# ---------------------------------------------------------------------------
DOMAINS: "OrderedDict[str, str]" = OrderedDict(
    [
        ("io", "I/O & Readers"),
        ("core", "Core Data Containers"),
        ("preprocessing", "Preprocessing & Artifacts"),
        ("filter", "Filtering"),
        ("channels", "Channels & Montages"),
        ("epochs", "Epochs & Evoked"),
        ("cov", "Covariance & Whitening"),
        ("forward", "Forward Modelling"),
        ("inverse", "Inverse & Source Estimation"),
        ("source_space", "Source Space & Morphing"),
        ("time_frequency", "Time-Frequency"),
        ("connectivity", "Connectivity"),
        ("decoding", "Decoding & Machine Learning"),
        ("stats", "Statistics"),
        ("simulation", "Simulation"),
        ("viz", "Visualisation"),
        ("misc", "Miscellaneous"),
    ]
)

SUBMODULE_DOMAIN: Dict[str, str] = {
    "io": "io",
    "export": "io",
    "preprocessing": "preprocessing",
    "chpi": "preprocessing",
    "filter": "filter",
    "channels": "channels",
    "epochs": "epochs",
    "event": "epochs",
    "forward": "forward",
    "bem": "forward",
    "minimum_norm": "inverse",
    "inverse_sparse": "inverse",
    "beamformer": "inverse",
    "dipole": "inverse",
    "source_space": "source_space",
    "source_estimate": "inverse",
    "morph": "source_space",
    "surface": "source_space",
    "time_frequency": "time_frequency",
    "connectivity": "connectivity",
    "decoding": "decoding",
    "stats": "stats",
    "simulation": "simulation",
    "viz": "viz",
    "report": "viz",
    "coreg": "channels",
    "transforms": "core",
    "cov": "cov",
}

DOMAIN_OVERRIDE: Dict[str, str] = {
    "mne.Info": "core",
    "mne.Annotations": "core",
    "mne.Report": "viz",
    "mne.create_info": "core",
}

# Public MNE-Python namespaces to inventory (documented public API only).
INVENTORY_SUBMODULES = [
    "",  # top-level mne namespace
    "io",
    "export",
    "preprocessing",
    "channels",
    "forward",
    "minimum_norm",
    "inverse_sparse",
    "beamformer",
    "source_space",
    "time_frequency",
    "decoding",
    "stats",
    "simulation",
]

# Submodules never inventoried: framework/plumbing not comparable to a C++
# scientific library.
SKIP_SUBMODULES = {
    "datasets",
    "gui",
    "commands",
    "cuda",
    "defaults",
    "utils",
    "fixes",
    "html_templates",
}

# Pure Python-ecosystem plumbing (config, logging, docs) — not parity targets.
SKIP_SYMBOLS = {
    "get_config",
    "set_config",
    "get_config_path",
    "set_log_level",
    "set_log_file",
    "open_docs",
    "sys_info",
    "use_log_level",
    "verbose",
    "set_cache_dir",
    "set_memmap_min_size",
    "get_montage_volume_labels",
    "open_report",
}


# ---------------------------------------------------------------------------
# Registry (the human-curated data source).
# ---------------------------------------------------------------------------
def load_registry() -> Dict[str, Any]:
    with open(REGISTRY_PATH, "r", encoding="utf-8") as f:
        return json.load(f)


def pinned_version(registry: Dict[str, Any]) -> str:
    return str(registry.get("parity", {}).get("mne_python_pinned", "")).strip()


def registry_class_python_map(registry: Dict[str, Any]) -> Dict[str, str]:
    """Every Python symbol named by an existing C++ class -> that class name."""
    mapping: Dict[str, str] = {}
    for cls in registry.get("classes", []):
        for key in ("mne_python", "python_equiv"):
            py = cls.get(key)
            if py:
                mapping.setdefault(py.strip(), cls["name"])
    return mapping


def registry_parity_records(registry: Dict[str, Any]) -> Dict[str, Dict[str, str]]:
    """The hand-curated parity verdicts, keyed by fully-qualified Python name."""
    out: Dict[str, Dict[str, str]] = {}
    for rec in registry.get("parity", {}).get("records", []):
        py = rec.get("python", "").strip()
        if py:
            out[py] = rec
    return out


# ---------------------------------------------------------------------------
# MNE-Python inventory (the machine part).
# ---------------------------------------------------------------------------
def _pinned_ok(version: str, pinned: str) -> bool:
    return ".".join(version.split(".")[:2]) == pinned


def _public_names(module: types.ModuleType) -> List[str]:
    explicit = getattr(module, "__all__", None)
    if explicit:
        return [n for n in explicit if not n.startswith("_")]
    return [n for n in dir(module) if not n.startswith("_")]


def _domain_for(qualname: str, submodule: str, obj: Any) -> str:
    if qualname in DOMAIN_OVERRIDE:
        return DOMAIN_OVERRIDE[qualname]
    if submodule and submodule in SUBMODULE_DOMAIN:
        return SUBMODULE_DOMAIN[submodule]
    mod = getattr(obj, "__module__", "") or ""
    for part in mod.split("."):
        if part in SUBMODULE_DOMAIN:
            return SUBMODULE_DOMAIN[part]
    return "core"


def build_python_inventory(pinned: str) -> List[Dict[str, Any]]:
    """Enumerate the pinned public MNE-Python API as inventory records."""
    import mne  # lazy so --help works without mne installed

    if not _pinned_ok(mne.__version__, pinned):
        sys.stderr.write(
            f"ERROR: registry pins MNE-Python {pinned}.x but the installed "
            f"version is {mne.__version__}. Install the pinned reference, or "
            f"bump 'parity.mne_python_pinned' in the registry deliberately and "
            f"regenerate.\n"
        )
        raise SystemExit(2)

    seen: "OrderedDict[str, Dict[str, Any]]" = OrderedDict()
    for sub in INVENTORY_SUBMODULES:
        if sub in SKIP_SUBMODULES:
            continue
        module = mne if sub == "" else getattr(mne, sub, None)
        if module is None:
            continue
        prefix = "mne" if sub == "" else f"mne.{sub}"
        for name in _public_names(module):
            if sub == "" and name in SKIP_SUBMODULES:
                continue
            if name in SKIP_SYMBOLS:
                continue
            try:
                obj = getattr(module, name)
            except Exception:
                continue
            if isinstance(obj, types.ModuleType):
                continue
            if not (inspect.isclass(obj) or inspect.isfunction(obj) or inspect.isbuiltin(obj)):
                continue
            qualname = f"{prefix}.{name}"
            if qualname in seen:
                continue
            seen[qualname] = {
                "python": qualname,
                "name": name,
                "submodule": sub or "(top-level)",
                "kind": "class" if inspect.isclass(obj) else "function",
                "domain": _domain_for(qualname, sub, obj),
            }
    return list(seen.values())


# ---------------------------------------------------------------------------
# Reconciliation + drift detection.
# ---------------------------------------------------------------------------
def classify(
    inventory: List[Dict[str, Any]],
    class_map: Dict[str, str],
    parity: Dict[str, Dict[str, str]],
) -> Tuple[List[Dict[str, Any]], List[str]]:
    """Assign each Python API a status from the registry data.

    Returns ``(records, unclassified)`` where ``unclassified`` lists inventory
    symbols the human data does not cover (drift).
    """
    records: List[Dict[str, Any]] = []
    unclassified: List[str] = []
    for item in inventory:
        py = item["python"]
        status: Optional[str] = None
        mne_cpp = ""
        note = ""
        evidence = "unverified"

        # 1. Existing C++ class proves an implemented, registered API.
        if py in class_map:
            status = STATUS_IMPLEMENTED
            mne_cpp = class_map[py]
            evidence = "registry-class"
        else:
            short = py.split(".")[-1]
            for reg_py, reg_cls in class_map.items():
                if reg_py.split(".")[-1] == short and reg_py.endswith(short):
                    status = STATUS_IMPLEMENTED
                    mne_cpp = reg_cls
                    evidence = "registry-class"
                    break

        # 2. Curated parity record (authoritative override / gap verdict).
        if py in parity:
            rec = parity[py]
            status = rec.get("status", STATUS_MISSING)
            mne_cpp = rec.get("mne_cpp") or mne_cpp
            note = rec.get("note", "")
            evidence = "parity-record"

        if status is None:
            unclassified.append(py)
            status = STATUS_MISSING
            evidence = "unclassified"

        rec_out = dict(item)
        rec_out["status"] = status
        rec_out["mne_cpp"] = mne_cpp
        rec_out["note"] = note
        rec_out["evidence"] = evidence
        records.append(rec_out)
    return records, unclassified


def stale_parity_records(
    inventory: List[Dict[str, Any]], parity: Dict[str, Dict[str, str]]
) -> List[str]:
    """Parity records naming a Python symbol the pinned reference no longer has."""
    inv_names = {i["python"] for i in inventory}
    return sorted(py for py in parity if py not in inv_names)


# ---------------------------------------------------------------------------
# Report assembly.
# ---------------------------------------------------------------------------
def summarise(records: List[Dict[str, Any]]) -> Dict[str, Any]:
    by_status: Dict[str, int] = {s: 0 for s in STATUS_ORDER}
    by_domain: "OrderedDict[str, Dict[str, int]]" = OrderedDict()
    for dom in DOMAINS:
        by_domain[dom] = {s: 0 for s in STATUS_ORDER}
    for r in records:
        by_status[r["status"]] += 1
        by_domain.setdefault(r["domain"], {s: 0 for s in STATUS_ORDER})
        by_domain[r["domain"]][r["status"]] += 1

    considered = (
        by_status[STATUS_IMPLEMENTED]
        + by_status[STATUS_PARTIAL]
        + by_status[STATUS_MISSING]
    )
    parity_pct = (
        100.0
        * (by_status[STATUS_IMPLEMENTED] + 0.5 * by_status[STATUS_PARTIAL])
        / considered
        if considered
        else 0.0
    )
    return {
        "total": len(records),
        "by_status": by_status,
        "by_domain": by_domain,
        "denominator_excludes_not_applicable": considered,
        "parity_percent": round(parity_pct, 1),
    }


def build_report(
    records: List[Dict[str, Any]],
    summary: Dict[str, Any],
    mne_version: str,
    pinned: str,
) -> Dict[str, Any]:
    records_sorted = sorted(
        records,
        key=lambda r: (
            list(DOMAINS).index(r["domain"]) if r["domain"] in DOMAINS else 999,
            STATUS_ORDER.index(r["status"]),
            r["python"],
        ),
    )
    return OrderedDict(
        [
            (
                "meta",
                OrderedDict(
                    [
                        ("generated_by", "tools/parity/gap_analysis.py"),
                        ("generated_on", _dt.date.today().isoformat()),
                        ("mne_python_ref", mne_version),
                        ("mne_python_pinned", pinned),
                        ("registry", "doc/api_registry.json"),
                        (
                            "note",
                            "Machine-rendered from doc/api_registry.json "
                            "('classes' + 'parity'). Do not hand-edit; edit the "
                            "registry and rerun tools/parity/gap_analysis.py.",
                        ),
                    ]
                ),
            ),
            ("summary", summary),
            ("apis", records_sorted),
        ]
    )


def render_markdown(report: Dict[str, Any]) -> str:
    meta = report["meta"]
    summ = report["summary"]
    lines: List[str] = []
    a = lines.append
    a("# MNE-CPP ↔ MNE-Python Gap Analysis (machine-rendered)")
    a("")
    a("> **Do not hand-edit.** This is rendered from the qualitative parity "
      "data in `doc/api_registry.json` (`classes` + `parity`). To change a "
      "verdict, edit the registry and rerun "
      "`python3 tools/parity/gap_analysis.py`.")
    a("")
    a(f"- Generated: **{meta['generated_on']}**")
    a(f"- MNE-Python reference: **{meta['mne_python_ref']}** "
      f"(pinned {meta['mne_python_pinned']}.x)")
    a(f"- Source of truth: `{meta['registry']}`")
    a("")
    a("## Summary")
    a("")
    bs = summ["by_status"]
    a(f"- **Total public MNE-Python APIs inventoried:** {summ['total']}")
    a(f"- Implemented: **{bs['implemented']}**")
    a(f"- Partial: **{bs['partial']}**")
    a(f"- Missing: **{bs['missing']}**")
    a(f"- Not-applicable: **{bs['not-applicable']}**")
    a(f"- **Parity** (implemented + ½·partial, excluding not-applicable): "
      f"**{summ['parity_percent']}%** of "
      f"{summ['denominator_excludes_not_applicable']} in-scope APIs")
    a("")
    a("Every parity figure exposes its denominator (in-scope = implemented + "
      "partial + missing; not-applicable excluded) and the pinned reference "
      "version.")
    a("")
    a("## Per-domain status")
    a("")
    a("| Domain | Implemented | Partial | Missing | N/A | Total |")
    a("|---|---:|---:|---:|---:|---:|")
    for dom, label in DOMAINS.items():
        counts = summ["by_domain"].get(dom)
        if not counts:
            continue
        tot = sum(counts.values())
        if tot == 0:
            continue
        a(f"| {label} | {counts['implemented']} | {counts['partial']} | "
          f"{counts['missing']} | {counts['not-applicable']} | {tot} |")
    a("")
    a("## Gaps (missing / partial), grouped by domain")
    a("")
    gaps = [r for r in report["apis"] if r["status"] in (STATUS_MISSING, STATUS_PARTIAL)]
    current_domain = None
    for r in gaps:
        if r["domain"] != current_domain:
            current_domain = r["domain"]
            a("")
            a(f"### {DOMAINS.get(current_domain, current_domain)}")
            a("")
            a("| Python API | Status | MNE-CPP | Notes |")
            a("|---|---|---|---|")
        note = r["note"] or ""
        cpp = r["mne_cpp"] or "—"
        a(f"| `{r['python']}` | {r['status']} | {cpp} | {note} |")
    a("")
    a("## Already implemented (do not re-implement)")
    a("")
    impl = [r for r in report["apis"] if r["status"] == STATUS_IMPLEMENTED]
    a(f"{len(impl)} MNE-Python APIs already have an MNE-CPP equivalent. See "
      "`mne-python-gap.json` (`status == \"implemented\"`) for the full "
      "mapping. TASK 8 candidates must not target any API listed there "
      "(AC-T8.0-3).")
    a("")
    return "\n".join(lines) + "\n"


def refresh_gap_doc(report: Dict[str, Any]) -> None:
    """Rewrite the header of doc/dev-notes/gap-analysis.md from generated data."""
    meta = report["meta"]
    summ = report["summary"]
    bs = summ["by_status"]
    header = [
        "# MNE-CPP Gap Analysis — Comprehensive Feature Comparison",
        "",
        "Internal developer reference. Compares MNE-CPP against MNE-Python (and",
        "MNE-C SVN) to identify features and algorithms not yet ported.",
        "",
        "> **Parity numbers below are machine-rendered from "
        "`doc/api_registry.json`.**",
        "> The authoritative, per-API machine-readable inventory lives in",
        "> [`doc/release/v2.4.0/mne-python-gap.json`](../release/v2.4.0/mne-python-gap.json)",
        "> and its rendered view",
        "> [`mne-python-gap.md`](../release/v2.4.0/mne-python-gap.md), both produced",
        "> by `tools/parity/gap_analysis.py`. Regenerate them before quoting any",
        "> parity figure. The domain narratives further below are hand-maintained",
        "> context and must not contradict the generated data.",
        "",
        f"- Generated reference: **MNE-Python {meta['mne_python_ref']}** "
        f"(pinned {meta['mne_python_pinned']}.x), on **{meta['generated_on']}**.",
        f"- Inventoried public APIs: **{summ['total']}** — "
        f"implemented **{bs['implemented']}**, partial **{bs['partial']}**, "
        f"missing **{bs['missing']}**, not-applicable **{bs['not-applicable']}**.",
        f"- Machine-rendered parity: **{summ['parity_percent']}%** of "
        f"{summ['denominator_excludes_not_applicable']} in-scope APIs "
        "(implemented + ½·partial; not-applicable excluded).",
        "",
        "---",
        "",
        "## Legend",
        "",
        "- **Source**: which reference codebase has the feature "
        "(C = MNE-C SVN, Py = MNE-Python, Both)",
        "- **Priority**: High / Medium / Low — based on user demand and "
        "scientific utility",
        "- ✅ = MNE-CPP already has this    ❌ = missing from MNE-CPP",
        "",
        "---",
        "",
    ]
    text = GAP_DOC.read_text(encoding="utf-8")
    marker = "## 1. Inverse & Source Estimation"
    idx = text.find(marker)
    if idx == -1:
        sys.stderr.write(
            "WARNING: could not find domain narrative marker in "
            "gap-analysis.md; leaving body untouched.\n"
        )
        return
    GAP_DOC.write_text("\n".join(header) + text[idx:], encoding="utf-8")


# ---------------------------------------------------------------------------
# Main.
# ---------------------------------------------------------------------------
def generate() -> Tuple[Dict[str, Any], str, List[str], List[str]]:
    registry = load_registry()
    pinned = pinned_version(registry)
    if not pinned:
        sys.stderr.write(
            "ERROR: registry has no 'parity.mne_python_pinned' version.\n"
        )
        raise SystemExit(2)

    class_map = registry_class_python_map(registry)
    parity = registry_parity_records(registry)
    inventory = build_python_inventory(pinned)

    import mne

    records, unclassified = classify(inventory, class_map, parity)
    stale = stale_parity_records(inventory, parity)
    summary = summarise(records)
    report = build_report(records, summary, mne.__version__, pinned)
    md = render_markdown(report)
    return report, md, unclassified, stale


def _dump_json(report: Dict[str, Any]) -> str:
    return json.dumps(report, indent=2, ensure_ascii=False) + "\n"


def _report_drift(unclassified: List[str], stale: List[str]) -> None:
    if unclassified:
        sys.stderr.write(
            f"\nDRIFT: {len(unclassified)} MNE-Python API(s) are not classified "
            "in doc/api_registry.json ('classes' or 'parity'). Add a parity "
            "record (or a class mapping) for each:\n  - "
            + "\n  - ".join(unclassified) + "\n"
        )
    if stale:
        sys.stderr.write(
            f"\nDRIFT: {len(stale)} parity record(s) name a symbol the pinned "
            "MNE-Python reference no longer exports. Remove or update:\n  - "
            + "\n  - ".join(stale) + "\n"
        )


def main(argv: Optional[List[str]] = None) -> int:
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    p.add_argument("--check", action="store_true",
                   help="CI mode: fail if reports are stale or data drifted")
    p.add_argument("--refresh-gap-doc", action="store_true",
                   help="also regenerate doc/dev-notes/gap-analysis.md header")
    p.add_argument("--allow-drift", action="store_true",
                   help="warn (do not fail) on unclassified/stale APIs")
    args = p.parse_args(argv)

    report, md, unclassified, stale = generate()
    json_text = _dump_json(report)
    drifted = bool(unclassified or stale)

    if args.check:
        problems: List[str] = []
        if not OUT_JSON.exists() or OUT_JSON.read_text(encoding="utf-8") != json_text:
            problems.append(str(OUT_JSON.relative_to(REPO_ROOT)) + " (stale)")
        if not OUT_MD.exists() or OUT_MD.read_text(encoding="utf-8") != md:
            problems.append(str(OUT_MD.relative_to(REPO_ROOT)) + " (stale)")
        _report_drift(unclassified, stale)
        if problems:
            sys.stderr.write(
                "\nERROR: gap analysis reports are stale. Regenerate with "
                "`python3 tools/parity/gap_analysis.py`:\n  - "
                + "\n  - ".join(problems) + "\n"
            )
        if problems or (drifted and not args.allow_drift):
            return 1
        print("Gap analysis reports are up to date; no drift.")
        return 0

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    OUT_JSON.write_text(json_text, encoding="utf-8")
    OUT_MD.write_text(md, encoding="utf-8")
    if args.refresh_gap_doc:
        refresh_gap_doc(report)

    summ = report["summary"]
    bs = summ["by_status"]
    print(f"Wrote {OUT_JSON.relative_to(REPO_ROOT)} and "
          f"{OUT_MD.relative_to(REPO_ROOT)}")
    print(f"  {summ['total']} APIs: {bs['implemented']} implemented, "
          f"{bs['partial']} partial, {bs['missing']} missing, "
          f"{bs['not-applicable']} n/a; parity {summ['parity_percent']}%")
    if drifted:
        _report_drift(unclassified, stale)
        if not args.allow_drift:
            sys.stderr.write(
                "\nData drift detected (see above). Reports were still written; "
                "fix the registry to clear drift.\n"
            )
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
