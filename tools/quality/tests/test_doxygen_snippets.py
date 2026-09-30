#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Fixture for the Doxygen @snippet contract between library headers and src/examples (T3.1).
"""Fixture for ``@snippet`` references from library headers into ``src/examples``.

The static tests run anywhere. The Doxygen tests need ``doxygen`` on PATH; they
are skipped without it unless ``MNE_REQUIRE_DOXYGEN=1``, which the Doxygen CI
jobs set so that a missing binary fails instead of passing silently.
"""

from __future__ import annotations

import os
import re
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
EXAMPLES = REPO_ROOT / "src" / "examples"
LIBRARIES = REPO_ROOT / "src" / "libraries"
REFERENCE_HEADER = LIBRARIES / "fiff" / "fiff_raw_data.h"
REFERENCE_EXAMPLE = EXAMPLES / "ex_read_raw" / "main.cpp"
REFERENCE_REGIONS = ("fiff_raw_data_open", "fiff_raw_data_read_segment")
SNIPPET = re.compile(r"[@\\]snippet\s+(\S+)\s+(\S+)")

DOXYGEN = shutil.which("doxygen")
REQUIRE_DOXYGEN = os.environ.get("MNE_REQUIRE_DOXYGEN") == "1"


def snippet_references() -> list[tuple[Path, int, str, str]]:
    refs = []
    for header in sorted(LIBRARIES.rglob("*.h")):
        for number, line in enumerate(header.read_text(encoding="utf-8", errors="replace").splitlines(), 1):
            for match in SNIPPET.finditer(line):
                refs.append((header, number, match.group(1), match.group(2)))
    return refs


def region(source: Path, name: str) -> list[str]:
    lines = source.read_text(encoding="utf-8").splitlines()
    marks = [index for index, line in enumerate(lines) if line.strip() == f"//! [{name}]"]
    if len(marks) != 2:
        raise AssertionError(f"{source}: marker [{name}] appears {len(marks)} times, expected 2")
    return lines[marks[0] + 1:marks[1]]


def codeline_text(codeline: ET.Element) -> str:
    parts = []
    for element in codeline.iter():
        if element.tag == "sp":
            parts.append(" ")
        elif element is not codeline and element.text:
            parts.append(element.text)
        if element is not codeline and element.tail:
            parts.append(element.tail)
    return "".join(parts)


class TestSnippetReferences(unittest.TestCase):
    def test_doxyfile_searches_the_examples_tree(self) -> None:
        text = (REPO_ROOT / "doc" / "Doxyfile").read_text(encoding="utf-8")
        self.assertRegex(text, r"(?m)^EXAMPLE_PATH\s*=\s*\.\./src/examples\s*$")
        self.assertRegex(text, r"(?m)^EXAMPLE_RECURSIVE\s*=\s*YES\s*$")
        self.assertRegex(text, r"(?m)^CASE_SENSE_NAMES\s*=\s*NO\s*$")

    def test_references_are_qualified_and_resolve(self) -> None:
        """AC-T3.1-3: every example is main.cpp, so a reference must name its directory."""
        refs = snippet_references()
        self.assertGreaterEqual(len(refs), len(REFERENCE_REGIONS))
        for header, number, path, name in refs:
            with self.subTest(ref=f"{header.relative_to(REPO_ROOT)}:{number}"):
                self.assertIn("/", path, f"bare basename {path!r} is ambiguous under src/examples")
                self.assertTrue((EXAMPLES / path).is_file(), f"src/examples/{path} does not exist")
                self.assertTrue(region(EXAMPLES / path, name))

    def test_reference_pair_is_wired(self) -> None:
        refs = {(path, name) for header, _, path, name in snippet_references() if header == REFERENCE_HEADER}
        self.assertEqual({("ex_read_raw/main.cpp", name) for name in REFERENCE_REGIONS}, refs)


@unittest.skipUnless(DOXYGEN or REQUIRE_DOXYGEN, "doxygen not on PATH")
class TestDoxygenXml(unittest.TestCase):
    """Run Doxygen on the reference header only, against the real examples tree."""

    def run_doxygen(self, header_text: str) -> tuple[subprocess.CompletedProcess, Path]:
        self.assertIsNotNone(DOXYGEN, "MNE_REQUIRE_DOXYGEN=1 but doxygen is not on PATH")
        tmp = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, tmp)
        (tmp / "fiff_raw_data.h").write_text(header_text, encoding="utf-8")
        config = "\n".join([
            f"INPUT = {tmp / 'fiff_raw_data.h'}",
            f"EXAMPLE_PATH = {EXAMPLES}",
            "EXAMPLE_RECURSIVE = YES",
            f"OUTPUT_DIRECTORY = {tmp / 'out'}",
            "GENERATE_XML = YES", "GENERATE_HTML = NO", "GENERATE_LATEX = NO",
            "XML_PROGRAMLISTING = NO", "QUIET = YES", "WARN_IF_UNDOCUMENTED = NO",
            "MACRO_EXPANSION = YES", "PREDEFINED = FIFFSHARED_EXPORT=",
        ])
        result = subprocess.run([DOXYGEN, "-"], input=config, capture_output=True, text=True, check=False)
        return result, tmp / "out" / "xml"

    def listings(self, xml_dir: Path) -> list[ET.Element]:
        # Compound file names depend on the platform's CASE_SENSE_NAMES default, so match by name.
        for path in xml_dir.glob("class*.xml"):
            compound = ET.parse(path).getroot().find("compounddef")
            if compound is not None and compound.findtext("compoundname") == "FIFFLIB::FiffRawData":
                return list(compound.iter("programlisting"))
        self.fail("FiffRawData is not documented inside FIFFLIB")

    def test_xml_contains_exactly_the_named_regions(self) -> None:
        """AC-T3.1-1 and AC-T3.1-2: each listing is its region, line for line, and nothing else."""
        result, xml_dir = self.run_doxygen(REFERENCE_HEADER.read_text(encoding="utf-8"))
        self.assertNotIn("snippet", result.stderr)
        listings = self.listings(xml_dir)
        self.assertEqual(len(REFERENCE_REGIONS), len(listings))
        for listing, name in zip(listings, REFERENCE_REGIONS):
            with self.subTest(region=name):
                self.assertEqual("ex_read_raw/main.cpp", listing.get("filename"))
                got = [codeline_text(line) for line in listing.findall("codeline")]
                self.assertEqual(region(REFERENCE_EXAMPLE, name), got)
                self.assertFalse(any("QCommandLineParser" in line or "make_compensator" in line for line in got))

    def test_bare_basename_is_rejected(self) -> None:
        """AC-T3.1-3: an unqualified main.cpp is ambiguous and yields an empty listing."""
        header = REFERENCE_HEADER.read_text(encoding="utf-8").replace(
            "@snippet ex_read_raw/main.cpp fiff_raw_data_open", "@snippet main.cpp fiff_raw_data_open")
        result, xml_dir = self.run_doxygen(header)
        self.assertIn("ambiguous", result.stderr)
        first, second = self.listings(xml_dir)
        self.assertEqual(("main.cpp", []), (first.get("filename"), first.findall("codeline")))
        self.assertEqual(region(REFERENCE_EXAMPLE, REFERENCE_REGIONS[1]),
                         [codeline_text(line) for line in second.findall("codeline")])


if __name__ == "__main__":
    unittest.main()
