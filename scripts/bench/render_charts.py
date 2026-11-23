#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>
#
# @since    3.0.0
# @date     November, 2026
#
#
# @brief    SVG charts of the benchmark results for the website's Performance page.
"""SVG charts of benchmark results, without plotting libraries.

Reads a results file written by ``run_benchmarks.py`` and writes one self-contained SVG per chart, each with
its title, axis, values and a footer naming the hardware, operating system, compiler, commit and the command
that reproduces it. The SVGs are committed, so the documentation builds offline.

    python3 scripts/bench/render_charts.py doc/website/static/bench/results.json doc/website/static/bench
"""

from __future__ import annotations

import argparse
import json
import math
from html import escape
from pathlib import Path
from typing import Any

WIDTH = 760
BAR_HEIGHT = 26
FONT = 'font-family="-apple-system, Segoe UI, Roboto, Helvetica, Arial, sans-serif"'
PALETTE = ["#0b6ea8", "#e8833a", "#5aa05a", "#9467bd", "#8c564b"]


def nice_ceiling(value: float) -> float:
    """The smallest 1, 2 or 5 times a power of ten that is at least value."""
    if value <= 0:
        return 1.0
    power = 10 ** math.floor(math.log10(value))
    for step in (1, 2, 5, 10):
        if value <= step * power:
            return step * power
    return 10 * power


def format_value(value: float) -> str:
    if value == 0:
        return "0"
    if abs(value) >= 100:
        return f"{value:,.0f}"
    if abs(value) >= 10:
        return f"{value:.1f}"
    if abs(value) >= 0.1:
        return f"{value:.2f}"
    return f"{value:.3g}"


def footer(environment: dict[str, str], command: str, y: float) -> tuple[list[str], float]:
    lines = [f"{environment['machine']} · {environment['os']} · {environment['compiler']} · "
             f"commit {environment['commit']} · {environment['date']}"]
    if environment.get("reference"):
        lines.append(f"Reference: {environment['reference']}")
    lines.append(f"Reproduce: {command}")
    out = [f'<text x="12" y="{y + 16 * (i + 1):.0f}" font-size="11" fill="#555" {FONT}>{escape(t)}</text>'
           for i, t in enumerate(lines)]
    return out, y + 16 * len(lines) + 10


def svg(chart: dict[str, Any], height: float, body: list[str]) -> str:
    head = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{WIDTH}" height="{height:.0f}" '
            f'viewBox="0 0 {WIDTH} {height:.0f}" role="img" aria-label="{escape(chart["title"])}">',
            '<rect width="100%" height="100%" fill="#fff"/>',
            f'<text x="12" y="24" font-size="16" font-weight="600" {FONT}>{escape(chart["title"])}</text>']
    if chart.get("subtitle"):
        head.append(f'<text x="12" y="40" font-size="11" fill="#555" {FONT}>{escape(chart["subtitle"])}</text>')
    return "\n".join(head + body + ["</svg>"]) + "\n"


def bar_chart(chart: dict[str, Any], environment: dict[str, str]) -> str:
    """Horizontal grouped bars: one group per category, one bar per series."""
    series = chart["series"]
    categories = list(dict.fromkeys(p["x"] for s in series for p in s["points"]))
    values = {(s["label"], p["x"]): p["y"] for s in series for p in s["points"]}
    plot_left = max(150, 14 + 7 * max(len(str(c)) for c in categories))
    plot_width = WIDTH - plot_left - 90
    maximum = nice_ceiling(max(values.values()))
    top = 56 + (22 if len(series) > 1 else 0)
    group = BAR_HEIGHT * len(series) + 12
    body = []
    if len(series) > 1:
        x = plot_left
        for row, item in enumerate(series):
            body.append(f'<rect x="{x}" y="52" width="12" height="12" fill="{PALETTE[row % len(PALETTE)]}"/>')
            body.append(f'<text x="{x + 16}" y="62" font-size="12" {FONT}>{escape(item["label"])}</text>')
            x += 34 + 7 * len(item["label"])
    for index, category in enumerate(categories):
        y0 = top + index * group
        body.append(f'<text x="{plot_left - 8}" y="{y0 + (group - 12) / 2:.0f}" font-size="12" text-anchor="end" '
                    f'dominant-baseline="middle" {FONT}>{escape(str(category))}</text>')
        for row, item in enumerate(series):
            value = values.get((item["label"], category))
            if value is None:
                continue
            y = y0 + row * BAR_HEIGHT
            width = plot_width * value / maximum
            body.append(f'<rect x="{plot_left}" y="{y + 3}" width="{width:.1f}" height="{BAR_HEIGHT - 6}" '
                        f'fill="{PALETTE[row % len(PALETTE)]}"><title>{escape(item["label"])}: '
                        f'{format_value(value)} {escape(chart["unit"])}</title></rect>')
            body.append(f'<text x="{plot_left + width + 5:.1f}" y="{y + BAR_HEIGHT / 2:.0f}" font-size="11" '
                        f'dominant-baseline="middle" {FONT}>{format_value(value)}</text>')
    bottom = top + len(categories) * group
    body.append(f'<line x1="{plot_left}" y1="{top - 4}" x2="{plot_left}" y2="{bottom}" stroke="#999"/>')
    for tick in range(6):
        x = plot_left + plot_width * tick / 5
        body.append(f'<line x1="{x:.1f}" y1="{bottom}" x2="{x:.1f}" y2="{bottom + 4}" stroke="#999"/>')
        body.append(f'<text x="{x:.1f}" y="{bottom + 16}" font-size="10" text-anchor="middle" fill="#555" '
                    f'{FONT}>{format_value(maximum * tick / 5)}</text>')
    better = "higher is better" if chart.get("higher_is_better") else "lower is better"
    body.append(f'<text x="{plot_left + plot_width / 2:.0f}" y="{bottom + 32}" font-size="11" text-anchor="middle" '
                f'{FONT}>{escape(chart["unit"])} ({better})</text>')
    y = bottom + 40
    for note in chart.get("notes", []):
        body.append(f'<text x="12" y="{y + 14:.0f}" font-size="11" fill="#b00020" {FONT}>{escape(note)}</text>')
        y += 16
    notes, height = footer(environment, chart["command"], y)
    return svg(chart, height, body + notes)


def render(results: dict[str, Any]) -> dict[str, str]:
    return {f"{chart['id']}.svg": bar_chart(chart, results["environment"]) for chart in results["charts"]}


def mdx(text: str) -> str:
    """Text safe in MDX, which reads <, > and braces as JSX."""
    return escape(str(text), quote=False).replace("{", "&#123;").replace("}", "&#125;")


def render_page(results: dict[str, Any], intro: str, image_base: str = "/bench") -> str:
    """The Performance page: the environment, then every chart with its method and its numbers as a table."""
    env = results["environment"]
    lines = ["---", "id: performance", 'title: "Performance"', "sidebar_label: Performance", "---", "",
             "{/* Generated by scripts/bench/render_charts.py from doc/website/static/bench/results.json. "
             "Do not edit; edit scripts/bench/performance_intro.md instead. */}", "", intro.rstrip(), "",
             "## Test system", "", "| | |", "|---|---|",
             f"| Machine | {env['machine']} |", f"| Operating system | {env['os']} |",
             f"| Compiler | {env['compiler']} |", f"| MNE-CPP commit | `{env['commit']}` |",
             f"| Measured | {env['date']} |"]
    if env.get("reference"):
        lines.append(f"| Reference | {env['reference']} |")
    for chart in results["charts"]:
        lines += ["", f"## {mdx(chart['title'])}", ""]
        if chart.get("description"):
            lines += [chart["description"], ""]
        lines += [f"![{mdx(chart['title'])}]({image_base}/{chart['id']}.svg)", ""]
        labels = [s["label"] for s in chart["series"]]
        categories = list(dict.fromkeys(p["x"] for s in chart["series"] for p in s["points"]))
        values = {(s["label"], p["x"]): p["y"] for s in chart["series"] for p in s["points"]}
        lines += ["| | " + " | ".join(mdx(label) for label in labels) + " |", "|---|" + "---:|" * len(labels)]
        for category in categories:
            cells = [format_value(values[(label, category)]) + f" {chart['unit']}" if (label, category) in values else "–"
                     for label in labels]
            lines.append(f"| {mdx(category)} | " + " | ".join(mdx(cell) for cell in cells) + " |")
        for note in chart.get("notes", []):
            lines += ["", f":::caution\n{mdx(note)}\n:::"]
        lines += ["", f"Reproduce: `{chart['command']}`"]
    return "\n".join(lines) + "\n"


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("results", type=Path, help="results JSON from run_benchmarks.py")
    parser.add_argument("out_dir", type=Path, help="directory for the SVG files")
    parser.add_argument("--page", type=Path, help="also write the Performance page (MDX) here")
    parser.add_argument("--check", action="store_true", help="fail if a committed SVG or the page differs")
    args = parser.parse_args(argv)
    results = json.loads(args.results.read_text(encoding="utf-8"))
    files = {args.out_dir / name: text for name, text in render(results).items()}
    if args.page:
        intro = (Path(__file__).resolve().parent / "performance_intro.md").read_text(encoding="utf-8")
        files[args.page] = render_page(results, intro)
    stale = []
    for path, text in files.items():
        if args.check:
            if not path.exists() or path.read_text(encoding="utf-8") != text:
                stale.append(path)
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(text, encoding="utf-8")
            print(f"wrote {path}")
    for path in stale:
        print(f"STALE {path}; run scripts/bench/render_charts.py")
    return 1 if stale else 0


if __name__ == "__main__":
    raise SystemExit(main())
