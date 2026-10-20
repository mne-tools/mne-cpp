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


if __name__ == "__main__":
    unittest.main()
