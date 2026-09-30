#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     September, 2026
#
#
# @brief    Fixtures for @snippet rendering in tools/doxy2mdx/doxy2mdx.py (T3.3).
"""Fixtures for how ``doxy2mdx`` renders ``@snippet`` program listings into MDX."""

from __future__ import annotations

import importlib.util
import io
import json
import shutil
import sys
import tempfile
import textwrap
import unittest
from contextlib import redirect_stderr
from pathlib import Path
from xml.sax.saxutils import escape

_REPO_ROOT = Path(__file__).resolve().parents[3]
_spec = importlib.util.spec_from_file_location("doxy2mdx_t33", _REPO_ROOT / "tools" / "doxy2mdx" / "doxy2mdx.py")
assert _spec is not None and _spec.loader is not None
doxy2mdx = importlib.util.module_from_spec(_spec)
sys.modules["doxy2mdx_t33"] = doxy2mdx
_spec.loader.exec_module(doxy2mdx)

EXAMPLE = """int main()
{
    QStringList include;
    //! [widget_use]
    include << "STI 014";   // <keep> & {braces}
    if (include.size() > 0) {
        return 1;
    }
    //! [widget_use]
    return 0;
}
"""
REGION = ['    include << "STI 014";   // <keep> & {braces}',
          "    if (include.size() > 0) {", "        return 1;", "    }"]
HEADER = "/**\n * SPDX-License-Identifier: BSD-3-Clause\n * @author   Jane Doe <jane@example.org>\n */\n"


def _codeline(text: str) -> str:
    body = escape(text).replace(" ", "<sp/>")
    return f'<codeline><highlight class="normal">{body}</highlight></codeline>'


def _listing(lines: list[str], filename: str | None = "ex_widget/main.cpp") -> str:
    attr = f' filename="{filename}"' if filename is not None else ""
    return f"<programlisting{attr}>" + "".join(_codeline(line) for line in lines) + "</programlisting>"


def _compound(detail: str) -> str:
    return textwrap.dedent(f"""\
        <?xml version='1.0' encoding='UTF-8' standalone='no'?>
        <doxygen><compounddef id="class_d_e_m_o_1_1_widget" kind="class">
        <compoundname>DEMO::Widget</compoundname>
        <briefdescription><para>A widget.</para></briefdescription>
        <detaileddescription><para>Use it:</para><para>{detail}</para></detaileddescription>
        <location file="demo/widget.h"/>
        </compounddef></doxygen>
        """)


class SnippetRenderingTestCase(unittest.TestCase):
    def setUp(self) -> None:
        self.root = Path(tempfile.mkdtemp())
        self.addCleanup(shutil.rmtree, self.root)
        example = self.root / "src" / "examples" / "ex_widget"
        example.mkdir(parents=True)
        (example / "main.cpp").write_text(EXAMPLE, encoding="utf-8")
        (self.root / "src" / "libraries" / "demo").mkdir(parents=True)
        (self.root / "src" / "libraries" / "demo" / "widget.h").write_text(HEADER, encoding="utf-8")
        self.entry = {"name": "Widget", "module": "demo", "header": "demo/widget.h", "documented": True,
                      "example": "ex_widget", "example_snippet": ["ex_widget/main.cpp#widget_use"],
                      "example_mode": "run", "required_data": ["mne-cpp-test-data"],
                      "screenshots": ["demo/overview"], "sidebar_position": 1}
        self.xml = self.root / "xml"
        self.xml.mkdir()

    def run_generator(self, detail: str, entry: dict | None = None) -> tuple[int | str, str]:
        registry = {"modules": {"demo": {"dir_slug": "demo", "namespace": "DEMO", "include": "demo"}},
                    "classes": [entry or self.entry]}
        (self.root / "doc").mkdir(exist_ok=True)
        (self.root / "doc" / "api_registry.json").write_text(json.dumps(registry), encoding="utf-8")
        (self.xml / "class_d_e_m_o_1_1_widget.xml").write_text(_compound(detail), encoding="utf-8")
        (self.xml / "index.xml").write_text(
            '<doxygenindex><compound refid="class_d_e_m_o_1_1_widget" kind="class">'
            "<name>DEMO::Widget</name></compound></doxygenindex>", encoding="utf-8")
        out = self.root / "api"
        doxy2mdx._FILES_WITHOUT_AUTHOR.clear()
        argv = ["--xml-dir", str(self.xml), "--out-dir", str(out), "--registry",
                str(self.root / "doc" / "api_registry.json"), "--repo-root", str(self.root),
                "--classes", "DEMO::Widget"]
        try:
            with redirect_stderr(io.StringIO()):
                code: int | str = doxy2mdx.main(argv)
        except SystemExit as exc:
            code = str(exc)
        page = out / "demo" / "widget.mdx"
        return code, page.read_text(encoding="utf-8") if page.is_file() else ""


class TestSnippetRendering(SnippetRenderingTestCase):
    def test_region_renders_byte_for_byte_after_dedent(self) -> None:
        """AC-T3.3-1: the fenced block equals the source region, dedented, with no escaping."""
        code, page = self.run_generator(_listing(REGION))
        self.assertEqual(0, code)
        expected = textwrap.dedent("\n".join(REGION))
        self.assertIn(f'```cpp title="src/examples/ex_widget/main.cpp"\n{expected}\n```', page)

    def test_plain_code_block_is_unchanged(self) -> None:
        entry = {key: value for key, value in self.entry.items() if key != "example_snippet"}
        code, page = self.run_generator(_listing(["int x = 1;"], filename=None), entry)
        self.assertEqual(0, code, page)
        self.assertIn("```cpp\nint x = 1;\n```", page)

    def test_example_metadata_links_source_instead_of_copying_it(self) -> None:
        """AC-T3.3-3: the example section links the source and never inlines the file."""
        code, page = self.run_generator(_listing(REGION))
        self.assertEqual(0, code)
        example = page[page.index("## Example"):page.index("## Authors")]
        self.assertIn("src/examples/ex_widget/main.cpp", example)
        self.assertIn("(runs under CTest)", example)
        self.assertIn("mne-cpp-test-data", example)
        self.assertIn("![demo/overview](/img/manual/auto/demo/overview.png)", example)
        self.assertNotIn("```", example)
        self.assertNotIn("return 0;", page)


class TestStrictErrors(SnippetRenderingTestCase):
    def assert_fails(self, detail: str, fragment: str, entry: dict | None = None) -> None:
        code, _ = self.run_generator(detail, entry)
        self.assertIsInstance(code, str, "generator did not fail")
        self.assertIn(fragment, code)

    def test_empty_listing_fails(self) -> None:
        """AC-T3.3-2: Doxygen emits an empty listing for a missing region or ambiguous file."""
        self.assert_fails(_listing([]), "empty listing")

    def test_listing_that_is_not_a_region_fails(self) -> None:
        """AC-T3.3-2: a listing that no longer matches the source region (stale XML) fails."""
        self.assert_fails(_listing(REGION[:-1]), "matches 0 regions")

    def test_unknown_example_file_fails(self) -> None:
        self.assert_fails(_listing(REGION, filename="ex_gone/main.cpp"), "no such file")

    def test_registered_snippet_must_render(self) -> None:
        """A registry example_snippet without a matching @snippet in the XML fails."""
        self.assert_fails("", "is not rendered from the Doxygen XML")

    def test_malformed_xml_fails(self) -> None:
        """AC-T3.3-2: unparseable compound XML is an error, not a skipped page."""
        (self.xml / "class_d_e_m_o_1_1_widget.xml").write_text("<doxygen><compounddef", encoding="utf-8")
        (self.xml / "index.xml").write_text(
            '<doxygenindex><compound refid="class_d_e_m_o_1_1_widget" kind="class">'
            "<name>DEMO::Widget</name></compound></doxygenindex>", encoding="utf-8")
        registry = self.root / "doc" / "api_registry.json"
        registry.parent.mkdir(exist_ok=True)
        registry.write_text(json.dumps({"modules": {"demo": {"dir_slug": "demo"}}, "classes": [self.entry]}),
                            encoding="utf-8")
        with redirect_stderr(io.StringIO()), self.assertRaises(Exception):
            doxy2mdx.main(["--xml-dir", str(self.xml), "--out-dir", str(self.root / "api"),
                           "--registry", str(registry), "--repo-root", str(self.root),
                           "--classes", "DEMO::Widget"])


if __name__ == "__main__":
    unittest.main()
