# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2026 MNE-CPP Authors
#   Christoph Dinh <christoph.dinh@mne-cpp.org>

#
# @since    2.4.0
# @date     October, 2026
#
#
# @brief    Tests for tools/doxy2mdx/generate_examples.py.
"""Tests for the examples gallery generator."""

from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location("generate_examples", REPO_ROOT / "tools/doxy2mdx/generate_examples.py")
gen = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(gen)


class GenerateExamplesTest(unittest.TestCase):
    def test_page_groups_escapes_and_links(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            ex = root / gen.EXAMPLES / "ex_demo"
            ex.mkdir(parents=True)
            (ex / "main.cpp").write_text(
                '/** @brief Fit {dipoles} to <data>. */\n#include <fiff/fiff.h>\n#include <inv/inv.h>\n'
                'QCommandLineOption in({"file", "f"}, "The input <in> | {x}.", "in");\n')
            api = root / gen.API / "inv"
            api.mkdir(parents=True)
            (api / "inv-dipole.mdx").write_text("---\nsidebar_label: InvDipole\n---\nsrc/examples/ex_demo/main.cpp\n")
            out = root / "out"
            gen.generate(root, out)
            page = (out / "inv" / "ex_demo.mdx").read_text()
            index = (out / "index.mdx").read_text()
        self.assertIn("Fit &#123;dipoles&#125; to &lt;data&gt;.", page)
        self.assertIn("| `--file` | The input &lt;in&gt; &#124; &#123;x&#125;. |", page)
        self.assertIn("[Inverse](../../api/inv/index.mdx), [FIFF I/O](../../api/fiff/index.mdx)", page)
        self.assertIn("- [InvDipole](../../api/inv/inv-dipole.mdx)", page)
        self.assertIn("| [ex_demo](./inv/ex_demo.mdx) |", index)

    def test_usages_rank_examples_and_cut_excerpts(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for name, body in {
                "ex_one": "    // InvDipole in a comment\n    int a = 0;\n\n    //! [demo]\n    InvDipole d;\n"
                          "    d.fit();\n    //! [demo]\n",
                "ex_two": "    InvDipole x;\n    InvDipole y;\n",
                "ex_other_lib": "    InvDipole z;\n",
                "ex_prefix": "    MyInvDipole w;\n",
            }.items():
                ex = root / gen.EXAMPLES / name
                ex.mkdir(parents=True)
                lib = "fiff" if name == "ex_other_lib" else "inv"
                (ex / "main.cpp").write_text(f"/** @brief {name} */\n#include <{lib}/x.h>\n{body}")
            found = gen.usages("InvDipole", "inv", root)
        self.assertEqual([(u["name"], u["uses"]) for u in found], [("ex_two", 2), ("ex_one", 1)])
        self.assertEqual(found[1]["code"], ["InvDipole d;", "d.fit();"])
        self.assertEqual(found[1]["line"], 7)

    def test_committed_gallery_is_current(self) -> None:
        self.assertEqual(gen.main(["--check"]), 0)

    def test_api_slug(self) -> None:
        self.assertEqual(gen.api_slug("disp3D"), "disp3-d")

    def test_heading_ids_follow_docusaurus(self) -> None:
        kn = gen.kn
        self.assertEqual(kn.slugify("Contextual Minimum-Norm Estimates (CMNE)"), "contextual-minimum-norm-estimates-cmne")
        self.assertEqual(kn.slugify("calculateInverse(p_fiffEvoked, pick_normal)"), "calculateinversep_fiffevoked-pick_normal")
        with tempfile.TemporaryDirectory() as tmp:
            page = Path(tmp) / "p.mdx"
            page.write_text("# Title\n\n### getKernel()\n\n```cpp\n# not a heading\n```\n\n### getKernel()\n\n"
                            "## Custom {#my-id}\n")
            self.assertEqual([hid for _text, hid in kn.headings(page)], ["title", "getkernel", "getkernel-1", "my-id"])

    def test_unresolved_registry_references_fail(self) -> None:
        kn = gen.kn
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / kn.DOCS / "manual").mkdir(parents=True)
            (root / kn.DOCS / "manual" / "m.mdx").write_text("---\ntitle: M\n---\n\n## Known\n")
            (root / kn.DOCS / "api" / "inv").mkdir(parents=True)
            (root / kn.DOCS / "api" / "inv" / "inv-a.mdx").write_text('---\ntitle: "INVLIB::InvA"\n---\n\n### run(x)\n')
            topic = {"id": "t", "title": "T", "summary": "S", "manual": ["manual/m.mdx#known", "manual/m.mdx#gone"],
                     "tools": ["manual/none.mdx"], "classes": ["InvA", "InvB"], "methods": ["InvA::run", "InvA::walk"],
                     "examples": ["ex_a", "ex_missing"], "start": "ex_a"}
            facets = {"tasks": {"io": "IO"}, "modalities": {"meg": "MEG"}, "levels": {"start": "Start"}}
            registry = {"classes": [{"name": "InvA"}], "topics": {"entries": [topic]},
                        "examples": {"facets": facets, "entries": {
                            "ex_a": {"tasks": ["io"], "modalities": ["meg"], "level": "start"},
                            "ex_b": {"tasks": ["nope"], "modalities": ["meg"], "level": "start"}}}}
            (root / kn.REGISTRY).parent.mkdir(parents=True, exist_ok=True)
            (root / kn.REGISTRY).write_text(json.dumps(registry))
            with self.assertRaises(ValueError) as caught:
                kn.load(root, ["ex_a", "ex_c"])
        errors = str(caught.exception).splitlines()
        self.assertEqual(sorted(errors), sorted([
            "examples: src/examples/ex_c has no entry",
            "examples: 'ex_b' is not a directory in src/examples",
            "examples.ex_b: unknown task 'nope'",
            "topics.t: manual/m.mdx#gone: no heading with id 'gone'",
            "topics.t: manual/none.mdx: no such page under doc/website/docs",
            "topics.t: class InvB is not in the registry",
            "topics.t: InvA::walk has no heading on inv-a.mdx",
            "topics.t: example ex_missing does not exist",
        ]))

    def test_committed_topics_connect_theory_api_and_examples(self) -> None:
        names = [p.name for p in sorted((REPO_ROOT / gen.EXAMPLES).glob("ex_*/"))]
        know = gen.kn.load(REPO_ROOT, names)
        self.assertGreaterEqual(len(know.topics), 10)
        for topic in know.topics:
            with self.subTest(topic=topic["id"]):
                self.assertTrue(topic["manual"] and topic["api"] and topic["examples"])
                self.assertEqual(sum(e["start"] for e in topic["examples"]), 1)

    def test_moved_example_keeps_its_old_url(self) -> None:
        stub = REPO_ROOT / gen.REDIRECTS / "connectivity" / "ex_dsp_rt" / "index.html"
        self.assertIn('url=../../dsp/ex_dsp_rt/"', stub.read_text())
        self.assertTrue((REPO_ROOT / gen.OUT / "dsp" / "ex_dsp_rt.mdx").is_file())


if __name__ == "__main__":
    unittest.main()
